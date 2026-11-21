// MINIATURAS LOCAIS (miniatura.h), com as funcoes reais do tex_cache.
//
// O que a TCL mostrou (20/11/2026): cartaz de 1000x1500 vindo do GitHub para
// ser desenhado a 192 de largura, 300-600 ms de decode por arte, repetido a
// cada abertura do app porque o disco guardava o original. O comportamento
// pedido, conferido aqui:
//   1. o primeiro decode caro grava a miniatura no tamanho do pedido;
//   2. o pedido seguinte vem da miniatura, SEM rede e SEM o original (que e
//      apagado no meio do teste), com o mesmo tamanho, os mesmos pixels (a
//      menos do JPEG) e o tamanho do ORIGINAL preservado para a promocao;
//   3. a miniatura corrompida e apagada e o decode volta ao original, que
//      grava uma nova;
//   4. arte com transparencia volta exata (formato cru);
//   5. GIF e WebP animado nunca viram miniatura; arte que ja e pequena e
//      rapida tambem nao.
#include "../src/sdlcompat.h"
#include <SDL2/SDL_image.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../src/tex_cache.c"
#include <assert.h>
#include <stdio.h>
#include "nvtmp.h"

#define URL "https://raw.githubusercontent.com/exemplo/colecoes/main/pasta/poster-grande.png"
#define URL2 "https://raw.githubusercontent.com/exemplo/colecoes/main/pasta/outra-pasta.png"
static char DIR_[160];

// Cartaz "de verdade" o bastante: degrade suave com textura, opaco, 1000x1500.
static SDL_Surface *cartaz(int w, int h, int comAlfa) {
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
  int x, y;
  assert(s);
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      unsigned char *p = (unsigned char *)s->pixels + (size_t)y * s->pitch + (size_t)x * 4;
      p[0] = (unsigned char)(x * 255 / w);
      p[1] = (unsigned char)(y * 255 / h);
      p[2] = (unsigned char)(((x / 40) + (y / 40)) % 2 ? 200 : 60);
      p[3] = comAlfa ? (unsigned char)((x * 7 + y * 3) % 256) : 255;
    }
  return s;
}

static int achar(const char *c) {
  int i;
  for (i = 0; i < nMax; i++) if (itens[i].estado != VAZIO && !strcmp(itens[i].caminho, c)) return i;
  return -1;
}

static void esperar(int idx, Estado e) {
  int k;
  for (k = 0; k < 2000; k++) {
    int ok;
    SDL_LockMutex(mtx); ok = itens[idx].estado == e && !itens[idx].naFilaDec; SDL_UnlockMutex(mtx);
    if (ok) return;
    SDL_Delay(2);
  }
  fprintf(stderr, "slot %d nao chegou ao estado %d (esta em %d)\n", idx, e, itens[idx].estado);
  assert(0);
}

// Solta o slot como um despejo faria, para o proximo pedido comecar do zero.
static void soltar(int idx) {
  SDL_LockMutex(mtx);
  if (itens[idx].sup) { SDL_FreeSurface(itens[idx].sup); itens[idx].sup = NULL; }
  free(itens[idx].bruto); itens[idx].bruto = NULL;
  memset(&itens[idx], 0, sizeof itens[idx]);
  itens[idx].estado = VAZIO;
  SDL_UnlockMutex(mtx);
}

// Pede a arte como a fileira pede, e devolve o slot ja DECODIFICADO.
// `viaRede`: 1 quando o pedido foi para a fila de rede (o teste entrega a fila
// ao decode, como faria o fio de rede com o arquivo ja no disco).
static int pedir(float larg, int *viaRede) {
  int b;
  quadroAtual++;
  assert(tex_obter_larg(URL, larg) == 0);
  b = achar(URL); assert(b >= 0);
  SDL_LockMutex(mtx);
  *viaRede = filaIni != filaFim;
  if (*viaRede) {
    assert(tirarFila(fila, &filaIni, filaFim) == b);
    paraDecode(b);
  }
  SDL_UnlockMutex(mtx);
  esperar(b, DECODIFICADO);
  return b;
}

static double diferencaMedia(SDL_Surface *a, SDL_Surface *b) {
  long soma = 0, n = 0;
  int x, y, c;
  assert(a->w == b->w && a->h == b->h);
  for (y = 0; y < a->h; y++)
    for (x = 0; x < a->w; x++) {
      const unsigned char *p = (const unsigned char *)a->pixels + (size_t)y * a->pitch + (size_t)x * 4;
      const unsigned char *q = (const unsigned char *)b->pixels + (size_t)y * b->pitch + (size_t)x * 4;
      for (c = 0; c < 4; c++) { soma += abs((int)p[c] - (int)q[c]); n++; }
    }
  return (double)soma / (double)n;
}

static SDL_Surface *copia(SDL_Surface *s) {
  return SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ABGR8888, 0);
}

static void criterios(void) {
  const unsigned char png[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  const unsigned char jpg[4] = {0xFF, 0xD8, 0xFF, 0xE0};
  const unsigned char gif[6] = {'G', 'I', 'F', '8', '9', 'a'};
  unsigned char webp[32], webpAnim[32];
  memset(webp, 0, sizeof webp); memcpy(webp, "RIFF", 4); memcpy(webp + 8, "WEBPVP8 ", 8);
  memset(webpAnim, 0, sizeof webpAnim); memcpy(webpAnim, "RIFF", 4); memcpy(webpAnim + 8, "WEBPVP8X", 8);
  webpAnim[20] = 0x02;
  assert(mini_vale_gravar(1000, 1500, 192, 288, 20, png, 8));
  assert(mini_vale_gravar(2560, 1440, 320, 180, 20, jpg, 4));
  assert(mini_vale_gravar(3840, 2160, 1280, 720, 20, webp, 32));
  assert(!mini_vale_gravar(1000, 1500, 192, 288, 900, gif, 6));
  assert(!mini_vale_gravar(3840, 2160, 1280, 720, 900, webpAnim, 32));
  webpAnim[20] = 0x10;                       // VP8X com alfa, parado: vale
  assert(mini_vale_gravar(3840, 2160, 1280, 720, 20, webpAnim, 32));
  assert(!mini_vale_gravar(250, 375, 192, 288, 10, png, 8));   // pouco a ganhar
  assert(mini_vale_gravar(250, 375, 192, 288, 80, png, 8));    // mas lento: vale
  assert(!mini_vale_gravar(3840, 2160, 3840, 2160, 900, jpg, 4)); // tela cheia 4K: nao
  puts("ok  criterio: GIF e WebP animado nunca; pequeno e rapido nao; reduzido ou lento sim");
}

static void transparencia(void) {
  char cam[700];
  SDL_Surface *a = cartaz(416, 138, 1), *b;
  MiniInfo mi, lido;
  int corr = 0;
  memset(&mi, 0, sizeof mi);
  mi.srcW = 1200; mi.srcH = 400; mi.limite = 416; memcpy(mi.magica, "\x89PNG", 4);
  mini_nome(DIR_, "https://exemplo/logo.png", 416, cam, sizeof cam);
  assert(mini_gravar(cam, a, &mi) == 416L * 138 * 4 + 32);
  b = mini_ler(cam, 416, &lido, &corr);
  assert(b && !corr && diferencaMedia(a, b) == 0.0);
  assert(lido.srcW == 1200 && lido.srcH == 400 && !memcmp(lido.magica, "\x89PNG", 4));
  SDL_FreeSurface(b);
  // Outro teto com o mesmo nome (colisao): nao serve, mas nao e apagada.
  assert(!mini_ler(cam, 480, &lido, &corr) && !corr && access(cam, F_OK) == 0);
  // Um byte trocado no rodape: invalida, apagada.
  { FILE *f = fopen(cam, "r+b"); assert(f); fseek(f, -10, SEEK_END); fputc('x', f); fclose(f); }
  assert(!mini_ler(cam, 416, &lido, &corr) && corr && access(cam, F_OK) != 0);
  SDL_FreeSurface(a);
  puts("ok  logo com transparencia volta exato; rodape adulterado apaga o arquivo");
}

int main(void) {
  SDL_Thread *dec;
  char orig[600], cam[700];
  SDL_Surface *primeira;
  double msOrig, msMini;
  int b, viaRede, srcW;
  MiniStats st;

  setvbuf(stdout, NULL, _IONBF, 0);
  snprintf(DIR_, sizeof DIR_, "%s/nuvio-miniatura-%d", nvt_dir(), (int)getpid());
  mkdir(DIR_, 0755);
  memset(itens, 0, sizeof itens);
  nMax = 8;
  mtx = SDL_CreateMutex(); cond = SDL_CreateCond();
  condDec = SDL_CreateCond(); condLivre = SDL_CreateCond();
  rodando = 1;
  snprintf(dirCache, sizeof dirCache, "%s", DIR_);
  printf("     codificador JPEG: %s\n", jpeg_rapido_codificador() ? "sim" : "nao (miniatura crua)");

  criterios();
  transparencia();

  { SDL_Surface *s = cartaz(1000, 1500, 0);
    nomeDeCache(URL, orig, sizeof orig);
    assert(IMG_SavePNG(s, orig) == 0);
    SDL_FreeSurface(s); }

  dec = SDL_CreateThread(threadDecode, "tex-dec", NULL);
  assert(dec);
  mini_stats(NULL, 1);

  // 1. Primeiro pedido: decode do original de 1000x1500, grava a miniatura.
  { Uint64 t0 = SDL_GetPerformanceCounter();
    b = pedir(192, &viaRede);
    msOrig = (double)(SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency(); }
  assert(viaRede);
  primeira = copia(itens[b].sup);
  srcW = itens[b].fonteW;
  assert(srcW == 1000 && primeira->w <= itens[b].limite && primeira->w < 1000);
  mini_nome(dirCache, URL, itens[b].limite, cam, sizeof cam);
  mini_stats(&st, 1);
  assert(st.gravadas == 1 && st.lidas == 0 && access(cam, F_OK) == 0);
  { struct stat sb; stat(cam, &sb);
    printf("     original 1000x1500 -> %dx%d em %.1f ms; miniatura %ld bytes\n",
           primeira->w, primeira->h, msOrig, (long)sb.st_size); }
  puts("ok  primeiro decode caro grava a miniatura no tamanho do pedido");

  // 2. O original some do disco (poda). O pedido seguinte nao vai a rede: a
  //    miniatura basta.
  soltar(b);
  assert(unlink(orig) == 0);
  { Uint64 t0 = SDL_GetPerformanceCounter();
    b = pedir(192, &viaRede);
    msMini = (double)(SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency(); }
  assert(!viaRede);
  mini_stats(&st, 1);
  assert(st.lidas == 1 && st.gravadas == 0);
  assert(itens[b].sup->w == primeira->w && itens[b].sup->h == primeira->h);
  assert(itens[b].fonteW == srcW);
  { double d = diferencaMedia(primeira, itens[b].sup);
    printf("     da miniatura em %.1f ms (original: %.1f ms); diferenca media %.2f/255\n", msMini, msOrig, d);
    assert(d < 3.0); }
  assert(msMini < msOrig);
  puts("ok  segundo pedido vem da miniatura, sem rede e sem o original, mesmos pixels e fonteW");

  // 3. Miniatura truncada: apagada, decode volta ao original (que voltou ao
  //    disco), e uma miniatura nova e gravada.
  soltar(b);
  { SDL_Surface *s = cartaz(1000, 1500, 0); assert(IMG_SavePNG(s, orig) == 0); SDL_FreeSurface(s); }
  assert(truncate(cam, 100) == 0);
  b = pedir(192, &viaRede);
  mini_stats(&st, 1);
  assert(st.corrompidas == 1 && st.gravadas == 1);
  assert(itens[b].sup->w == primeira->w && diferencaMedia(primeira, itens[b].sup) == 0.0);
  { MiniInfo mi; int corr = 0; SDL_Surface *s = mini_ler(cam, itens[b].limite, &mi, &corr);
    assert(s && !corr); SDL_FreeSurface(s); }
  mini_stats(NULL, 1);
  puts("ok  miniatura corrompida e apagada, o original decodifica e grava outra valida");

  // 4. Desligada (tex_mini_ligado = 0): nada e lido nem gravado.
  soltar(b);
  unlink(cam);
  tex_mini_ligado = 0;
  b = pedir(192, &viaRede);
  mini_stats(&st, 1);
  assert(st.gravadas == 0 && st.lidas == 0 && access(cam, F_OK) != 0);
  tex_mini_ligado = 1;
  puts("ok  com as miniaturas desligadas o caminho antigo fica intacto");

  // 5. Previa de 128 px (a fileira adiantada) com o arquivo do cartao: o
  //    decode da previa grava TAMBEM a miniatura do cartao, e o pedido do
  //    cartao que vem depois nao decodifica o original de novo.
  soltar(b);
  { SDL_Surface *s = cartaz(1000, 1500, 0); char o2[600];
    nomeDeCache(URL2, o2, sizeof o2); assert(IMG_SavePNG(s, o2) == 0); SDL_FreeSurface(s); }
  orcamento = 64L * 1024 * 1024;
  mini_stats(NULL, 1);
  quadroAtual += 2;
  assert(tex_previa_pedir(URL2, 192) == 1);
  { int p = achar(URL2), limCartao;
    char c128[700], cCartao[700];
    assert(p >= 0);
    limCartao = itens[p].limiteArquivo;
    assert(limCartao > NV_TEX_PREVIA_LARG);
    SDL_LockMutex(mtx);
    if (filaIni != filaFim) { assert(tirarFila(fila, &filaIni, filaFim) == p); paraDecode(p); }
    SDL_UnlockMutex(mtx);
    esperar(p, DECODIFICADO);
    assert(itens[p].sup->w <= NV_TEX_PREVIA_LARG);
    mini_nome(dirCache, URL2, NV_TEX_PREVIA_LARG, c128, sizeof c128);
    mini_nome(dirCache, URL2, limCartao, cCartao, sizeof cCartao);
    mini_stats(&st, 1);
    printf("     previa: %dx%d; miniaturas gravadas=%d (128 e %d)\n", itens[p].sup->w, itens[p].sup->h, st.gravadas, limCartao);
    assert(st.gravadas == 2 && access(c128, F_OK) == 0 && access(cCartao, F_OK) == 0);
    soltar(p);
    // O cartao: da miniatura, sem rede e sem decodificar o PNG.
    { int q; quadroAtual++;
      assert(tex_obter_larg(URL2, 192) == 0);
      q = achar(URL2); assert(q >= 0 && itens[q].limite == limCartao);
      SDL_LockMutex(mtx); assert(filaIni == filaFim); SDL_UnlockMutex(mtx);
      esperar(q, DECODIFICADO);
      mini_stats(&st, 1);
      assert(st.lidas == 1 && st.gravadas == 0 && itens[q].fonteW == 1000);
      assert(itens[q].sup->w == primeira->w);
      soltar(q); }
    { char o2[600]; nomeDeCache(URL2, o2, sizeof o2); unlink(o2); } }
  puts("ok  a previa da fileira adiantada grava a miniatura do cartao; o cartao nao decodifica o original");

  SDL_LockMutex(mtx); rodando = 0; SDL_CondBroadcast(condDec); SDL_UnlockMutex(mtx);
  SDL_WaitThread(dec, NULL);
  SDL_FreeSurface(primeira);
  unlink(orig);
  { char cmd[300]; snprintf(cmd, sizeof cmd, "rm -rf '%s'", DIR_); if (system(cmd)) {} }
  puts("miniatura: tudo ok");
  return 0;
}
