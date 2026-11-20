// O RITMO DO VIDRO NA ESCADA DE EFEITOS (auditoria 20/11, P2).
//
// Antes: o nivel 2 do ritmo do vidro (fundo a cada 3 quadros, 1 passada) so
// existia em gfx_vidro_governar, que nao tinha chamador em producao. O teste
// vidro_qualidade passava chamando a funcao direto. Aqui o caminho e o de
// producao: a TV simulada entrega quadros como main.c (gpun_medir a cada
// quadro, gpun_relatar a cada 3 s, gpun_quadro_inicio antes de desenhar) e o
// que se confere e o que chega ao gfx (gfx_vidro_nivel_definir) e o que os
// Ajustes respondem (qualidade do vidro).
//
// A TV simulada: 60 Hz; o custo depende do que a escada ligou.
//   vidro fosco ritmo 1 -> cadencia 1-2-2 (36 fps, a Home da TCL)
//   vidro fosco ritmo 2 -> o que cada cenario disser (`fpsRitmo2`)
//   vidro esfumacado (degrau 1) ou sem vidro -> 60 fps
// Os numeros de FPS sao do MODELO, nao da TCL: o teste prova a REGRA (ordem,
// condicoes, volta), nao o ganho real -- esse so a TV mede.
#include "gpunivel.h"
#include "gfx.h"
#include "dados.h"
#include "perfiltv.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gpun_teste_reiniciar(void);

static int falhas;
#define OK(c, ...) do { if (c) { printf("PASS: "); printf(__VA_ARGS__); printf("\n"); } \
                         else { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); falhas++; } } while (0)

// --- dubles de gfx/GL/dados (como tests/gpunivel.c) --------------------------
SDL_GLContext SDL_GL_GetCurrentContext(void) { return NULL; }
float gfx_tex_aspect_atual, gfx_opacidade_grupo = 1.0f;
int gfx_hdr_output;
void gfx_definir_efeitos_leves(int l) { (void)l; }
void gfx_definir_efeitos_minimos(int m) { (void)m; }
int  gfx_efeitos_leves(void) { return 0; }
void gfx_tex_esquecer(GLuint t) { (void)t; }
void gfx_tamanho_alvo(int w, int h) { (void)w; (void)h; }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float px, float py, float raio,
              float cr, float cg, float cb, float ca) {
  (void)r; (void)tex; (void)modo; (void)foco; (void)px; (void)py; (void)raio; (void)cr; (void)cg; (void)cb; (void)ca;
}
// O que o gfx recebe da escada: e isto que decide o fundo do vidro (gfx.c, vidNivel).
static int gfxNivel = -1, gfxChamadas;
void gfx_vidro_nivel_definir(int n) { gfxNivel = n; gfxChamadas++; }
static char gravado[512];
int dados_gravar(const char *nome, const char *c) { (void)nome; snprintf(gravado, sizeof gravado, "%s", c); return 1; }
char *dados_ler(const char *nome) { (void)nome; return gravado[0] ? strdup(gravado) : NULL; }
const GLubyte *glGetString(GLenum e) { return (const GLubyte *)(e == GL_RENDERER ? "Mali-G31" : ""); }
void glGetIntegerv(GLenum e, GLint *v) { (void)e; *v = 0; }
#undef glTexImage2D
void glGenTextures(GLsizei n, GLuint *v) { while (n--) *v++ = 1; }
void glGenFramebuffers(GLsizei n, GLuint *v) { while (n--) *v++ = 2; }
void glDeleteTextures(GLsizei n, const GLuint *v) { (void)n; (void)v; }
void glDeleteFramebuffers(GLsizei n, const GLuint *v) { (void)n; (void)v; }
void glBindTexture(GLenum t, GLuint v) { (void)t; (void)v; }
void glBindFramebuffer(GLenum t, GLuint v) { (void)t; (void)v; }
void glTexImage2D(GLenum t, GLint l, GLint f, GLsizei w, GLsizei h, GLint b, GLenum fmt, GLenum ty, const GLvoid *p) {
  (void)t; (void)l; (void)f; (void)w; (void)h; (void)b; (void)fmt; (void)ty; (void)p;
}
void glTexParameteri(GLenum t, GLenum p, GLint v) { (void)t; (void)p; (void)v; }
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tt, GLuint v, GLint l) { (void)t; (void)a; (void)tt; (void)v; (void)l; }
GLenum glCheckFramebufferStatus(GLenum t) { (void)t; return GL_FRAMEBUFFER_COMPLETE; }
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { (void)x; (void)y; (void)w; (void)h; }

// --- Ajustes: as escolhas da pessoa, com a MESMA regra de src/ajustes.c ------
// (ajustes_vidro / _fosco / _qualidade / _efeitos_automaticos, linhas ~3008-3036).
// Interface de vidro (V_LIGA_AUTO): 0 Ligado, 1 Desligado, 2 Automatico.
// Vidro fosco: 1 Ligado, 2 Automatico (a escada pode vetar), outro = desligado.
// Qualidade: 0 Automatica, 1 Desempenho, 2 Alta.
static int vVidro = 2, vFosco = 2, vQual = 0, vAnim = 2;
int ajustes_vidro(void) { return vVidro == 0 || (vVidro == 2 && !gpun_efeito_ativo(4)); }
int ajustes_vidro_fosco(void) { return vFosco == 1 || (vFosco == 2 && !gpun_fosco_vetado()); }
int ajustes_vidro_qualidade(void) { return vQual == 1 ? 1 : vQual == 2 ? 2 : gpun_efeito_ativo(1) ? 1 : 0; }
int ajustes_efeitos_automaticos(void) {
  return ((vQual == 0 && vVidro != 1) ? 1 : 0) | (vFosco == 2 ? 2 : 0) | (vAnim == 2 ? 4 : 0) | (vVidro == 2 ? 8 : 0);
}

// --- a TV simulada -----------------------------------------------------------
static double fpsRitmo2 = 40.0;   // o que o ritmo 2 rende neste cenario
static double tvFps(void) {
  int fosco = ajustes_vidro() && ajustes_vidro_fosco() && ajustes_vidro_qualidade() != 1;
  if (!fosco) return 60.0;
  return gfxNivel == 2 ? fpsRitmo2 : 36.0;
}
// Uma janela de 3 s como main.c: gpun_quadro_inicio + gpun_medir por quadro,
// gpun_relatar no fim, com o periodo do painel (16,67 ms).
static void janela(int cena) {
  double t = 0.0, pior = 0.0;
  int quadros = 0;
  while (t < 3000.0) {
    double dt;
    gpun_quadro_inicio();
    dt = 1000.0 / tvFps();
    gpun_medir(dt, dt - 4.0, 4.0, cena, 1);
    t += dt; quadros++;
    if (dt > pior) pior = dt;
  }
  gpun_relatar(quadros * 1000.0 / t, pior, 1000.0 / 60.0, cena, 1);
}
static void janelas(int n, int cena) { while (n-- > 0) janela(cena); }
static void reinicia(void) {
  gpun_teste_reiniciar(); gravado[0] = 0;
  vVidro = 2; vFosco = 2; vQual = 0; vAnim = 2; fpsRitmo2 = 40.0; gfxNivel = -1;
  gpun_quadro_inicio();
}

int main(void) {
  // 1. A Home da TCL (36 fps em 60 Hz): primeiro o ritmo 2, ainda fosco.
  reinicia();
  OK(gfxNivel == 1 && gpun_vidro_ritmo() == 1, "arranque: gfx recebe ritmo 1 (o padrao)");
  janelas(1, 1);   // a primeira janela de uma cena e descartada (regra da escada)
  janelas(2, 1);
  OK(gpun_vidro_ritmo() == 1 && gpun_degrau() == 0, "2 janelas lentas: nada muda ainda");
  janelas(1, 1);
  gpun_quadro_inicio();
  OK(gpun_vidro_ritmo() == 2 && gfxNivel == 2, "3a janela lenta (36 fps): a escada liga o ritmo 2 e o gfx o recebe (%d)", gfxNivel);
  OK(gpun_degrau() == 0 && ajustes_vidro_qualidade() == 0 && ajustes_vidro_fosco(),
     "o vidro continua fosco (degrau 0, qualidade Automatica)");
  // 1b. O ritmo 2 nao bastou (40 fps < 55): mais 3 janelas lentas e so entao o degrau 1.
  janelas(2, 1);
  OK(gpun_degrau() == 0, "com o ritmo 2, 2 janelas lentas ainda nao provam o vidro esfumacado");
  janelas(1, 1);
  OK(gpun_degrau() == 1 && ajustes_vidro_qualidade() == 1, "3 janelas lentas no ritmo 2 -> prova do degrau 1 (esfumacado)");
  janelas(3, 1);   // 40 -> 60 fps: ganho >= 10 % e >= 3 fps em 3 janelas
  OK(gpun_degrau() == 1 && gpun_efeito_ativo(1), "esfumacado com ganho medido: mantido");
  gpun_quadro_inicio();
  OK(gfxNivel == 1, "com o esfumacado o gfx volta ao ritmo 1 (nao ha fundo a capturar)");

  // 2. O ritmo 2 basta (60 fps): a TV fica no vidro FOSCO, sem descer degrau.
  reinicia(); fpsRitmo2 = 60.0;
  janelas(4, 1);
  OK(gpun_vidro_ritmo() == 2, "ritmo 2 ligado pela lentidao");
  janelas(9, 1);
  OK(gpun_degrau() == 0 && ajustes_vidro_qualidade() == 0 && gpun_vidro_ritmo() == 2,
     "60 fps com o ritmo 2: fica fosco, degrau 0 (9 janelas rapidas: ainda sem volta)");
  // 2b. 10 janelas rapidas: tenta voltar ao ritmo 1; a Home volta a 36 -> ritmo 2 de novo
  //     e a proxima volta espera 100 relatorios (sem vai-e-vem a cada 40 s).
  janelas(1, 1);
  OK(gpun_vidro_ritmo() == 1, "10 janelas rapidas: tentativa de volta ao ritmo 1");
  janelas(3, 1);
  OK(gpun_vidro_ritmo() == 2, "lento de novo: ritmo 2");
  janelas(30, 1);
  OK(gpun_vidro_ritmo() == 2, "volta que nao segurou: 30 janelas rapidas nao tentam de novo (espera de 100)");
  { int k = 0;
    while (k < 120 && gpun_vidro_ritmo() == 2) { janela(1); k++; }
    // ja passaram 3 + 30 relatorios desde o recomeco da espera: faltam ~70.
    OK(gpun_vidro_ritmo() == 1 && k >= 60 && k <= 72, "passada a espera de 100 relatorios, tenta de novo (depois de mais %d janelas)", k); }

  // 3. A Home sadia (60 fps) nunca mexe no vidro.
  // (no modelo o vidro fosco custa 36 fps; a Home sadia e a TV sem vidro.)
  reinicia(); vVidro = 1;   // Desligado
  janelas(40, 1);
  OK(gpun_vidro_ritmo() == 1 && gpun_degrau() == 0, "60 fps (sem vidro): ritmo 1, degrau 0 depois de 2 min");

  // 4. Qualidade Alta: o desfoque de sempre. A escada nao mexe no ritmo e o gfx recebe 1.
  reinicia(); vQual = 2;
  janelas(12, 1);
  gpun_quadro_inicio();
  OK(gpun_vidro_ritmo() == 1 && gfxNivel == 1 && ajustes_vidro_qualidade() == 2,
     "Alta: ritmo 1 sempre, mesmo a 36 fps (a escada segue nos outros degraus: %d)", gpun_degrau());

  // 5. Fosco ja vetado (degrau 2 gravado de outra vez): sem desfoque, sem ritmo.
  reinicia(); vFosco = 0;
  janelas(4, 1);
  OK(gpun_vidro_ritmo() == 1, "fosco desligado: o ritmo nao entra (nao ha desfoque a poupar)");

  // 6. Troca de cena NAO desfaz o ritmo; a janela da troca e descartada.
  reinicia();
  janelas(4, 1);
  OK(gpun_vidro_ritmo() == 2, "ritmo 2 na Home");
  janelas(3, 2);   // Explorar
  OK(gpun_vidro_ritmo() == 2, "trocar de tela mantem o ritmo 2");

  // 7. Ajuste explicito "Efeitos visuais" (nao Automatico): a escada nao age.
  reinicia(); gpun_preferencia(1);
  janelas(12, 1);
  OK(gpun_vidro_ritmo() == 1, "efeitos manuais: escada parada, ritmo 1");

  if (falhas) { printf("gpun_vidro: %d falha(s)\n", falhas); return 1; }
  puts("gpun_vidro: ok");
  return 0;
}
