// A barrinha "Segure para opcoes" nao aparece no TOQUE, so no segurar.
// Simula o que a Home e a Biblioteca fazem a cada quadro: KEYDOWN em t0, a tela
// pergunta se desenha a barra com (agora - t0); o toque solta em ~80-140 ms.
#include "segurar.h"
#include "layout.h"
#include <assert.h>
#include <stdio.h>

// quadros (16 ms) em que a barra apareceria numa pressao de `durMs`
static int quadros_com_barra(unsigned durMs) {
  int n = 0;
  for (unsigned t = 0; t < durMs; t += 16) if (segurar_barra_visivel(t)) n++;
  return n;
}
int main(void) {
  // toque: de 60 a 140 ms (o mais lento de um dedo) NUNCA desenha a barra
  for (unsigned d = 30; d <= 140; d += 10) assert(quadros_com_barra(d) == 0);
  // segurar de verdade: a barra aparece e fica ate o menu abrir
  assert(quadros_com_barra(NV_HOLD_MS) > 20);
  assert(!segurar_barra_visivel(0) && !segurar_barra_visivel(149) && segurar_barra_visivel(150));
  puts("segurar_barra: PASS");
  return 0;
}
