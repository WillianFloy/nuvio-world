#!/bin/bash
# PRETO PURO NAO PISCA GRAFITE (video da TCL, 18/11): toda troca de titulo ou
# de pasta reinicia a transicao de cor do corviva; o fundo era interpolado ate
# o FUNDO fixo (grafite 0,051) e so voltava a 0 no quadro seguinte. Aqui, com o
# corviva.c real, troca-se de arte quadro a quadro (como passar por posteres e
# pastas) e o fundo tem de ficar em 0 em TODOS os quadros; com o Grafite, o
# fundo fica no grafite; trocar o tema leva ao novo fundo e la fica.
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-tema-preto-fundo"
cat > "$out.c" <<'C'
#include "corviva.h"
#include "layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int tema;
int ajustes_tema_preto(void) { return tema; }   // forte: vence o fraco do corviva.c
char *dados_ler(const char *n) { (void)n; return NULL; }
int dados_gravar_leve(const char *n, const char *b) { (void)n; (void)b; return 0; }
static unsigned char img[180 * 320 * 4];
static void pinta(int r, int g, int b) {
  for (int i = 0; i < 320 * 180; i++) { img[i*4] = r; img[i*4+1] = g; img[i*4+2] = b; img[i*4+3] = 255; }
}
static int falhas;
static void confere(float alvo, const char *onde, int q) {
  for (int k = 0; k < 3; k++)
    if (nv_cor_fundo_viva[k] < alvo - 0.002f || nv_cor_fundo_viva[k] > alvo + 0.002f) {
      printf("FALHOU %s quadro %d: fundo %.4f (alvo %.3f)\n", onde, q, nv_cor_fundo_viva[k], alvo); falhas++; return;
    }
}
int main(void) {
  CorvivaPaleta azul, laranja, verde;
  pinta(30, 80, 180);  if (corviva_extrair(img, 320, 180, 320 * 4, &azul) != 1) return 2;
  pinta(220, 120, 30); if (corviva_extrair(img, 320, 180, 320 * 4, &laranja) != 1) return 2;
  pinta(40, 170, 90);  if (corviva_extrair(img, 320, 180, 320 * 4, &verde) != 1) return 2;
  corviva_anotar("a", &azul); corviva_anotar("b", &laranja); corviva_anotar("c", &verde);
  const char *seq[] = { "a", "b", "c", "b", "a", NULL, "c" };   // NULL: sem titulo (pasta sem arte)
  int modos[] = { CORVIVA_SIMPLES, CORVIVA_IMERSIVA, CORVIVA_GRADIENTE };
  for (int m = 0; m < 3; m++) {
    tema = 1;
    int q = 0;
    for (int volta = 0; volta < 3; volta++)
      for (int i = 0; i < 7; i++)
        for (int f = 0; f < 9; f++, q++) {   // ~150 ms por arte: troca no meio da transicao
          if (seq[i]) corviva_definir(seq[i], CORVIVA_HOME);
          corviva_quadro(1 / 60.0f, modos[m], 1, 0);
          if (q > 0) confere(0.0f, "Preto puro", q);
        }
    // Volta ao Grafite: chega ao grafite e fica nele trocando de arte.
    tema = 0;
    for (int f = 0; f < 120; f++) { corviva_definir(seq[f % 5], CORVIVA_HOME); corviva_quadro(1 / 60.0f, modos[m], 1, 0); }
    for (int f = 0; f < 60; f++) { corviva_definir(seq[(f / 9) % 5], CORVIVA_HOME); corviva_quadro(1 / 60.0f, modos[m], 1, 0); confere(0.051f, "Grafite", f); }
    // E de novo ao Preto: depois da transicao do tema, 0 em todo quadro.
    tema = 1;
    for (int f = 0; f < 60; f++) corviva_quadro(1 / 60.0f, modos[m], 1, 0);
    for (int f = 0; f < 90; f++) { corviva_definir(seq[(f / 9) % 5], CORVIVA_HOME); corviva_quadro(1 / 60.0f, modos[m], 1, 0); confere(0.0f, "Preto puro (de novo)", f); }
  }
  if (falhas) return 1;
  puts("ok: Preto puro fica em #000 em toda troca de arte (3 modos de cor); Grafite fica no grafite");
  return 0;
}
C
cc -Wall -Wextra -O2 -Isrc "$out.c" src/corviva.c -o "$out" -lm
"$out"
