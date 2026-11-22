// AJUSTES WORLD (TV, 21/11; layout aprovado pelo dono no quadro "Definitivo"):
// as regras puras da tela nova, sem GL, sem ajustes.c e sem SDL, para o teste
// de comportamento (tests/ajustes_world_modelo.c) e para quem desenha
// (src/ajustes_ux_world.inc).
//
//  - a grade de valores (opcao com muitos valores): colunas e o andar das setas,
//    pulando valor indisponivel;
//  - o "segurar OK volta ao padrao": OK curto age ao soltar, OK segurado
//    restaura uma vez e o soltar seguinte nao age;
//  - o rascunho das setas laterais: o valor muda na linha na hora e so grava
//    depois de uma pausa (ou quando o foco sai), para nao reaplicar idioma,
//    resolucao ou tema a cada toque;
//  - a fileira de abas e a rolagem da lista: o que esta em foco sempre a vista.
#ifndef NUVIO_AJWMODELO_H
#define NUVIO_AJWMODELO_H

// ---- grade de valores --------------------------------------------------------
// Ate 5 valores: uma coluna (lista curta le melhor). Ate 12: duas. Acima: tres.
static inline int ajw_grade_colunas(int n) { return n <= 5 ? 1 : n <= 12 ? 2 : 3; }

// Anda na grade de `n` valores em `cols` colunas. dx anda dentro da fileira;
// dy pula uma fileira (mesma coluna). `ok` (pode ser NULL) diz quais valores
// podem ser escolhidos: o andar pula os outros na mesma direcao e, sem nenhum
// adiante, fica onde estava. Nunca da a volta.
static inline int ajw_grade_mover(int atual, int n, int cols, int dx, int dy, const unsigned char *ok) {
  int passo, i;
  if (n <= 0) return 0;
  if (cols < 1) cols = 1;
  if (atual < 0 || atual >= n) atual = 0;
  passo = dx ? dx : dy * cols;
  if (!passo) return atual;
  for (i = atual + passo; i >= 0 && i < n; i += passo) {
    if (dx && i / cols != atual / cols) break;   // lateral nao muda de fileira
    if (!ok || ok[i]) return i;
  }
  // Baixo na ultima fileira incompleta: a ultima celula escolhivel.
  if (dy > 0 && !dx && (atual / cols) < (n - 1) / cols) {
    for (i = n - 1; i > atual; i--) if (!ok || ok[i]) return i;
  }
  return atual;
}

// ---- segurar OK ----------------------------------------------------------------
#define AJW_SEGURAR_MS 650u
typedef struct { unsigned desde; int armado, usado; } AjwSegurar;

// OK desceu numa linha. Sem padrao para voltar, o OK segue o caminho de sempre
// (retorna 0: quem chamou age na hora). Com padrao, arma e espera o soltar.
static inline int ajw_ok_baixo(AjwSegurar *s, unsigned agora, int podeRestaurar) {
  s->usado = 0;
  if (!podeRestaurar) { s->armado = 0; return 0; }
  s->armado = 1; s->desde = agora;
  return 1;
}
// Repeticao do controle ou o relogio do quadro: 1 = restaurar agora (uma vez).
static inline int ajw_ok_tique(AjwSegurar *s, unsigned agora) {
  if (!s->armado || agora - s->desde < AJW_SEGURAR_MS) return 0;
  s->armado = 0; s->usado = 1;
  return 1;
}
// OK subiu: 1 = foi curto, aja agora; 2 = foi longo e nenhum tique restaurou
// ainda (o quadro travou): restaure agora; 0 = nada (ja restaurou ou nao armou).
static inline int ajw_ok_cima(AjwSegurar *s, unsigned agora) {
  if (s->usado) { s->usado = 0; return 0; }
  if (!s->armado) return 0;
  s->armado = 0;
  return agora - s->desde < AJW_SEGURAR_MS ? 1 : 2;
}
static inline void ajw_ok_soltar(AjwSegurar *s) { s->armado = s->usado = 0; }

// ---- rascunho das setas laterais ---------------------------------------------------
#define AJW_RASCUNHO_MS 700u
typedef struct { int op, v; unsigned ate; } AjwRascunho;   // op < 0 = nenhum

static inline void ajw_rasc_limpar(AjwRascunho *r) { r->op = -1; r->v = 0; r->ate = 0; }
// Passo `d` (-1/+1, ja multiplicado pelo passo da opcao) sobre o valor que a
// linha mostra: o rascunho se existe para esta opcao, senao o gravado.
// Limita em [lo, hi] sem dar a volta. Devolve 1 se o valor mudou.
static inline int ajw_rasc_passo(AjwRascunho *r, int op, int gravado, int lo, int hi, int d, unsigned agora) {
  int base = r->op == op ? r->v : gravado, novo = base + d;
  if (novo < lo) novo = lo;
  if (novo > hi) novo = hi;
  if (novo == base) return 0;
  r->op = op; r->v = novo; r->ate = agora + AJW_RASCUNHO_MS;
  return 1;
}
// 1 = a pausa acabou: grave agora.
static inline int ajw_rasc_vencido(const AjwRascunho *r, unsigned agora) {
  return r->op >= 0 && (int)(agora - r->ate) >= 0;
}

// ---- abas e rolagem ----------------------------------------------------------------
// Deslocamento da fileira de abas: zero se todas cabem; senao o minimo para a
// aba `ativa` (de x0 a x1 na fileira, sem deslocamento) ficar inteira dentro de
// [0, largura] com `folga` dos dois lados, sem passar do fim da fileira.
static inline float ajw_abas_desloc(float total, float largura, float x0, float x1, float folga, float atual) {
  float d = atual, max = total - largura;
  if (total <= largura) return 0.0f;
  if (x0 - d < folga) d = x0 - folga;
  if (x1 - d > largura - folga) d = x1 - (largura - folga);
  if (d < 0) d = 0;
  if (d > max) d = max;
  return d;
}
// Rolagem da lista: o item em foco (y0..y1, na lista sem rolagem) inteiro na
// janela [0, janela] com `folga`; a lista nunca rola alem do fim.
static inline float ajw_rolagem(float total, float janela, float y0, float y1, float folga, float atual) {
  float r = atual, max = total - janela;
  if (total <= janela) return 0.0f;
  if (y0 - r < folga) r = y0 - folga;
  if (y1 - r > janela - folga) r = y1 - (janela - folga);
  if (r < 0) r = 0;
  if (r > max) r = max;
  return r;
}

#endif
