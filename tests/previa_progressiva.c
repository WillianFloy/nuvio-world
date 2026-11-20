// CARREGAMENTO PROGRESSIVO DE POSTERES (18/11), pelo caminho real do tex_cache
// (pedido -> fio de decode -> bombear -> upload), com cartazes locais 288x432:
//   1. tex_previa_pedir decodifica a PREVIA a 128 px (o proprio arquivo, uma vez)
//      e marca o item como previa; ocupa ~1/5 dos bytes do cartao inteiro.
//   2. O pedido do cartao (tex_obter_larg 288) PROMOVE e, enquanto a versao
//      grande nao sobe, CONTINUA devolvendo a textura da previa (nunca 0): o
//      cartaz nao pisca vazio na troca.
//   3. Pronta a versao do cartao, ela SUBSTITUI a previa no mesmo item (mesmo
//      aspecto, previa liberada): uma versao so na memoria.
//   4. Heroi (pedido > NV_TEX_LARG_MAX) nao aceita a previa: segue a regra antiga.
//   5. Previa com pressao de reproducao: nao pede.
#include "../src/sdlcompat.h"
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include "../src/tex_cache.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int achar(const char *nome) {
  int i; SDL_LockMutex(mtx); i = acharIndice(nome, hashCaminho(nome)); SDL_UnlockMutex(mtx); return i;
}
static Estado estado(const char *n) { int i = achar(n); Estado e = VAZIO; if (i >= 0) { SDL_LockMutex(mtx); e = itens[i].estado; SDL_UnlockMutex(mtx); } return e; }
static void dims(const char *n, int *w, int *h) { int i = achar(n); SDL_LockMutex(mtx); *w = itens[i].w; *h = itens[i].h; SDL_UnlockMutex(mtx); }
static void bomba(int q) { for (int t = 0; t < q; t++) { tex_novo_quadro(); tex_bombear(8); SDL_Delay(5); } }
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_Window *w = SDL_CreateWindow("previa", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w); assert(SDL_GL_CreateContext(w));
  assert(tex_iniciar(64) == 1);
  tex_escala(1.0f);
  const char *A = "deploy/app/art/poster/08.jpg", *B = "deploy/app/art/poster/09.jpg", *H = "deploy/app/art/poster/10.jpg";
  // 1. previa
  int pediu = 0;
  for (int t = 0; t < 200 && !pediu; t++) { pediu = tex_previa_pedir(A, 288.0f); bomba(1); }
  OK(pediu, "tex_previa_pedir aceitou o pedido com o cache ocioso");
  for (int t = 0; t < 600 && estado(A) != PRONTO; t++) bomba(1);
  int pw, ph; dims(A, &pw, &ph);
  printf("  previa: %dx%d\n", pw, ph);
  OK(estado(A) == PRONTO && pw == 128 && ph == 192, "previa decodificada a 128x192 (do arquivo de 288x432)");
  OK(tex_previa_em_uso(A), "item marcado como previa");
  long bPrev; bPrev = (long)pw * ph * 4;
  // 2. promocao: nunca 0 enquanto a grande nao sobe
  GLuint texPrev = 0; { int i = achar(A); SDL_LockMutex(mtx); texPrev = itens[i].tex; SDL_UnlockMutex(mtx); }
  int zeros = 0, quadros = 0, viuPrev = 0;
  for (int t = 0; t < 600; t++) {
    GLuint got = tex_obter_larg(A, 288.0f);
    quadros++;
    if (!got) zeros++;
    if (got == texPrev) viuPrev = 1;
    int cw, ch; dims(A, &cw, &ch);
    if (estado(A) == PRONTO && cw > 128) break;
    bomba(1);
  }
  int fw, fh; dims(A, &fw, &fh);
  printf("  cartao: %dx%d em %d quadros, %d quadros sem textura\n", fw, fh, quadros, zeros);
  OK(zeros == 0, "durante a promocao o pedido do cartao devolve a previa (nunca 0: sem piscar vazio)");
  OK(viuPrev || quadros == 1, "a previa foi a textura mostrada ate a versao do cartao subir");
  OK(fw == 288 && fh == 432, "versao do cartao 288x432 publicada no mesmo item");
  OK(!tex_previa_em_uso(A), "a previa foi substituida (uma versao so)");
  OK(fabsf((float)fw / fh - (float)pw / ph) < 0.01f, "mesmo aspecto: o enquadramento nao muda na troca");
  long bFull; bFull = (long)fw * fh * 4;
  printf("  bytes previa %ld, cartao %ld\n", bPrev, bFull);
  OK(bPrev > 0 && bPrev * 4 < bFull, "a previa custa menos de 1/4 da memoria do cartao");
  // 4. heroi nao aceita a previa
  for (int t = 0; t < 200 && !tex_previa_pedir(B, 288.0f); t++) bomba(1);
  for (int t = 0; t < 600 && estado(B) != PRONTO; t++) bomba(1);
  OK(tex_previa_em_uso(B), "segunda previa pronta");
  GLuint heroi = tex_obter_larg(B, 1200.0f);
  OK(heroi == 0, "pedido de heroi (tela cheia) nao recebe a previa borrada");
  // 5. reproducao: sem previas
  tex_pressao_reproducao(1);
  int pediuPlay = 0;
  for (int t = 0; t < 20; t++) { pediuPlay |= tex_previa_pedir(H, 288.0f); bomba(1); }
  tex_pressao_reproducao(0);
  OK(!pediuPlay, "com o player aberto nenhuma previa e pedida");
  if (falhas) return 1;
  puts("previa_progressiva: ok");
  return 0;
}
