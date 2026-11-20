// MODO DE DESEMPENHO (World, 20/11): o grid de apresentacao do Qualidade.
#include "../src/ritmo.h"
#include <stdio.h>
#include <math.h>
double ritmo_proximo_alvo(double ultimoAlvo, double agora, double periodo);
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
int main(void) {
  const double p = 1000.0 / 60.0;
  double alvo = 0, agora = 1000.0, prev = 0; int uniforme = 1;
  // Quadros leves (8 ms de desenho): alvo a cada 2 periodos, exatos.
  for (int i = 0; i < 60; i++) {
    alvo = ritmo_proximo_alvo(alvo, agora, p);
    if (i > 1 && fabs((alvo - prev) - 2 * p) > 1e-6) uniforme = 0;
    prev = alvo; agora = alvo - p + 8.0;   // o quadro seguinte comeca 1 periodo antes do alvo
  }
  OK(uniforme, "Qualidade: apresentacao a cada 33,3 ms exatos com quadro leve");
  // Quadro pesado (CPU passou do alvo): o grid recomeca no proximo slot, sem alvo no passado.
  { double a = ritmo_proximo_alvo(1000.0, 1100.0, p);
    OK(a >= 1100.0 + 0.5 * p, "quadro atrasado: o alvo nunca fica no passado"); }
  OK(fabs(ritmo_proximo_alvo(0.0, 50.0, p) - (50.0 + p)) < 1e-9, "primeiro quadro: 1 periodo adiante");
  if (falhas) return 1; puts("ritmo: ok"); return 0;
}
