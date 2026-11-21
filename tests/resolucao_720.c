// AUTOMATICA DESCE PARA 720p SO QUANDO A TV NAO SEGURA 1080p (src/resolucao.h, World 21/11).
// As series sao os FPS= reais (relatorios de 3 s) dos logs de 21/11:
//   - SEMP 50SK8300 (Android 9, superficie 1920x1080): 15 fps de media, mesmo apos os efeitos no minimo;
//   - TCL do dono (superficie 1280x720, lisa): 30-50 fps, com quedas isoladas abaixo de 32.
// "N2" no log = gpu-nivel chegou ao fim da escada de efeitos; antes disso quem cede sao os efeitos.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "resolucao.h"

static const double SEMP_ANTES[] = { 8.9, 15.0, 14.7, 15.1, 16.3, 14.1, 18.7, 14.1, 13.6, 12.6, 13.8, 11.3 };
static const double SEMP_DEPOIS[] = { 14.8, 35.8, 21.9, 30.3, 24.9, 12.8, 13.5, 19.6, 14.3, 29.6, 33.0, 15.1 };
static const double TCL[] = { 36.6, 31.7, 48.8, 31.6, 36.1, 42.2, 30.6, 36.0, 38.2, 37.3, 33.9, 49.3, 51.5,
                              31.2, 33.3, 37.4, 34.4, 41.2, 39.2, 41.8, 38.2, 35.9, 34.5, 34.5, 33.9, 32.4, 33.9 };
#define N(a) (int)(sizeof a / sizeof *a)

int main(void) {
  Res720 v; int i, quando = -1;
  // 1. Efeitos ainda descendo: os pixels nao cedem, por pior que esteja.
  memset(&v, 0, sizeof v);
  for (i = 0; i < N(SEMP_ANTES); i++) assert(!res_720_amostra(&v, SEMP_ANTES[i], 0.0, 0, 1));
  // 2. SEMP com os efeitos no minimo: desce, e uma vez so. 35,8 no segundo relatorio zera a sequencia.
  for (i = 0; i < N(SEMP_DEPOIS); i++)
    if (res_720_amostra(&v, SEMP_DEPOIS[i], 0.0, 1, 1)) { assert(quando < 0); quando = i; }
  printf("SEMP: desce no relatorio %d depois dos efeitos no minimo (~%d s)\n", quando + 1, (quando + 1) * 3);
  assert(quando >= RES_720_SEGUIDAS - 1 && quando <= 8);
  // 3. TCL (lisa): quedas isoladas abaixo de 32 nao bastam.
  memset(&v, 0, sizeof v);
  for (i = 0; i < N(TCL); i++) assert(!res_720_amostra(&v, TCL[i], 0.0, 1, 1));
  // 4. Amostra invalida (player, abertura, aquecimento) nao conta nem zera.
  memset(&v, 0, sizeof v);
  assert(!res_720_amostra(&v, 12, 0, 1, 1)); assert(!res_720_amostra(&v, 12, 0, 1, 1));
  assert(!res_720_amostra(&v, 60, 0, 1, 0));
  assert(!res_720_amostra(&v, 12, 0, 1, 1)); assert(res_720_amostra(&v, 12, 0, 1, 1));
  // 5. Com relogio da GPU, ele decide (fps baixo por outro motivo nao derruba a resolucao).
  memset(&v, 0, sizeof v);
  for (i = 0; i < 10; i++) assert(!res_720_amostra(&v, 12, 15.0, 1, 1));
  for (i = 0; i < RES_720_SEGUIDAS - 1; i++) assert(!res_720_amostra(&v, 50, 30.0, 1, 1));
  assert(res_720_amostra(&v, 50, 30.0, 1, 1));
  // 6. O veredito gravado vale para esta versao; outra versao mede de novo.
  assert(res_720_ler("720 2.0.45\n", "2.0.45"));
  assert(!res_720_ler("720 2.0.45\n", "2.0.46"));
  assert(!res_720_ler("720 2.0.4\n", "2.0.45"));
  assert(!res_720_ler(NULL, "2.0.45") && !res_720_ler("1080 2.0.45\n", "2.0.45"));
  puts("resolucao_720: ok");
  return 0;
}
