// CAPTURA DO SALVOS (CH+) COM MENU DO TOPO: a Home real por baixo, a folha vazia
// e com itens, para conferir posicao, cantos e material contra a prancha 3A.
//   bash tests/salvos_world_shot.sh /tmp/nv-salvos
// NAO ENTRA NA SUITE (*_shot): precisa de janela GL e de olho humano.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *janela;
static const char *dirDados;
static Uint32 relogio = 1000;

static void gravarBmp(const char *bmp) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && s);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(s, bmp) == 0);
  SDL_FreeSurface(s);
  free(pix);
  printf("captura: %s\n", bmp);
}
static void quadros(int n, const char *bmp) {
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
    if (bmp && i == n - 1) gravarBmp(bmp);
    SDL_GL_SwapWindow(janela);
  }
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-salvos/s";
  static CatItem itens[40];
  static CatFileira fils[3];
  char cam[800], bmp[800];
  FILE *a;
  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  assert(dirDados && strstr(dirDados, "nuvio-salvos-shot"));
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: salvos", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(janela && SDL_GL_CreateContext(janela));
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  snprintf(cam, sizeof cam, "%s/cache", dirDados);
  tex_cache_dir(cam);
  gfx_icones_dir("deploy/app/art");
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w"); assert(a);
  fprintf(a, "idioma 0\nhomeLayoutLocal 1\nmodernSidebar 1\nmenuTopoLocal %d\nselected_theme 9\n",
          getenv("NV_SEM_TOPO") ? 0 : 1);
  fclose(a);
  ajustes_dir(dirDados);
  assert(home_iniciar("deploy/app/art"));
  memset(itens, 0, sizeof itens); memset(fils, 0, sizeof fils);
  for (int i = 0; i < 20; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt90%05d", i);
    snprintf(c->tipo, sizeof c->tipo, i % 3 ? "movie" : "series");
    snprintf(c->titulo, sizeof c->titulo, "Titulo %d", i);
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
  quadros(40, NULL);
  snprintf(bmp, sizeof bmp, "%s-0-home.bmp", saida);
  quadros(1, bmp);
  spainel_abrir();
  quadros(110, NULL);
  snprintf(bmp, sizeof bmp, "%s-1-vazio.bmp", saida);
  quadros(1, bmp);
  spainel_fechar();
  quadros(60, NULL);
  for (int i = 8; i < 14; i++) salvos_definir(&itens[i], 1);
  spainel_abrir();
  quadros(110, NULL);
  snprintf(bmp, sizeof bmp, "%s-2-itens.bmp", saida);
  quadros(1, bmp);
  return 0;
}
