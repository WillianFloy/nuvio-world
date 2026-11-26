// FUNDACAO DAS PECAS R8 (Fase 2), com o desenho de verdade (gfx + text):
//  1. material: no Grafite o miolo e a Tinta (14,14,16); no Preto puro e #000;
//     nos dois o fio de 1,5 px aparece na borda e a sombra escurece embaixo;
//  2. foco de acao (veu osso + contorno de 2 px) e de conteudo (3 px);
//  3. o peso 800 (TXT_R8_*X) e a Montserrat ExtraBold na Montserrat — mais
//     largo que o 700 — e o Bold da propria familia nas outras (Inter);
//  4. os icones r8_* das pranchas carregam e desenham;
//  5. a copia parada recuada: escala em torno da origem, deslocamento, veu na
//     mesma passada, e a faixa deslocada do Desdobrar.
// Salva r8-fundacao.png na pasta dada (para olhar).
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "r8estilo.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas;
#define OK(c, ...) do { if (!(c)) { printf("FALHOU: "); printf(__VA_ARGS__); printf("\n"); falhas++; } else { printf("ok  "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void px(int x, int y, int *r, int *g, int *b) {
  unsigned char p[4];
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(x, 1079 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
  *r = p[0]; *g = p[1]; *b = p[2];
}
static int lum(int x, int y) { int r, g, b; px(x, y, &r, &g, &b); return (r + g + b) / 3; }
static void limpar(float c) { glClearColor(c, c, c, 1); glClear(GL_COLOR_BUFFER_BIT); }

// Pixels acesos do texto "Editar atalhos" desenhado no estilo `e`.
static int tinta(TxtEstilo e) {
  int n = 0;
  unsigned char *b = malloc(600 * 80 * 4);
  limpar(0); gfx_novo_quadro(); txt_novo_quadro();
  TxtLinha l = txt_linha(e, "Editar atalhos", 255, 255, 255, 255);
  txt_desenhar(l, 100, 100);
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(100, 1080 - 100 - 80, 600, 80, GL_RGBA, GL_UNSIGNED_BYTE, b);
  for (int i = 0; i < 600 * 80; i++) if (b[i * 4] > 128) n++;
  free(b);
  return n;
}

// GLES so garante RGBA na leitura: le em RGBA e grava a superficie de 32 bits.
static void salvar(const char *dir, const char *nome) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, 1920, 1080, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
  unsigned char *p = s->pixels, *row = malloc((size_t)s->pitch);
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
  for (int y = 0; y < 540; y++) {
    memcpy(row, p + y * s->pitch, (size_t)s->pitch);
    memcpy(p + y * s->pitch, p + (1079 - y) * s->pitch, (size_t)s->pitch);
    memcpy(p + (1079 - y) * s->pitch, row, (size_t)s->pitch);
  }
  for (int i = 0; i < 1920 * 1080; i++) p[i * 4 + 3] = 255;
  char c[1024]; snprintf(c, sizeof c, "%s/%s.png", dir, nome);
  IMG_SavePNG(s, c); SDL_FreeSurface(s); free(row);
  printf("PNG: %s\n", c);
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : "/tmp";
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_Window *w = SDL_CreateWindow("r8", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  SDL_GLContext gl = SDL_GL_CreateContext(w);
  if (!gl) { printf("sem GL\n"); return 2; }
  GLuint fbo, tex;
  glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  if (!gfx_iniciar() || !txt_iniciar("deploy/app", 1)) { printf("sem gfx/txt\n"); return 2; }
  tex_iniciar(32);
  gfx_icones_dir("deploy/app/art");
  gfx_snap_iniciar(1920, 1080);
  GfxRect P = { 600, 200, 640, 400 };

  // ---- 1. material, Grafite e Preto puro
  for (int tema = 0; tema < 2; tema++) {
    int r, g, b, borda, fora, sob, longe;
    gfx_tema_preto = tema;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, 1920, 1080);
    limpar(0.35f); gfx_novo_quadro();
    r8_material(P, R8_RAIO_FOLHA, 1.0f);
    px(920, 400, &r, &g, &b);
    if (!tema) OK(abs(r - 14) <= 2 && abs(g - 14) <= 2 && abs(b - 16) <= 2 + 1, "Grafite: miolo e a Tinta (14,14,16): %d,%d,%d", r, g, b);
    else OK(r <= 1 && g <= 1 && b <= 1, "Preto puro: miolo e #000: %d,%d,%d", r, g, b);
    borda = lum(920, 200 - 1); fora = lum(920, 200 - 60); sob = lum(920, 600 + 40); longe = lum(100, 1000);
    OK(borda > lum(920, 205) + 8, "tema %d: o fio de 1,5 px aparece na borda (%d sobre o miolo %d)", tema, borda, lum(920, 205));
    OK(sob < longe - 20, "tema %d: a sombra escurece embaixo (%d contra %d longe)", tema, sob, longe);
    OK(sob < fora, "tema %d: a sombra e mais forte embaixo (dy 30) que em cima (%d < %d)", tema, sob, fora);
  }
  gfx_tema_preto = 0;

  // ---- 2. foco
  limpar(14.0f / 255.0f); gfx_novo_quadro();
  r8_foco_acao((GfxRect){ 100, 100, 400, 60 }, R8_RAIO_PAINEL, 1.0f);
  r8_foco_conteudo((GfxRect){ 100, 300, 300, 200 }, R8_RAIO_CARTAZ, 1.0f);
  { int r, g, b, miolo = lum(300, 130), contorno, cont3a, cont3b;
    px(300, 101, &r, &g, &b); contorno = (r + g + b) / 3;
    OK(miolo > 40 && miolo < 60, "foco de acao: veu osso 13 %% no miolo (%d)", miolo);
    OK(contorno > 220, "foco de acao: contorno osso na borda (%d)", contorno);
    cont3a = lum(250, 302); cont3b = lum(250, 305);
    OK(cont3a > 220 && cont3b < 40, "foco de conteudo: contorno de 3 px por dentro (%d em +2, %d em +5)", cont3a, cont3b); }

  // ---- 3. peso 800
  { int x8, x7, i8, i7;
    txt_definir_fonte_interface(TXT_FAMILIA_MONTSERRAT);
    x8 = txt_largura(TXT_R8_40X, "Salvos 12"); x7 = txt_largura(TXT_R8_44B, "Salvos 12") * 40 / 44;
    OK(x8 > 0 && x8 > x7 + 4, "Montserrat: 800 (ExtraBold) mais largo que o 700 (%d > %d)", x8, x7);
    txt_definir_fonte_interface(TXT_FAMILIA_INTER);
    i8 = txt_largura(TXT_R8_44X, "Salvos 12"); i7 = txt_largura(TXT_R8_44B, "Salvos 12");
    OK(i8 == i7 && i8 > 0, "Inter: 800 cai no Bold da propria familia (%d == %d)", i8, i7);
    // 600 (Montserrat SemiBold): entre o 500 e o 700 na Montserrat; Bold na Inter.
    { int m5, m6, m7, n6, n7;
      txt_definir_fonte_interface(TXT_FAMILIA_MONTSERRAT);
      // A largura quase nao muda entre os pesos da Montserrat; a TINTA (pixels
      // acesos do traco) sim: e ela que separa 500, 600 e 700.
      m5 = tinta(TXT_R8_26M); m6 = tinta(TXT_R8_26S); m7 = tinta(TXT_R8_26B);
      OK(m6 > m5 * 104 / 100 && m7 > m6 * 104 / 100, "Montserrat: o traco do 600 (SemiBold) fica entre o 500 e o 700 (%d < %d < %d pixels)", m5, m6, m7);
      txt_definir_fonte_interface(TXT_FAMILIA_INTER);
      n6 = txt_largura(TXT_R8_26S, "Editar atalhos"); n7 = txt_largura(TXT_R8_26B, "Editar atalhos");
      OK(n6 == n7 && n6 > 0, "Inter: 600 cai no Bold da propria familia (%d == %d)", n6, n7); }
    txt_definir_fonte_interface(TXT_FAMILIA_MONTSERRAT);
    int c1, s1, mm, pe;
    txt_estilo_medidas(TXT_R8_60X, &c1, &pe); txt_estilo_medidas(TXT_R8_19M, &s1, &mm);
    OK(c1 == 60 && s1 == 19, "corpos das pranchas: 60 e 19 (%d, %d)", c1, s1); }

  // ---- 4. icones
  { const char *n[] = { "r8_bookmark", "r8_bell", "r8_pulse", "r8_palette", "r8_qr", "r8_gauge" };
    for (int i = 0; i < 6; i++) {
      int acesos = 0;
      for (int k = 0; k < 200; k++) {   // a textura chega em alguns quadros
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, 1920, 1080);
        limpar(0); gfx_novo_quadro(); tex_novo_quadro(); tex_bombear(16);
        gfx_icone((GfxRect){ 200, 200, 96, 96 }, n[i], 1, 1, 1, 1);
        acesos = 0;
        for (int y = 200; y < 296; y += 3) for (int x = 200; x < 296; x += 3) if (lum(x, y) > 128) acesos++;
        if (acesos) break;
        SDL_Delay(10);
      }
      OK(acesos > 20 && acesos < 700, "icone %s desenha traco (%d amostras acesas)", n[i], acesos);
    } }

  // ---- 5. copia parada recuada e faixa deslocada
  if (gfx_snap_ok()) {
    gfx_snap_comecar();
    limpar(0.2f);
    gfx_cor((GfxRect){ 0, 0, 960, 1080 }, 0, 1, 0, 0, 1);        // metade esquerda vermelha
    gfx_cor((GfxRect){ 900, 500, 120, 80 }, 0, 0, 0, 1, 1);      // marca azul no meio
    gfx_snap_terminar();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, 1920, 1080);
    limpar(0); gfx_novo_quadro();
    // Estante: a Home sobe 60 px, encolhe para 0,96 em torno do topo-centro e apaga 65 %.
    gfx_snap_desenhar_xf(0.96f, 960, 0, 0, -60, 0.65f);
    { int r, g, b;
      // a marca azul (960, 540) vai para (960, 0 + 540*0,96 - 60) = (960, 458)
      px(960, 458, &r, &g, &b);
      OK(b > 70 && b < 110 && r < 10, "recuo: a marca vai para (960, 458) e o veu de 65 %% fica (azul %d)", b);
      px(960, 540, &r, &g, &b);
      OK(b < 40, "recuo: a marca saiu do lugar antigo (azul %d)", b);
      // borda de baixo recuada: y = 1080*0,96 - 60 = 977; abaixo e o fundo (preto)
      px(500, 1000, &r, &g, &b);
      OK(r + g + b < 6, "recuo: abaixo da copia recuada sobra o fundo (%d,%d,%d)", r, g, b); }
    // Desdobrar: a faixa da direita anda 470 px; a esquerda fica.
    limpar(0); gfx_novo_quadro();
    gfx_snap_desenhar_faixa((GfxRect){ 0, 0, 1000, 1080 }, 0, 0);
    gfx_snap_desenhar_faixa((GfxRect){ 1000, 0, 920, 1080 }, 470, 0);
    { int r, g, b;
      px(1010, 540, &r, &g, &b); OK(r + g + b < 6, "faixa: o vao de 470 px abre a direita (%d,%d,%d)", r, g, b);
      px(1480, 540, &r, &g, &b); OK(b > 200, "faixa: o pedaco da marca anda 470 px (azul %d)", b);
      px(500, 540, &r, &g, &b); OK(r > 200, "faixa: a esquerda fica no lugar (vermelho %d)", r); }
  } else { printf("FALHOU: sem snapshot\n"); falhas++; }

  // quadro para olhar: Grafite e Preto puro lado a lado, com texto e icone
  for (int tema = 0; tema < 2; tema++) {
    gfx_tema_preto = tema;
    if (!tema) { limpar(0.18f); gfx_novo_quadro(); }
    for (int k = 0; k < 6; k++) {
      GfxRect p = { 140 + tema * 900, 160, 740, 700 };
      if (k) { limpar(0.18f); gfx_novo_quadro(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(16); gfx_tema_preto = 0; r8_material((GfxRect){ 140, 160, 740, 700 }, 22, 1); gfx_tema_preto = 1; r8_material((GfxRect){ 1040, 160, 740, 700 }, 22, 1); }
      for (int t2 = 0; t2 < 2; t2++) {
        GfxRect q = { 140 + t2 * 900, 160, 740, 700 };
        TxtLinha l = txt_linha(TXT_R8_40X, t2 ? "Preto puro" : "Grafite", 244, 242, 238, 255);
        txt_desenhar(l, q.x + 36, q.y + 40);
        l = txt_linha(TXT_R8_19M, "Material, fio e sombra das pranchas", 180, 177, 171, 255);
        txt_desenhar(l, q.x + 36, q.y + 100);
        r8_foco_acao((GfxRect){ q.x + 16, q.y + 160, q.w - 32, 60 }, 16, 1);
        gfx_icone((GfxRect){ q.x + 34, q.y + 177, 26, 26 }, "r8_bookmark", R8_OSSO, 1);
        l = txt_linha(TXT_R8_21S, "Salvos", 244, 242, 238, 255);
        txt_desenhar(l, q.x + 74, q.y + 176);
        r8_foco_conteudo((GfxRect){ q.x + 36, q.y + 260, 384, 216 }, 12, 1);
      }
      (void)p;
      SDL_Delay(15);
    }
  }
  salvar(dir, "r8-fundacao");
  printf("r8_fundacao_shot: %d falhas\n", falhas);
  return falhas ? 1 : 0;
}
