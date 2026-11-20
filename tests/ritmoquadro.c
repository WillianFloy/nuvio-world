// RITMO DOS QUADROS: estatisticas de frame time, prazo, latencia e cadencia
// em frequencias simuladas (30/50/60/90/100/120 Hz). Simulacao, nao aparelho.
#include "../src/ritmoquadro.h"
#include <assert.h>
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
static RqRelato simula(double hz, const int *varreduras, int n, int repete) {
  static RitmoQuadro r;
  double p = 1000.0 / hz, t = 1000.0;
  memset(&r, 0, sizeof r);
  rq_quadro(&r, t, t - 5, 4, 1);
  for (int k = 0; k < repete; k++) for (int i = 0; i < n; i++) { t += varreduras[i] * p; rq_quadro(&r, t, t - 6, 5, 1); }
  return rq_relato(&r, p);
}
int main(void) {
  const double HZ[6] = { 30, 50, 60, 90, 100, 120 };
  for (int h = 0; h < 6; h++) {
    int um[1] = { 1 };
    RqRelato o = simula(HZ[h], um, 1, 300);
    char m[96];
    snprintf(m, sizeof m, "%.0f Hz cadencia cheia: fps=%.1f p99=periodo, 0 fora do prazo", HZ[h], o.fps);
    OK(fabs(o.fps - HZ[h]) < 0.05 && fabs(o.p99 - 1000.0 / HZ[h]) < 0.05 && o.foraPrazo == 0 && o.hist[0] == 300, m);
  }
  { int dois[1] = { 2 }; RqRelato o = simula(60, dois, 1, 200);
    OK(fabs(o.fps - 30.0) < 0.05 && o.hist[1] == 200 && o.pulados == 200, "60 Hz a cada 2 varreduras: 30 fps uniformes (hist na coluna 2)"); }
  { int alt[2] = { 1, 2 }; RqRelato o = simula(60, alt, 2, 100);
    OK(fabs(o.fps - 40.0) < 0.05 && o.hist[0] == 100 && o.hist[1] == 100 && o.p50 < o.p95,
       "40 fps em 60 Hz: media 40 mas cadencia 16,7/33,3 (o histograma mostra o vai-e-vem)"); }
  { int tres[1] = { 3 }; RqRelato o = simula(120, tres, 1, 200);
    OK(fabs(o.fps - 40.0) < 0.05 && o.hist[2] == 200 && o.p50 == o.p99, "40 fps em 120 Hz: uniforme (3 varreduras sempre)"); }
  { int eng[10] = { 1,1,1,1,1,1,1,1,1,4 }; RqRelato o = simula(60, eng, 10, 30);
    OK(o.foraPrazo == 30 && o.pulados == 90 && o.max > 66.0 && o.p50 < 17.0, "engasgo de 1 em 10: p50 normal, p99/max e fora-do-prazo acusam"); }
  OK(rq_cadencia_uniforme(120, 40) == 3 && rq_cadencia_uniforme(60, 40) == 0 && rq_cadencia_uniforme(60, 30) == 2
     && rq_cadencia_uniforme(120, 30) == 4 && rq_cadencia_uniforme(90, 45) == 2 && rq_cadencia_uniforme(50, 25) == 2
     && rq_cadencia_uniforme(100, 50) == 2 && rq_cadencia_uniforme(60, 45) == 0, "cadencia uniforme = divisao inteira da frequencia");
  { double g[2] = { 30, 60 }; OK(fabs(rq_media_harmonica(g, 2) - 40.0) < 1e-9, "media harmonica de grupos de mesmos quadros (30 e 60 -> 40)"); }
  { static RitmoQuadro r; RqRelato o; memset(&r, 0, sizeof r);
    rq_quadro(&r, 100, 90, 5, 1);
    rq_tecla(&r, 105);                 // tecla no meio do quadro que comecou em 101
    rq_quadro(&r, 117, 101, 5, 1);     // este quadro comecou ANTES da tecla: nao responde
    rq_quadro(&r, 133, 117, 5, 1);     // este sim
    o = rq_relato(&r, 16.667);
    OK(o.nLat == 1 && fabs(o.lat50 - 28.0) < 0.01, "controle->tela: so o quadro iniciado depois da tecla conta (28 ms)"); }
  if (falhas) return 1;
  puts("ritmoquadro: ok");
  return 0;
}
