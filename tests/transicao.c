// 2.0.45: relogio adaptativo da transformacao Home -> Collection (src/transicao.h), puro.
#include "../src/transicao.h"
#include <stdio.h>
static int falhas;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
// Roda ate o fim; `prontoEm` = ms em que o conteudo fica pronto (<0 = nunca). Devolve a duracao total.
static float rodar(float prontoEm, float dt, int *mono, int *vivo, float *dvMax, float *revelaEm) {
  TrRelogio c; tr_iniciar(&c);
  float pAnt = 0.0f, vAnt = c.v; *mono = 1; *vivo = 1; *dvMax = 0.0f;
  for (int i = 0; i < 1000 && tr_ativo(&c); i++) {
    int pronto = prontoEm >= 0.0f && c.ms + dt >= prontoEm;
    tr_passo(&c, dt, pronto);
    if (c.p < pAnt - 1e-6f) *mono = 0;
    if (tr_ativo(&c) && c.v <= 0.0f) *vivo = 0;
    float dv = fabsf(c.v - vAnt); if (dv > *dvMax) *dvMax = dv;
    pAnt = c.p; vAnt = c.v;
  }
  *revelaEm = c.msRevela;
  return c.ms;
}
int main(void) {
  int mono, vivo; float dv, rev, total;
  const float dt = 1000.0f / 60.0f;
  total = rodar(0.0f, dt, &mono, &vivo, &dv, &rev);
  ok(total >= 595.0f && total <= 620.0f, "tudo pronto no clique: ~600 ms (saida 300 + revela 300)");
  ok(rev >= TR_SAIDA_MS - 1.0f && rev <= TR_SAIDA_MS + dt, "a revelacao nunca comeca antes de o cartao chegar ao destino");
  ok(mono && vivo, "quente: p so cresce e a velocidade nunca e zero antes do fim");
  total = rodar(500.0f, dt, &mono, &vivo, &dv, &rev);
  ok(total >= 795.0f && total <= 820.0f && rev >= 495.0f && rev <= 520.0f, "pronto em 500 ms: revela em ~500, fim em ~800 (a duracao se adapta)");
  ok(mono && vivo, "cruzeiro: desacelera sem congelar");
  total = rodar(-1.0f, dt, &mono, &vivo, &dv, &rev);
  ok(total <= TR_TETO_MS + dt && total >= TR_TETO_MS - dt, "nunca pronto: teto de 1000 ms");
  ok(rev >= TR_TETO_MS - TR_REVELA_MS - 1.0f && rev <= TR_TETO_MS - TR_REVELA_MS + dt, "o teto forca a revelacao em 700 ms");
  ok(mono && vivo && dv < 0.0006f, "sem degrau de velocidade na entrada da revelacao (Hermite com a velocidade do cruzeiro)");
  { TrRelogio c; tr_iniciar(&c); float vMin = 1; for (int i = 0; i < 40; i++) { tr_passo(&c, dt, 0); if (c.fase == TR_CRUZEIRO && c.v < vMin) vMin = c.v; }
    ok(c.fase == TR_CRUZEIRO && vMin > 0.0f && vMin < TR_V_SAIDA * 0.2f, "cruzeiro: a velocidade cai bem (desacelera) mas segue > 0"); }
  { float rr = 0; int okr = 1; for (int k = 0; k <= 4; k++) { if (tr_fileira(k, 0.0f) > 0.0f || tr_fileira(k, 1.0f) < 1.0f) okr = 0; rr += tr_fileira(k, 0.5f); }
    ok(okr && tr_fileira(0, 0.5f) > tr_fileira(3, 0.5f), "fileiras: em cascata dentro dos 300 ms da revelacao"); }
  ok(tr_cartao(0.0f) == 0.0f && tr_cartao(TR_P_SAIDA) >= 0.9999f, "o cartao sai da origem e chega ao destino no fim da saida");
  ok(fabsf(tr_escala(TR_P_SAIDA) - TR_ESCALA_CHEGADA) < 1e-5f && fabsf(tr_escala(1.0f) - 1.0f) < 1e-5f, "o destaque chega a 1,02 e assenta em 1,00");
  ok(tr_cartao_alfa(0.0f) >= 0.999f && tr_cartao_alfa(0.45f) <= 0.001f, "o cartao so cede o lugar na revelacao (troca de dono no mesmo retangulo)");
  ok(tr_home_recuo(0.0f) == 0.0f && tr_home_recuo(TR_P_SAIDA) >= 0.999f, "a Home recua durante a saida");
  ok(tr_cabecalho(TR_P_SAIDA) > 0.0f && tr_cabecalho(TR_P_CRUZ_MAX) > 0.9f, "o cabecalho se forma durante o movimento (antes do portao)");
  puts(falhas ? "transicao: FALHOU" : "transicao: ok");
  return falhas != 0;
}
