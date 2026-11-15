// Ritmo da navegacao por setas (src/navritmo.h) contra fluxos simulados.
#include <assert.h>
#include <stdio.h>
#include "../src/navritmo.h"

#define L 1001   // SDLK_LEFT, qualquer id
#define R 1002

// Segura `tecla` por `dur` ms com repeticao a cada `cad` ms; devolve entregues.
static int segurar(NavRitmo *n, int tecla, unsigned t0, unsigned dur, unsigned cad, unsigned *ult) {
  int ok = 0; unsigned t;
  for (t = t0; t <= t0 + dur; t += cad) if (navritmo_baixou(n, t, tecla)) { ok++; *ult = t; }
  return ok;
}
int main(void) {
  NavRitmo n = {0}; unsigned u = 0;
  // 1. Um toque: entrega exatamente um.
  assert(navritmo_baixou(&n, 1000, R) == 1);
  navritmo_soltou(&n, R);
  // 2. Segurar 2 s a 100 ms (C9) e a 40 ms: o mesmo ritmo, bem abaixo de 1 por repeticao.
  for (int k = 0; k < 2; k++) {
    unsigned cad = k ? 40 : 100;
    NavRitmo m = {0};
    int ok = segurar(&m, R, 5000, 2000, cad, &u);
    // 1 (toque) + passos de ~320 ms depois da espera de 350 ms: 6 a 7 em 2 s.
    assert(ok >= 5 && ok <= 7);
  }
  // 3. Os passos seguram o intervalo (nunca dois a menos de ~PASSO-FOLGA).
  { NavRitmo m = {0}; unsigned ant = 0, t; int primeiro = 1;
    for (t = 0; t <= 3000; t += 40) if (navritmo_baixou(&m, t, R)) {
      if (!primeiro && ant) assert(t - ant + NAV_FOLGA_MS >= NAV_PASSO_MS || ant == 0);
      primeiro = 0; ant = t; } }
  // 4. Primeiro passo extra so depois da espera.
  { NavRitmo m = {0}; assert(navritmo_baixou(&m, 0, R));
    for (unsigned t = 40; t < NAV_ESPERA_MS; t += 40) assert(!navritmo_baixou(&m, t, R)); }
  // 5. Soltar e apertar de novo e um toque novo, na hora.
  { NavRitmo m = {0}; assert(navritmo_baixou(&m, 0, R)); navritmo_soltou(&m, R);
    assert(navritmo_baixou(&m, 100, R));            // toque seguinte, sem esperar
    assert(!navritmo_baixou(&m, 110, R));           // seguro: nao e toque novo
  }
  // 6. Trocar de seta anda na hora.
  { NavRitmo m = {0}; assert(navritmo_baixou(&m, 0, R)); assert(navritmo_baixou(&m, 60, L)); }
  // 7. Pausa maior que NAV_PAUSA_MS: toque novo mesmo sem KEYUP.
  { NavRitmo m = {0}; assert(navritmo_baixou(&m, 0, R)); assert(navritmo_baixou(&m, 600, R)); }
  // 8. Sem fila: parar de mandar eventos nao produz mais nada.
  { NavRitmo m = {0}; unsigned t; int ok = segurar(&m, R, 0, 1000, 40, &t); (void)ok;
    assert(navritmo_baixou(&m, 5000, R) == 1 && !navritmo_baixou(&m, 5040, R)); }
  puts("navritmo: ok");
  return 0;
}
