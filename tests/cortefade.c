// Curva do esvanecer da linha cortada (sinopse da Vitrine). Sem GL.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/cortefade.h"
int main(void) {
  float ant = 2.0f;
  for (int i = 0; i <= 1000; i++) {
    float t = i / 1000.0f, a = cortefade_alfa(t);
    assert(a >= 0.0f && a <= 1.0f);
    assert(a <= ant + 1e-6f);               // nunca reaparece mais embaixo
    ant = a;
  }
  assert(cortefade_alfa(0.0f) == 1.0f && cortefade_alfa(0.10f) == 1.0f);   // topo inteiro
  assert(cortefade_alfa(0.95f) == 0.0f && cortefade_alfa(1.0f) == 0.0f);   // base some
  float meio = cortefade_alfa(0.5f);
  assert(meio > 0.35f && meio < 0.65f);                                      // glifo do meio ainda le
  assert(cortefade_alfa(0.2f) > 0.9f);                                       // discreto no comeco
  // faixas: cobrem a linha sem falha nem sobreposicao
  for (int h = 10; h <= 80; h++) {
    float cob = 0.0f;
    for (float t0 = 0.0f; t0 < h; t0 += CF_FAIXA_PX) cob += fminf(CF_FAIXA_PX, h - t0);
    assert(fabsf(cob - h) < 1e-3f);
  }
  puts("cortefade: tudo ok");
  return 0;
}
