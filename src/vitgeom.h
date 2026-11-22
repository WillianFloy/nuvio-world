#ifndef NV_VITGEOM_H
#define NV_VITGEOM_H
#include <math.h>
// Geometria horizontal das fileiras da Vitrine das colecoes (TV), sem dependencia
// de GL nem de estado do app: assim o teste exercita EXATAMENTE a conta usada.
//
// Regra (a mesma da grade, vtColunas): so contam cartoes INTEIROS que cabem entre
// o inicio do conteudo e a margem direita; o cartaz nao encolhe e o vizinho nunca
// aparece cortado na lateral. Qualquer formato: quem manda e o passo medido
// (largura do cartao + vao), entao retrato e paisagem se ajustam sozinhos.

#define VG_SEM_LIMITE (1 << 20)

// Cartoes inteiros que cabem em [x0, fim]. fim <= x0 ou passo <= 0 -> 1.
static inline int vitgeom_cabem(float x0, float fim, float w, float passo) {
  if (passo <= 0.0f) return 1;
  int n = (int)((fim - x0 + (passo - w)) / passo + 0.001f);
  return n < 1 ? 1 : n;
}
// 1o cartao visivel. Fica onde esta enquanto o foco cabe na janela; anda um
// cartao por vez quando o foco sai por uma das pontas; nunca passa do que sobra.
static inline int vitgeom_janela(int ini, int col, int n, int total) {
  int max = total > n ? total - n : 0;
  if (col < ini) ini = col;
  else if (col >= ini + n) ini = col - n + 1;
  return ini < 0 ? 0 : ini > max ? max : ini;
}
// Borda direita do ultimo cartao inteiro da janela.
static inline float vitgeom_limite(float x0, float w, float passo, int n) {
  return x0 + (n - 1) * passo + w;
}
// Folga do recorte dos dois lados da janela: cobre o anel de foco (anel px) mas
// fica abaixo de metade do vao, para o cartao vizinho nunca entrar.
static inline float vitgeom_folga(float anel, float w, float passo) {
  float meio = (passo - w) * 0.5f, f = anel + 2.0f;
  return f < meio ? f : meio;
}

// --- FOCO FIXO (TV, 2.0.33), o modelo da Netflix ---------------------------------
// O cartaz em foco fica sempre na mesma coluna da tela (a ancora) e a fileira
// desliza por baixo dele. A tela [0, S] tem n cartoes inteiros e, nas duas
// pontas, a mesma "espiada" do vizinho (aprox. meio cartaz), que esvanece ate a
// borda. Nada de margem diferente entre esquerda e direita.
#define VG_ESPIA_MIN 0.15f   // espiada minima, em fracao da largura do cartaz
#define VG_ESPIA_ALVO 0.45f  // a espiada preferida (~meio cartaz)

static inline float vitgeom_fixo_espia(float S, float w, float passo, int n) {
  return (S - n * passo - (passo - w)) * 0.5f;
}
// Cartoes inteiros entre as duas espiadas: dos dois numeros possiveis, o que deixa
// a espiada mais perto de meio cartaz (sem passar do minimo). Assim o vazio da
// esquerda com o foco no 1o cartaz fica pequeno.
static inline int vitgeom_fixo_n(float S, float w, float passo) {
  int n;
  float e1, e0;
  if (passo <= 0.0f || w <= 0.0f) return 1;
  n = (int)((S - 2.0f * VG_ESPIA_MIN * w - (passo - w)) / passo + 0.001f);
  if (n < 1) return 1;
  e1 = vitgeom_fixo_espia(S, w, passo, n);
  e0 = vitgeom_fixo_espia(S, w, passo, n - 1);
  if (n > 1 && fabsf(e0 - VG_ESPIA_ALVO * w) < fabsf(e1 - VG_ESPIA_ALVO * w)) n--;
  return n;
}
// Ancora: x do cartaz em foco. Sobra dividida para a espiada visivel (sem o vao)
// ser IGUAL nos dois lados: esquerda = A - vao, direita = S - (A + n*passo).
static inline float vitgeom_fixo_ancora(float S, float w, float passo, int n) {
  float vao = passo - w, sobra = S - n * passo;
  return (sobra + vao) * 0.5f;
}
// Deslocamento da fileira: o foco na ancora; no fim a fileira para e o foco anda
// ate o ultimo cartaz inteiro (nunca entra na espiada).
static inline float vitgeom_fixo_alvo(int col, int total, int n, float passo) {
  int max = total > n ? total - n : 0;
  if (col < 0) col = 0;
  return (float)(col > max ? max : col) * passo;
}

// --- ESPIADA DO ANTERIOR (World 22/11, dono: "quando nao ta horizontal ele nao
// mostra o anterior") ------------------------------------------------------------
// A coluna 0 fica no inicio do conteudo, alinhada ao titulo. Da coluna 1 em diante
// o cartaz em foco fica `esp` px a direita, e o anterior aparece na faixa que sobra
// (esp menos o vao). Com a margem de 2,5% (B3) a espiada antiga tinha so 48 px e
// esvanecia inteira: o cartaz em pe sumia (o deitado, mais largo, ainda mostrava
// uma tira). O valor: ~42% do cartaz, entre 64 e 150 px.
#define VG_ESPIA_ANT_FRAC 0.42f
static inline float vitgeom_espia_ant(float w) {
  float e = w * VG_ESPIA_ANT_FRAC;
  return e < 64.0f ? 64.0f : e > 150.0f ? 150.0f : e;
}
// Rolagem da fileira com o foco em `col`: coluna 0 em 0; depois col x passo - esp,
// sem passar do fim (o ultimo cartaz encosta na borda direita da `janela`, a
// largura entre o inicio do conteudo e a margem direita).
static inline float vitgeom_espia_alvo(int col, int total, float w, float passo, float janela, float esp) {
  float alvo = col > 0 ? (float)col * passo - esp : 0.0f;
  float max = (float)total * passo - (passo - w) - janela;
  if (alvo > max) alvo = max;
  return alvo < 0.0f ? 0.0f : alvo;
}

// PRE-CARGA DIRECIONAL (2.0.36). A Vitrine pedia cada cartaz so quando ele
// aparecia (0 chamadas a tex_lookahead_* em vertudo.c): na TV de 932 MB cada
// passo para uma fileira/coluna nova mostrava cartazes cinza ate a rede e o
// decode terminarem. Esta conta devolve, NA ORDEM, os cartazes que ainda nao
// estao na tela e que o foco vai alcançar primeiro, pela direcao em que ele anda:
//  - eixo 0 (horizontal): adiante na fileira (a partir do 1o fora da tela), depois
//    os 2 de tras que ja passaram da espiada, depois as fileiras vizinhas;
//  - eixo 1 (vertical): a fileira seguinte na direcao (na coluna lembrada), a
//    outra depois dela, e so entao a fileira de baixo/cima do outro lado, e a
//    horizontal.
// `nAdiante` = cartoes do foco ate a borda direita. Os que ja estao na tela ou
// em cache nao custam nada: quem pede (tex_lookahead_pedir) recusa o que o
// cache ja tem, e so especula com o cache ocioso, sem pressao de memoria.
typedef struct { short r, c; } VgPre;
static inline int vitgeom_pre_add(VgPre *out, int n, int max, int r, int c,
                                  int nLin, const int *nCols) {
  if (n >= max || r < 0 || r >= nLin || c < 0 || c >= nCols[r]) return n;
  for (int i = 0; i < n; i++) if (out[i].r == r && out[i].c == c) return n;
  out[n].r = (short)r; out[n].c = (short)c;
  return n + 1;
}
static inline int vitgeom_prebusca(int fr, int fc, int eixo, int dh, int dv,
                                   int nLin, const int *nCols, const int *colLemb,
                                   int nAdiante, VgPre *out, int max) {
  int n = 0, k, d, rr;
  if (!out || max <= 0 || !nCols || nLin <= 0 || fr < 0 || fr >= nLin) return 0;
  if (nAdiante < 1) nAdiante = 1;
  dh = dh < 0 ? -1 : 1; dv = dv < 0 ? -1 : 1;
  for (int passo = 0; passo < 2; passo++) {
    if ((passo == 0) == (eixo == 0)) {   // horizontal
      for (d = nAdiante; d <= nAdiante + 3; d++) n = vitgeom_pre_add(out, n, max, fr, fc + dh * d, nLin, nCols);
      for (d = 2; d <= 3; d++) n = vitgeom_pre_add(out, n, max, fr, fc - dh * d, nLin, nCols);
    } else {                             // vertical
      for (d = 1; d <= 3; d++) {
        rr = fr + dv * d;
        if (rr < 0 || rr >= nLin) break;
        int base = colLemb ? colLemb[rr] : fc;
        int quantos = d == 3 ? 3 : nAdiante;
        for (k = 0; k < quantos; k++) n = vitgeom_pre_add(out, n, max, rr, base + k, nLin, nCols);
      }
    }
  }
  return n;
}

#endif
