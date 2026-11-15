// Geometria das fileiras da Vitrine: cartoes inteiros que cabem, janela e recorte.
// Sem GL: exercita vitgeom.h, a mesma conta que vertudo.c usa.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/vitgeom.h"

#define ANEL 4.0f
// Verifica, em REPOUSO (scroll = ini*passo), que dentro da janela de recorte
// so aparecem cartoes inteiros: nenhum cortado a esquerda nem a direita.
static void confere_repouso(float x0, float fim, float w, float passo, int total) {
  int n = vitgeom_cabem(x0, fim, w, passo), col, ini = 0;
  float f = vitgeom_folga(ANEL, w, passo);
  assert(n >= 1 && f > 0.0f && f < (passo - w) * 0.5f + 0.001f);
  float esq = x0 - f, dir = vitgeom_limite(x0, w, passo, n) + f;
  // o conteudo nunca passa da margem direita (exceto o minimo de 1 cartao)
  if (n > 1) assert(vitgeom_limite(x0, w, passo, n) <= fim + 0.01f);
  // nao sobra espaco para mais um cartao inteiro
  assert(n == 1 || x0 + n * passo + w > fim + 0.01f || (x0 + n * passo + w - fim) > -0.01f);
  // percorre o foco 0..total-1 e volta, como o D-pad
  for (int passo_dir = 0; passo_dir < 2; passo_dir++)
    for (int k = 0; k < total; k++) {
      col = passo_dir ? total - 1 - k : k;
      ini = vitgeom_janela(ini, col, n, total);
      assert(ini >= 0 && (total <= n ? ini == 0 : ini + n <= total));
      assert(col >= ini && col < ini + (n < total ? n : total));   // foco sempre visivel
      for (int c = 0; c < total; c++) {
        float cx = x0 + (c - ini) * passo, ce = cx + w;
        int visivel_algo = cx < dir && ce > esq;           // aparece na janela
        int inteiro = cx >= esq && ce <= dir;              // cabe inteiro nela
        assert(!visivel_algo || inteiro);                  // nunca cortado
        assert(inteiro == (c >= ini && c < ini + n));      // so a janela
      }
    }
}

int main(void) {
  // Layout 1920x1080 (px de layout): conteudo comeca em x0 e a margem direita e
  // NV_CHROME_EDGE (64). Passos medidos: retrato ~ 190+24, paisagem ~ 340+24.
  const float tela = 1920.0f, margem = 64.0f, gap = 24.0f;
  const float x0s[] = { 80.0f, 120.0f, 210.0f, 300.0f };
  const float ws[] = { 150.0f, 190.0f, 230.0f, 300.0f, 340.0f, 420.0f };   // retrato e paisagem
  for (size_t i = 0; i < sizeof x0s / sizeof *x0s; i++)
    for (size_t j = 0; j < sizeof ws / sizeof *ws; j++) {
      float x0 = x0s[i], w = ws[j], passo = w + gap, fim = tela - margem;
      int n = vitgeom_cabem(x0, fim, w, passo);
      for (int total = 1; total <= 45; total += (total < 12 ? 1 : 11)) confere_repouso(x0, fim, w, passo, total);
      // o numero e exatamente "quantos inteiros cabem": n cabe, n+1 nao
      if (n > 1) assert(x0 + (n - 1) * passo + w <= fim + 0.01f);
      assert(x0 + n * passo + w > fim);
    }
  // Casos conhecidos: 7 retrato e 4 paisagem em 1920, e 3 em tela estreita.
  assert(vitgeom_cabem(210.0f, 1856.0f, 190.0f, 214.0f) == 7);
  assert(vitgeom_cabem(210.0f, 1856.0f, 340.0f, 364.0f) == 4);
  assert(vitgeom_cabem(100.0f, 800.0f, 190.0f, 214.0f) == 3);
  assert(vitgeom_cabem(100.0f, 300.0f, 190.0f, 214.0f) == 1);   // minimo: 1 cartao
  assert(vitgeom_cabem(500.0f, 300.0f, 190.0f, 214.0f) == 1);   // fim antes do inicio
  // Fronteira exata: cabem exatamente 5 (fim = ultimo cartao encosta na margem)
  assert(vitgeom_cabem(100.0f, 100.0f + 4 * 214.0f + 190.0f, 190.0f, 214.0f) == 5);
  assert(vitgeom_cabem(100.0f, 100.0f + 4 * 214.0f + 188.0f, 190.0f, 214.0f) == 4);
  // Janela: so anda quando o foco sai; de um em um; para no fim.
  int n = 5, ini = 0;
  for (int col = 0; col < 5; col++) assert((ini = vitgeom_janela(ini, col, n, 20)) == 0);
  assert((ini = vitgeom_janela(ini, 5, n, 20)) == 1);
  assert((ini = vitgeom_janela(ini, 6, n, 20)) == 2);
  assert((ini = vitgeom_janela(ini, 5, n, 20)) == 2);     // volta dentro da janela: fica
  assert((ini = vitgeom_janela(ini, 1, n, 20)) == 1);     // sai pela esquerda: anda 1
  assert((ini = vitgeom_janela(ini, 19, n, 20)) == 15);   // foco no ultimo: janela encosta no fim
  assert(vitgeom_janela(7, 0, 5, 3) == 0);                // fileira menor que a janela
  assert(vitgeom_janela(99, 2, 5, 8) == 2);               // ini velho invalido e corrigido
  // Escala de texto/tela (px reais): mais escala => cartoes maiores => menos colunas.
  assert(vitgeom_cabem(210.0f, 1856.0f, 190.0f * 1.25f, 214.0f * 1.25f + 0.0f) <
         vitgeom_cabem(210.0f, 1856.0f, 190.0f, 214.0f));
  // VERTICAL (fileiras): mesma regra, so linhas inteiras e a rolagem para no fim.
  { const float H = 1080.0f, base = 40.0f, gapv = 40.0f;
    const float topos[] = { 518.0f, 544.0f, 480.0f };
    const float cont[] = { 46.0f + 182.9f, 46.0f + 322.0f + 74.0f, 46.0f + 260.0f, 46.0f + 700.0f };
    for (size_t a = 0; a < sizeof topos / sizeof *topos; a++)
      for (size_t b = 0; b < sizeof cont / sizeof *cont; b++) {
        float topo = topos[a], c = cont[b], pas = c + gapv, limite = H - base;
        int n = vitgeom_cabem(topo, limite, c, pas);
        assert(n >= 1);
        if (n > 1 || topo + c <= limite + 0.01f) assert(topo + (n - 1) * pas + c <= limite + 0.01f);
        // a fileira seguinte nao cabe inteira (ficaria cortada)
        assert(topo + n * pas + c > limite - 0.01f);
        // em repouso, com o foco em qualquer fileira: so linhas inteiras e foco visivel,
        // e a rolagem nunca passa do fim (a ultima fica toda a vista, sem vao embaixo)
        for (int total = 1; total <= 30; total++)
          for (int foco = 0; foco < total; foco++) {
            int topoL = vitgeom_alvo_linha(foco, n, total);
            assert(topoL >= 0 && topoL <= (total > n ? total - n : 0));
            assert(foco >= topoL && foco < topoL + (n < total ? n : total));
            float fim_ult = topo + (total - 1 - topoL) * pas + c;     // base da ultima fileira
            if (total > n) assert(fim_ult >= topo + (n - 1) * pas + c - 0.01f || topoL == total - n);
            // nenhuma fileira visivel passa do limite de baixo
            for (int r = topoL; r < total; r++) {
              float rb = topo + (r - topoL) * pas + c;
              if (r < topoL + n) { if (n > 1 || r > topoL) assert(rb <= limite + 0.01f); }
            }
          }
      }
    // casos conhecidos (1080p, topo 518): paisagem 2 linhas, retrato 1
    assert(vitgeom_cabem(518.0f, 1040.0f, 46.0f + 182.9f, 46.0f + 182.9f + 40.0f) == 2);
    assert(vitgeom_cabem(518.0f, 1040.0f, 46.0f + 322.0f + 74.0f, 46.0f + 322.0f + 74.0f + 40.0f) == 1);
    assert(vitgeom_alvo_linha(5, 2, 6) == 4 && vitgeom_alvo_linha(3, 2, 6) == 3);
    assert(vitgeom_alvo_linha(0, 2, 1) == 0 && vitgeom_alvo_linha(9, 3, 2) == 0);
  }
  puts("vitgeom: tudo ok");
  return 0;
}
