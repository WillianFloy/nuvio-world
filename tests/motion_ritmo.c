// Tempo-base do movimento e ritmo de segurar a seta nas Collections (2.0.40).
// Usa a fisica REAL (anim.h) e as constantes REAIS (layout.h, navritmo.h).
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/anim.h"
#include "../src/navritmo.h"
// So as constantes de tempo do layout.h (o resto do cabecalho nao entra no teste).
#include "motion_consts.h"

static float tempo_ate(float w, float p) {
  float v = 0, x = 0;
  for (int ms = 1; ms < 5000; ms++) { x = anim_mola2(&v, x, 1.0f, 0.001f, w); if (x >= p) return (float)ms; }
  return 9999.0f;
}
// Segura a tecla `dur` ms, o controle repetindo a cada `cad` ms; devolve o intervalo medio entre passos.
static double medio(unsigned cad, unsigned dur, int *passos, unsigned *menor, unsigned *maior) {
  NavRitmo n = {0}; unsigned t, ant = 0, p0 = 0; int k = 0; *menor = 99999; *maior = 0;
  for (t = 1000; t <= 1000 + dur; t += cad) if (navritmo_baixou_vitrine(&n, t, 1)) {
    if (k >= 2) { unsigned d = t - ant; if (d < *menor) *menor = d; if (d > *maior) *maior = d; }
    if (k == 1) p0 = t;
    ant = t; k++;
  }
  *passos = k;
  return k > 2 ? (double)(ant - p0) / (double)(k - 2) : 0.0;
}
int main(void) {
  // 1. O movimento visivel (90%) leva o tempo-base, na velocidade normal e com "geral rapida" desligada.
  float t90 = tempo_ate(NV_MOTION_W2, 0.90f), t95 = tempo_ate(NV_MOTION_W2, 0.95f);
  printf("tempo-base: w=%.2f t90=%.0f ms t95=%.0f ms\n", NV_MOTION_W2, t90, t95);
  assert(fabsf(t90 - NV_MOTION_MS) <= 3.0f);
  // 2. Ganho do segurar sobre o toque (tempo de um toque / passo do segurar): 1,25x a 1,5x.
  float ganho = t90 / (float)NAV_PASSO_VITRINE_MS;
  printf("ganho do segurar: %.2fx (passo %u ms)\n", ganho, NAV_PASSO_VITRINE_MS);
  assert(ganho >= 1.25f && ganho <= 1.5f);
  // 3. A cadencia nao depende da repeticao do controle: media do intervalo ~= passo, para 33/40/50/100 ms.
  unsigned cads[] = {33, 40, 50, 66, 100};
  for (unsigned i = 0; i < sizeof cads / sizeof *cads; i++) {
    int k; unsigned mn, mx; double m = medio(cads[i], 5000, &k, &mn, &mx);
    printf("repeticao %3u ms: %d passos, intervalo medio %.0f ms (min %u, max %u)\n", cads[i], k, m, mn, mx);
    assert(fabs(m - (double)NAV_PASSO_VITRINE_MS) <= 12.0);
    assert(mn + cads[i] >= NAV_PASSO_VITRINE_MS && mx <= NAV_PASSO_VITRINE_MS + cads[i]);   // so oscila por 1 repeticao
  }
  // 4. Um toque anda um item na hora; o 1o passo extra so depois da espera; soltar para na hora.
  { NavRitmo n = {0};
    assert(navritmo_baixou_vitrine(&n, 1000, 7));
    for (unsigned t = 1040; t < 1000 + NAV_ESPERA_MS - 40; t += 40) assert(!navritmo_baixou_vitrine(&n, t, 7));
    navritmo_soltou(&n, 7);
    assert(navritmo_baixou_vitrine(&n, 1200, 7)); }
  // 5. Sem rajada depois de um travamento: 1,5 s sem eventos => toque novo; evento atrasado 400 ms => 1 passo so.
  { NavRitmo n = {0}; int ok = 0;
    for (unsigned t = 0; t <= 800; t += 40) ok += navritmo_baixou_vitrine(&n, t, 7);
    int antes = ok;
    ok += navritmo_baixou_vitrine(&n, 1000, 7);                   // atrasado (pausa > NAV_PAUSA_MS => toque novo)
    assert(ok == antes + 1);
    assert(!navritmo_baixou_vitrine(&n, 1040, 7)); }
  // 6. Trocar de seta anda na hora; parar de mandar eventos nao deixa fila.
  { NavRitmo n = {0}; assert(navritmo_baixou_vitrine(&n, 0, 1)); assert(navritmo_baixou_vitrine(&n, 60, 2));
    assert(navritmo_baixou_vitrine(&n, 9000, 2) == 1 && !navritmo_baixou_vitrine(&n, 9040, 2)); }
  puts("motion_ritmo: ok");
  return 0;
}
