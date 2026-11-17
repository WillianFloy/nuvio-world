// Curvas do texto do destaque (2.0.42): escalonamento logo -> meta -> sinopse.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/herotexto.h"
static int n;
#define OK(c) do { if (!(c)) { printf("FALHOU: %s (linha %d)\n", #c, __LINE__); n++; } } while (0)
int main(void) {
  // Tempos aprovados: saida 150, entrada 300, 60 entre elementos; fim em 520.
  OK(HT_SAIDA_MS == 150.0f && HT_ENTRA_MS == 300.0f && HT_ESCALA_MS == 60.0f);
  OK(fabsf(HT_TOTAL_MS - 520.0f) < 1e-3f);
  // Antes de comecar nada aparece; depois do fim tudo assentado.
  for (int i = 0; i < HT_N; i++) {
    OK(ht_entra_alfa(i, 0.0f) == 0.0f && ht_entra_dy(i, 0.0f) == HT_ENTRA_PX);
    OK(ht_entra_alfa(i, HT_TOTAL_MS) == 1.0f && ht_entra_dy(i, HT_TOTAL_MS) == 0.0f);
    // Cada elemento leva exatamente 300 ms e comeca no seu instante.
    float ini = HT_ATRASO_MS + HT_ESCALA_MS * (float)i;
    OK(ht_entra_alfa(i, ini) == 0.0f && ht_entra_alfa(i, ini + HT_ENTRA_MS) == 1.0f);
    OK(ht_entra_alfa(i, ini + HT_ENTRA_MS * 0.5f) > 0.45f && ht_entra_alfa(i, ini + HT_ENTRA_MS * 0.5f) < 0.55f);
    // Monotono (nunca volta atras).
    float ant = -1.0f, antDy = 1e9f;
    for (float t = 0; t <= 600.0f; t += 4.0f) {
      float a = ht_entra_alfa(i, t), dy = ht_entra_dy(i, t);
      OK(a >= ant - 1e-6f && dy <= antDy + 1e-6f && a >= 0.0f && a <= 1.0f); ant = a; antDy = dy; }
  }
  // Ordem: em qualquer instante o logo esta a frente da meta, e a meta da sinopse.
  for (float t = 0; t <= 520.0f; t += 4.0f)
    OK(ht_entra_alfa(0, t) >= ht_entra_alfa(1, t) - 1e-6f && ht_entra_alfa(1, t) >= ht_entra_alfa(2, t) - 1e-6f);
  // Ninguem acende antes de o antigo ter quase sumido: aos 100 ms (inicio do logo) o antigo
  // assentado esta a <20% (so ~1/3 da saida falta).
  HtInstante cheio = ht_instante(HT_TOTAL_MS);
  OK(cheio.a[0] == 1.0f && cheio.dy[2] == 0.0f);
  OK(ht_saida_alfa(&cheio, 0, 0.0f) == 1.0f && ht_saida_alfa(&cheio, 0, HT_SAIDA_MS) == 0.0f);
  OK(ht_saida_alfa(&cheio, 1, HT_ATRASO_MS) < 0.26f);
  OK(ht_saida_dy(&cheio, 0, HT_SAIDA_MS) == -HT_SOBE_PX);
  // Continuidade na INTERRUPCAO: o bloco antigo comeca exatamente de onde o novo estava (sem salto).
  for (float t = 0; t <= 520.0f; t += 13.0f) {
    HtInstante s = ht_instante(t);
    for (int i = 0; i < HT_N; i++) {
      OK(fabsf(ht_saida_alfa(&s, i, 0.0f) - ht_entra_alfa(i, t)) < 1e-6f);
      OK(fabsf(ht_saida_dy(&s, i, 0.0f) - ht_entra_dy(i, t)) < 1e-6f);
      // ...e daí so apaga (monotono) ate zero.
      float ant = 2.0f; for (float u = 0; u <= HT_SAIDA_MS; u += 5.0f) { float a = ht_saida_alfa(&s, i, u); OK(a <= ant + 1e-6f); ant = a; }
      OK(ht_saida_alfa(&s, i, HT_SAIDA_MS) == 0.0f);
    }
  }
  // Passo maximo por quadro (60 fps): nenhuma mudanca de alfa maior que 12% entre quadros.
  float maxPasso = 0;
  for (int i = 0; i < HT_N; i++) for (float t = 0; t < 600.0f; t += 16.7f) {
    float d = fabsf(ht_entra_alfa(i, t + 16.7f) - ht_entra_alfa(i, t)); if (d > maxPasso) maxPasso = d; }
  OK(maxPasso < 0.12f);
  OK(ht_acabou(HT_TOTAL_MS) && !ht_acabou(HT_TOTAL_MS - 1.0f));
  // Vitrine: dentro da janela de 350 ms; logo 30-100%, meta 42-100%, sinopse 54-100%.
  OK(ht_vit_entra(0, 0.30f) == 0.0f && ht_vit_entra(1, 0.42f) == 0.0f && ht_vit_entra(2, 0.54f) == 0.0f);
  for (int i = 0; i < HT_N; i++) OK(ht_vit_entra(i, 1.0f) == 1.0f && ht_vit_entra(i, 0.0f) == 0.0f);
  for (float p = 0; p <= 1.0f; p += 0.01f)
    OK(ht_vit_entra(0, p) >= ht_vit_entra(1, p) - 1e-6f && ht_vit_entra(1, p) >= ht_vit_entra(2, p) - 1e-6f);
  puts(n ? "herotexto: FALHOU" : "herotexto: ok");
  return n ? 1 : 0;
}
