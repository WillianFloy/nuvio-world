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
  // AS DUAS PONTAS COM A MESMA RAMPA (24/11): a direita tem a largura da
  // esquerda e o ultimo cartao inteiro nunca entra nela.
  {
    const float W = 1920.0f, L = 40.0f;
    for (float fimInt = 800.0f; fimInt <= 1919.0f; fimInt += 37.0f) {
      float cheio = vitgeom_fade_dir_cheio(fimInt, W, L);
      assert(cheio >= fimInt - 0.001f || cheio >= W - 1.0f);       // cartao inteiro intacto
      if (fimInt <= W - L) assert(fabsf((W - cheio) - L) < 0.001f);   // rampa = 40 px, como a esquerda
      else assert(W - cheio <= L + 0.001f);                         // nunca maior que a esquerda
      assert(cheio <= W - 1.0f);
    }
    // o caso do relato: sobra quase um cartao a direita (fimInt = 1450) NAO vira
    // um degrade de 470 px
    assert(W - vitgeom_fade_dir_cheio(1450.0f, W, L) <= L + 0.001f);
  }

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
  // Foco fixo (2.0.33): espiadas iguais, ~meio cartaz, foco nunca na espiada.
  { const float S = 1920.0f; const float ws[] = {190.0f, 340.0f, 150.0f, 260.0f}, vaos[] = {24.0f, 24.0f, 18.0f, 30.0f};
    for (int k = 0; k < 4; k++) {
      float w = ws[k], passo = w + vaos[k];
      int n = vitgeom_fixo_n(S, w, passo);
      float A = vitgeom_fixo_ancora(S, w, passo, n);
      float esq = A - vaos[k], dir = S - (A + n * passo);
      assert(n >= 1);
      assert(fabsf(esq - dir) < 0.01f);                       // mesma espiada nos dois lados
      assert(esq >= VG_ESPIA_MIN * w - 0.5f && esq <= 0.75f * w);  // ~meio cartaz, nunca exagerado
      printf("w=%.0f n=%d espiada=%.0f (%.2f w) vazio esq. no 1o=%.0f\n", w, n, esq, esq / w, A);
      assert(A + (n - 1) * passo + w <= S - esq + 0.01f);     // ultimo inteiro antes da espiada
      for (int total = 1; total < 30; total++)
        for (int col = 0; col < total; col++) {
          float x = A + col * passo - vitgeom_fixo_alvo(col, total, n, passo);
          assert(x >= A - 0.01f && x + w <= A + (n - 1) * passo + w + 0.01f);
          if (col <= total - n) assert(fabsf(x - A) < 0.01f);   // foco preso na ancora
        }
    }
    puts("vitgeom foco fixo: ok"); }
  { // 2.0.36: pre-carga direcional — ordem, limites, sem duplicata, direcao.
    int nCols[6] = {12, 12, 12, 0, 12, 5}, lemb[6] = {0, 3, 0, 0, 7, 2};
    VgPre o[64];
    int n = vitgeom_prebusca(1, 2, 0, +1, +1, 6, nCols, lemb, 5, o, 64);
    assert(n > 0 && o[0].r == 1 && o[0].c == 2 + 5);          // horizontal: 1o fora da tela, a frente
    assert(o[1].r == 1 && o[1].c == 2 + 6 && o[3].c == 2 + 8);
    for (int i = 0; i < n; i++) {                              // limites e unicidade
      assert(o[i].r >= 0 && o[i].r < 6 && o[i].c >= 0 && o[i].c < nCols[o[i].r]);
      for (int j = i + 1; j < n; j++) assert(!(o[i].r == o[j].r && o[i].c == o[j].c));
    }
    n = vitgeom_prebusca(1, 8, 0, -1, +1, 6, nCols, lemb, 5, o, 64);   // andando para a esquerda
    assert(n > 0 && o[0].r == 1 && o[0].c == 8 - 5);
    n = vitgeom_prebusca(1, 2, 1, +1, +1, 6, nCols, lemb, 5, o, 64);   // vertical para baixo
    assert(o[0].r == 2 && o[0].c == lemb[2] && o[4].r == 2 && o[4].c == lemb[2] + 4);
    { int viuLinha3 = 0; for (int i = 0; i < n; i++) if (o[i].r == 3) viuLinha3 = 1; assert(!viuLinha3); }  // fileira vazia
    n = vitgeom_prebusca(4, 0, 1, -1, -1, 6, nCols, lemb, 5, o, 64);   // vertical para cima
    assert(o[0].r == 3 + 0 || o[0].r == 2);                    // pula a fileira sem cartazes
    assert(o[0].r == 2 && o[0].c == lemb[2]);
    n = vitgeom_prebusca(0, 0, 1, +1, -1, 6, nCols, lemb, 5, o, 64);   // sem fileira acima: so a horizontal
    for (int i = 0; i < n; i++) assert(o[i].r >= 0);
    n = vitgeom_prebusca(0, 0, 1, +1, +1, 6, nCols, lemb, 5, o, 3);    // respeita o teto
    assert(n == 3);
    assert(vitgeom_prebusca(0, 0, 0, 1, 1, 0, nCols, lemb, 5, o, 8) == 0);   // sem fileiras
    assert(vitgeom_prebusca(9, 0, 0, 1, 1, 6, nCols, lemb, 5, o, 8) == 0);   // foco fora
    puts("vitgeom pre-carga direcional: ok"); }
  // ESPIADA DO ANTERIOR (World 22/11): o foco da coluna 1 em diante fica `esp` a direita do inicio,
  // o anterior aparece numa faixa maior que o vao, nada passa do fim e o ultimo encosta na direita.
  { const float ws[] = { 160.0f, 213.0f, 284.0f, 380.0f, 50.0f, 600.0f };
    for (unsigned k = 0; k < sizeof ws / sizeof ws[0]; k++) {
      float w = ws[k], passo = w + 22.0f, janela = 1600.0f, esp = vitgeom_espia_ant(w);
      int total = 30;
      float max = total * passo - (passo - w) - janela;
      assert(esp >= 64.0f && esp <= 150.0f);
      assert(vitgeom_espia_alvo(0, total, w, passo, janela, esp) == 0.0f);            // coluna 0 alinhada
      for (int c = 1; c < total; c++) {
        float a = vitgeom_espia_alvo(c, total, w, passo, janela, esp);
        float xf = c * passo - a, xa = (c - 1) * passo - a;                              // relativos ao inicio
        assert(a >= 0.0f && a <= max + 0.01f);
        assert(xf + w <= janela + 0.01f);                                                 // foco inteiro
        if (a < max - 0.01f) {
          assert(fabsf(xf - esp) < 0.01f);                                                 // foco no mesmo x
          assert(xa + w > 0.0f && xa + w - 0.0f >= esp - (passo - w) - 0.01f);             // a faixa do anterior aparece
        }
        if (c > 1) assert(a >= vitgeom_espia_alvo(c - 1, total, w, passo, janela, esp));  // monotono
      }
      float aUlt = vitgeom_espia_alvo(total - 1, total, w, passo, janela, esp);
      assert(fabsf((total - 1) * passo - aUlt + w - janela) < 0.01f);                      // o ultimo encosta na direita
      assert(vitgeom_espia_alvo(1, 2, w, passo, janela, esp) == 0.0f);                    // fileira que cabe nao rola
    }
    puts("vitgeom espiada do anterior: ok"); }
  return 0;
}
