#ifndef NV_VITGEOM_H
#define NV_VITGEOM_H
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
#define VG_ESPIA_MIN 0.28f   // espiada minima, em fracao da largura do cartaz

// Cartoes inteiros entre as duas espiadas.
static inline int vitgeom_fixo_n(float S, float w, float passo) {
  int n;
  if (passo <= 0.0f || w <= 0.0f) return 1;
  n = (int)((S - 2.0f * VG_ESPIA_MIN * w - (passo - w)) / passo + 0.001f);
  return n < 1 ? 1 : n;
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

#endif
