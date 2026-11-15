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
// Fileira (ou cartao) do topo da janela quando o foco esta em `foco`: o foco sobe
// para o topo, mas a rolagem PARA no fim — as ultimas `linhas` ficam todas a
// vista, sem vao em branco abaixo da ultima.
static inline int vitgeom_alvo_linha(int foco, int linhas, int total) {
  int max = total > linhas ? total - linhas : 0;
  if (foco < 0) foco = 0;
  return foco > max ? max : foco; }
static inline float vitgeom_folga(float anel, float w, float passo) {
  float meio = (passo - w) * 0.5f, f = anel + 2.0f;
  return f < meio ? f : meio;
}

#endif
