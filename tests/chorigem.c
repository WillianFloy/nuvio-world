// A regra de origem dos paineis do CH (pura): bash tests/chorigem.sh
#include "chorigem.h"
#include <stdio.h>
static int f;
static void ok(int c, const char *m) { if (!c) { f++; printf("FALHA: %s\n", m); } }
int main(void) {
  // Automatica: topo > relogio > centro, para os tres paineis
  for (int p = 0; p < 3; p++) {
    ok(chorigem_decidir(p, 0, 1, 1, 1) == CHO_TOPO, "auto: topo visivel -> topo");
    ok(chorigem_decidir(p, 0, 1, 0, 1) == CHO_RELOGIO, "auto: sem topo, relogio -> relogio");
    ok(chorigem_decidir(p, 0, 1, 0, 0) == CHO_CENTRO, "auto: nada -> centro");
    ok(chorigem_decidir(p, 1, 1, 1, 1) == CHO_RELOGIO, "relogio fixo: mesmo com o topo");
    ok(chorigem_decidir(p, 1, 1, 1, 0) == CHO_CENTRO, "relogio fixo sem relogio -> centro");
    ok(chorigem_decidir(p, 2, 1, 1, 1) == CHO_CENTRO, "centro fixo");
  }
  // "Busca a partir do menu do topo" desligado: so a Busca deixa de nascer da lupa
  ok(chorigem_decidir(CHO_P_BUSCA, 0, 0, 1, 1) == CHO_RELOGIO, "busca sem a lupa: relogio");
  ok(chorigem_decidir(CHO_P_BUSCA, 0, 0, 1, 0) == CHO_CENTRO, "busca sem a lupa e sem relogio: centro");
  ok(chorigem_decidir(CHO_P_SALVOS, 0, 0, 1, 1) == CHO_TOPO, "o ajuste da lupa nao muda os Salvos");
  { GfxRect c = chorigem_centro(1920); ok(c.x + c.w * 0.5f == 960.0f && c.y < 100.0f, "centro: no alto, no meio"); }
  printf("%s\n", f ? "chorigem: FALHOU" : "chorigem: ok");
  return f != 0;
}
