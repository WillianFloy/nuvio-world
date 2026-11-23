// World (23/11): o trailer em TELA CHEIA e desenhado em escala real, mesmo com a escala de texto e componentes != 1.
// Fotos da TCL: com a escala de texto em 0,8, a interface da pausa (veus, barra, tempo, "Continuar") e o furo saiam com
// 80% da tela, presos no canto de cima a esquerda. detail.c real, GLES2 por software; NAO prova nada sobre a TV.
//   1. pausado, escala 0,8: os veus de cima e de baixo chegam aos cantos da direita (antes: alfa 0 ali, sem interface);
//   2. o centro da tela e furo (alfa 0): o video aparece;
//   3. a escala de texto volta ao que era depois do desenho (nada vaza para o resto do quadro).
#include "../src/detail.h"
#include "../src/catalogo.h"
#include "../src/home.h"
#include "../src/gfx.h"
#include "../src/text.h"
#include "../src/trailer.h"
#include "../src/tex_cache.h"
#include "../src/dados.h"
#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static int falhas;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
static void px(int x, int y, unsigned char *o) { glReadPixels(x, 1080 - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, o); }
static void quadro(void) {
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);   // vermelho: onde ninguem desenhou
  tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  detail_atualizar(0.016f, SDL_GetTicks()); detail_desenhar(SDL_GetTicks()); glFinish(); }
int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_Window *w = SDL_CreateWindow("t", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w && SDL_GL_CreateContext(w));
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1)); dados_iniciar("deploy/app/art"); tex_iniciar(32);
  CatItem v; memset(&v, 0, sizeof v);
  snprintf(v.imdb, sizeof v.imdb, "tt0000001"); snprintf(v.titulo, sizeof v.titulo, "Como Magica"); snprintf(v.tipo, sizeof v.tipo, "movie");
  cat_definir_tudo(&v, 1, NULL, 0);
  HomeItem hi; memset(&hi, 0, sizeof hi); hi.indice = 0;
  detail_abrir(&hi);
  for (int i = 0; i < 90; i++) quadro();
  gfx_escala_txt_definir(0.8f);
  trailer_shot_pausado(1);
  float antes = gfx_escala();
  quadro();
  unsigned char c[4], m[4], e[4];
  px(1900, 1070, c);   // canto inferior direito: dentro do veu de baixo
  px(960, 540, m);     // centro: furo
  px(1900, 20, e);     // canto superior direito: dentro do veu de cima
  printf("canto inf. dir. rgba=%d,%d,%d,%d | centro rgba=%d,%d,%d,%d | canto sup. dir. rgba=%d,%d,%d,%d\n",
         c[0], c[1], c[2], c[3], m[0], m[1], m[2], m[3], e[0], e[1], e[2], e[3]);
  ok(c[3] > 40, "escala 0,8: o veu de baixo cobre o canto inferior direito (antes: alfa 0, sem interface)");
  ok(e[3] > 40, "escala 0,8: o veu de cima cobre o canto superior direito");
  ok(m[3] < 10, "o centro e furo: o video aparece");
  ok(gfx_escala() == antes, "a escala ativa volta ao que era depois do desenho");
  trailer_shot_pausado(0);
  printf("%s\n", falhas ? "trailer_cheia_escala: FALHOU" : "trailer_cheia_escala: ok");
  return falhas != 0;
}
