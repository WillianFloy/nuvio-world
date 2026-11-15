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
  // VERTICAL (fileiras): a focada fica sempre no topo (sob a sinopse), a janela vai
  // ate o pe da tela; a anterior nao aparece em cima, a ultima cabe inteira no
  // mesmo ponto e a seguinte espia embaixo sem cartaz cortado por janela menor.
  { const float H = 1080.0f, gapv = 40.0f, titulo = 46.0f;
    const float topos[] = { 518.0f, 544.0f, 480.0f };
    const float cont[] = { titulo + 182.9f, titulo + 322.0f, titulo + 322.0f + 74.0f };
    for (size_t a2 = 0; a2 < sizeof topos / sizeof *topos; a2++)
      for (size_t b2 = 0; b2 < sizeof cont / sizeof *cont; b2++) {
        float topo = topos[a2], c = cont[b2], pas = c + gapv, clipTopo = topo - 8.0f;
        // a fileira focada (qualquer uma, ate a ultima) cabe inteira na tela
        assert(topo + c <= H);
        for (int foco = 0; foco < 20; foco++) {
          float scroll = foco * pas;
          // a anterior termina acima do recorte de cima: nunca espia em cima
          if (foco > 0) assert(topo + (foco - 1) * pas - scroll + c < clipTopo);
          // a focada esta no topo; a seguinte comeca abaixo do fim da focada
          assert(fabsf(topo + foco * pas - scroll - topo) < 0.01f);
          assert(topo + (foco + 1) * pas - scroll > topo + c);
        }
        // a janela vai ate o pe da tela: o que aparece da seguinte nao e cortado por outra janela
        float visivel = H - (topo + pas + titulo);
        assert(visivel <= 322.0f * 1.0f + 1.0f || c < titulo + 322.0f);
      }
    // casos conhecidos (1080p, topo 518): retrato sem rotulo espia ~1/3 da seguinte
    { float pas = 46.0f + 322.0f + 40.0f, visivel = 1080.0f - (518.0f + pas + 46.0f);
      assert(visivel > 322.0f * 0.30f && visivel < 322.0f * 0.40f); }
  }
  puts("vitgeom: tudo ok");
  return 0;
}
