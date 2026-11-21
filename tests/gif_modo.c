// "GIF animado nas Collections" (World 21/11): a TCL do dono (932 MB) nunca animava porque a
// tabela por RAM zera o orcamento em TV de 1 GB; a SEMP (mais RAM) animava o mesmo GIF.
// Roda tres vezes (NV_GIF_MEM_GB = 0,9 / 1,5 / 8) com o gif.c do Android.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "gif.h"
int main(void) {
  double gb = atof(getenv("NV_GIF_MEM_GB"));
  gif_modo(0); int autom = gif_pode_animar();
  gif_modo(1); int ligado = gif_pode_animar();
  gif_modo(2); int deslig = gif_pode_animar();
  gif_modo(7); int fora = gif_pode_animar();   // valor fora da lista le Automatico
  printf("RAM %.1f GB: automatico=%d ligado=%d desligado=%d\n", gb, autom, ligado, deslig);
  assert(autom == (gb > 1.0));   // a regra de sempre
  assert(ligado == 1);           // Ligado anima tambem em TV de 1 GB
  assert(deslig == 0);           // Desligado nunca anima
  assert(fora == autom);
  return 0;
}
