// AS TRES APRESENTACOES DO CH+ (World, 24/11): Estante (padrao), Coluna e
// Classico, com a Home real por baixo. Captura e confere COMPORTAMENTO:
//   - Estante: banner no alto que segue o foco (a arte muda ao andar), cartoes
//     lado a lado na mesma fileira, a fileira rola para o cartao caber, o
//     banner nasce do retangulo de origem e cresce ate a tela, sem a pilula
//     Estilo na barra;
//   - Coluna: a linha em foco se abre (mais alta que as outras) e a de baixo
//     desce; ao andar, a anterior fecha;
//   - Classico: a barra continua com a pilula Estilo (4) e a lista de sempre.
//   bash tests/salvos_estilos_shot.sh /tmp/nv-salvos-estilos
#include "ajustes.h"
#include "catalogo.h"
#include "dados.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "menu.h"
#include "salvos.h"
#include "salvospainel.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *janela;
static const char *dirDados;
static Uint32 relogio = 1000;
static int falhas;
static unsigned char *pix;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "ok" : "FALHA", m); if (!c) falhas++; }

static void ler(void) { glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix); }
static void gravarBmp(const char *bmp) {
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  for (int y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  SDL_SaveBMP(s, bmp);
  SDL_FreeSurface(s);
  printf("captura: %s\n", bmp);
}
// Media e variancia de luminancia num retangulo (y de cima para baixo).
static void lum(int x0, int y0, int w, int h, double *m, double *v) {
  double s = 0, s2 = 0; long n = 0;
  for (int y = y0; y < y0 + h; y += 2)
    for (int x = x0; x < x0 + w; x += 2) {
      const unsigned char *p = pix + ((1079 - y) * 1920 + x) * 4;
      double l = 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2];
      s += l; s2 += l * l; n++;
    }
  *m = s / n; *v = s2 / n - (*m) * (*m);
}
static void quadros(int n, int capturar) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, relogio);
    spainel_atualizar(1.0f / 60.0f, relogio);
    txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    home_desenhar(relogio);
    menu_desenhar(relogio);
    if (spainel_visivel()) spainel_desenhar(relogio);
    if (capturar && i == n - 1) ler();
    SDL_GL_SwapWindow(janela);
  }
}
static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  spainel_evento(&e);
  e.type = SDL_KEYUP; spainel_evento(&e);
}
static void ajustes(int estilo) {
  char cam[800];
  FILE *a;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w"); assert(a);
  fprintf(a, "idioma 0\nhomeLayoutLocal 1\nmodernSidebar 1\nmenuTopoLocal 1\nselected_theme 9\nsalvosEstiloLocal %d\n", estilo);
  fclose(a);
  ajustes_dir(dirDados);
}
// Foco na primeira linha da aba Salvos.
static void irSalvos(void) {
  spainel_ir_aba(0);
  quadros(30, 0);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-salvos-estilos/s";
  static CatItem itens[40];
  static CatFileira fils[3];
  char cam[800], bmp[900];
  float x, y, w, h, x2, y2, w2, h2;
  double m, v, m2, v2;
  pix = malloc(1920 * 1080 * 4);
  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#ifndef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
  janela = SDL_CreateWindow("Nuvio: salvos estilos", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!janela || !SDL_GL_CreateContext(janela)) { fprintf(stderr, "sem GL\n"); return 1; }
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  if (!gfx_iniciar() || !txt_iniciar("deploy/app", 1)) { fprintf(stderr, "sem gfx\n"); return 1; }
  tex_iniciar(192);
  snprintf(cam, sizeof cam, "%s/cache", dirDados);
  tex_cache_dir(cam);
  gfx_icones_dir("deploy/app/art");
  ajustes(0);
  if (!home_iniciar("deploy/app/art")) { fprintf(stderr, "sem home\n"); return 1; }
  memset(itens, 0, sizeof itens); memset(fils, 0, sizeof fils);
  for (int i = 0; i < 20; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt90%05d", i);
    snprintf(c->tipo, sizeof c->tipo, i % 3 ? "movie" : "series");
    snprintf(c->titulo, sizeof c->titulo, "Titulo numero %d", i);
    snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", 20 + i);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", 20 + i);
    if (i < 6) { c->progresso = 30 + i * 8; c->restanteMin = 40; }
  }
  snprintf(fils[0].chave, sizeof fils[0].chave, "continue_watching");
  snprintf(fils[0].titulo, sizeof fils[0].titulo, "Continuar assistindo");
  snprintf(fils[0].tipo, sizeof fils[0].tipo, "movie");
  fils[0].ini = 0; fils[0].n = 6;
  snprintf(fils[1].chave, sizeof fils[1].chave, "pop_movie");
  snprintf(fils[1].titulo, sizeof fils[1].titulo, "Popular - Filme");
  snprintf(fils[1].tipo, sizeof fils[1].tipo, "movie");
  fils[1].ini = 6; fils[1].n = 14;
  cat_definir_tudo(itens, 20, fils, 2);
  for (int i = 0; i < 14; i++) salvos_definir(&itens[i], 1);
  quadros(40, 0);

  // ---------------------------------------------------------------- ESTANTE
  ok(ajustes_salvos_estilo() == 0, "Estante e o padrao");
  spainel_abrir_de(1700, 40, 120, 52);
  quadros(4, 1);
  ok(spainel_teste_modo() == 0, "abriu na Estante");
  ok(spainel_teste_banner(&x, &y, &w, &h) && w < 1700 && x > 100, "no comeco o banner ainda esta perto da origem");
  snprintf(bmp, sizeof bmp, "%s-e0-abrindo.bmp", saida); gravarBmp(bmp);
  quadros(80, 1);
  ok(spainel_teste_banner(&x, &y, &w, &h) && w >= 1919 && h >= 1079, "depois o banner cobre a tela");
  irSalvos();
  quadros(30, 1);
  snprintf(bmp, sizeof bmp, "%s-e1-aberto.bmp", saida); gravarBmp(bmp);
  ok(spainel_aba_atual() == 0 && spainel_foco_indice() == 0, "foco no primeiro salvo");
  lum(96, 60, 900, 300, &m, &v);
  ok(v > 60.0, "o alto da tela e a arte do titulo (tem variancia)");
  ok(spainel_teste_n_chips() == 3, "sem a pilula Estilo na barra");
  ok(spainel_teste_linha(0, &x, &y, &w, &h) && spainel_teste_linha(1, &x2, &y2, &w2, &h2) &&
     fabsf(y - y2) < 1.0f && x2 > x + w, "cartoes lado a lado na mesma fileira");
  ok(y > 452.0f, "os cartoes moram na folha de baixo");
  lum(96, 60, 900, 300, &m, &v);
  tecla(SDLK_RIGHT); quadros(40, 1);
  ok(spainel_foco_indice() == 1, "DIREITA anda na fileira");
  lum(96, 60, 900, 300, &m2, &v2);
  ok(fabs(m - m2) > 1.5 || fabs(v - v2) > 30.0, "o banner trocou de arte com o foco");
  snprintf(bmp, sizeof bmp, "%s-e2-direita.bmp", saida); gravarBmp(bmp);
  for (int k = 0; k < 4; k++) { tecla(SDLK_RIGHT); quadros(16, 0); }
  quadros(40, 1);
  ok(spainel_foco_indice() == 5, "anda ate o sexto cartao");
  ok(spainel_teste_linha(5, &x, &y, &w, &h) && x + w <= 1920.0f / gfx_escala_ui() - 96 + 1 && x >= 48, "a fileira rolou e o cartao em foco cabe");
  snprintf(bmp, sizeof bmp, "%s-e3-rolou.bmp", saida); gravarBmp(bmp);
  tecla(SDLK_DOWN); quadros(50, 1);
  ok(spainel_foco_indice() >= 6, "BAIXO vai para a fileira de baixo");
  ok(spainel_teste_linha(spainel_foco_indice(), &x2, &y2, &w2, &h2) && y2 > y, "e ela esta abaixo");
  snprintf(bmp, sizeof bmp, "%s-e4-baixo.bmp", saida); gravarBmp(bmp);
  spainel_fechar(); quadros(80, 0);
  ok(!spainel_visivel(), "Estante fecha");

  // ---------------------------------------------------------------- COLUNA
  ajustes(1);
  spainel_abrir_de(1700, 40, 120, 52);
  quadros(6, 1);
  snprintf(bmp, sizeof bmp, "%s-c0-abrindo.bmp", saida); gravarBmp(bmp);
  quadros(80, 0);
  irSalvos();
  quadros(40, 1);
  ok(spainel_teste_modo() == 1, "abriu na Coluna");
  ok(spainel_teste_n_chips() == 3, "Coluna tambem sem a pilula Estilo");
  snprintf(bmp, sizeof bmp, "%s-c1-aberto.bmp", saida); gravarBmp(bmp);
  ok(spainel_teste_linha(0, &x, &y, &w, &h) && spainel_teste_linha(1, &x2, &y2, &w2, &h2) &&
     h > h2 + 200.0f && y2 >= y + h - 1.0f, "a linha em foco esta aberta e a de baixo desceu");
  ok(x > 1000.0f, "a coluna fica a direita");
  tecla(SDLK_DOWN); quadros(50, 1);
  ok(spainel_foco_indice() == 1, "BAIXO anda uma linha");
  ok(spainel_teste_linha(0, &x, &y, &w, &h) && spainel_teste_linha(1, &x2, &y2, &w2, &h2) &&
     h2 > h + 200.0f, "a nova abriu e a anterior fechou");
  snprintf(bmp, sizeof bmp, "%s-c2-baixo.bmp", saida); gravarBmp(bmp);
  spainel_fechar(); quadros(80, 0);

  // ---------------------------------------------------------------- CLASSICO
  ajustes(2);
  spainel_abrir();
  quadros(80, 0);
  irSalvos();
  quadros(30, 1);
  ok(spainel_teste_modo() == 2, "abriu no Classico");
  ok(spainel_teste_n_chips() == 4, "Classico mantem a pilula Estilo");
  ok(spainel_teste_linha(0, &x, &y, &w, &h) && spainel_teste_linha(1, &x2, &y2, &w2, &h2) &&
     fabsf(h - h2) < 1.0f, "Classico: linhas da mesma altura (a lista de sempre)");
  snprintf(bmp, sizeof bmp, "%s-k1-classico.bmp", saida); gravarBmp(bmp);
  spainel_fechar(); quadros(60, 0);
  printf("%s\n", falhas ? "salvos_estilos: FALHOU" : "salvos_estilos: ok");
  return falhas != 0;
}
