// EXPANSAO DO CARTAZ EM REPOUSO, entrada e saida (World, 20/11/2026).
//
// O gesto: o foco pousa num cartaz em pe e, depois do atraso, ele se abre em
// 16:9 mostrando a arte deitada, empurrando os vizinhos. O que o dono via
// (Home e Collections): "e seco e da uma leve bugada". As causas, no codigo:
//   1. Home: mover o foco zerava a abertura NUM QUADRO (expAbre = 0): o cartao
//      voltava a cartaz e os vizinhos saltavam ~570 px de uma vez.
//   2. As duas telas abriam e fechavam com a mola de PRIMEIRA ordem, que parte
//      com a velocidade maxima no primeiro quadro (o "seco").
//   3. A arte trocava de cartaz para deitada NUM QUADRO, na metade da abertura,
//      e a deitada costumava nao estar pronta: o cartao piscava vazio.
//   4. As duas artes eram pedidas pela largura do quadro, que cresce durante a
//      abertura: o cache promovia e re-decodificava no meio do movimento.
//
// Aqui fica o relogio, puro (sem SDL nem GL) para o teste: um cartao abrindo
// (o do foco) e ate EXP_SAI_N fechando, cada um com posicao e velocidade de
// uma mola criticamente amortecida (anim_mola2: parte e chega macia, sem
// passar do ponto). Voltar a um cartao que ainda fecha continua de onde ele
// esta. Quem desenha soma as aberturas por coluna (exp_propria, exp_empurra)
// e cruza as artes por exp_mistura_arte.
#ifndef NV_EXPANSAO_H
#define NV_EXPANSAO_H
#include "anim.h"

#define EXP_SAI_N 3
// Frequencias da mola (rad/s): abre em ~420 ms, fecha em ~300 ms (95%).
#define EXP_W_ABRE  11.0f
#define EXP_W_FECHA 15.5f

typedef struct { int r, c; float p, v; } ExpCartao;
typedef struct {
  ExpCartao abre;              // o do foco (r = -1: nenhum)
  unsigned desde;              // ms em que o foco pousou nele
  ExpCartao sai[EXP_SAI_N];    // fechando (r = -1: livre)
} ExpEstado;

static inline void exp_zerar(ExpEstado *e) {
  int i;
  e->abre.r = e->abre.c = -1; e->abre.p = e->abre.v = 0.0f; e->desde = 0;
  for (i = 0; i < EXP_SAI_N; i++) { e->sai[i].r = e->sai[i].c = -1; e->sai[i].p = e->sai[i].v = 0.0f; }
}

// Passa o cartao do foco para os que fecham (se estava aberto de fato). Sem
// vaga, sai o que estiver mais fechado — o salto que sobra e o menor possivel.
static inline void exp_soltar(ExpEstado *e) {
  int i, k = -1;
  if (e->abre.r < 0 || e->abre.p <= 0.003f) { e->abre.p = e->abre.v = 0.0f; return; }
  for (i = 0; i < EXP_SAI_N; i++) if (e->sai[i].r < 0) { k = i; break; }
  if (k < 0) { k = 0; for (i = 1; i < EXP_SAI_N; i++) if (e->sai[i].p < e->sai[k].p) k = i; }
  e->sai[k] = e->abre;
  e->abre.p = e->abre.v = 0.0f;
}

// Um quadro. (r, c) = o cartaz em foco; `pode` = ele pode abrir (cartaz em
// pe, opcao ligada...); `pausado` = menu do cartaz ou OK segurado: o que ja
// abriu fica, o que nao abriu espera, e o atraso recomeca quando soltar.
static inline void exp_passo(ExpEstado *e, int r, int c, int pode, int pausado,
                             unsigned agora, unsigned atrasoMs, float dt, int reduzido) {
  int i;
  if (!pode) { r = c = -1; }
  if (r != e->abre.r || c != e->abre.c) {
    exp_soltar(e);
    e->abre.r = r; e->abre.c = c; e->abre.p = e->abre.v = 0.0f;
    e->desde = agora;
    // Voltou a um cartao que ainda fechava: continua de onde ele estava.
    for (i = 0; i < EXP_SAI_N; i++)
      if (r >= 0 && e->sai[i].r == r && e->sai[i].c == c) {
        e->abre = e->sai[i];
        e->sai[i].r = e->sai[i].c = -1; e->sai[i].p = e->sai[i].v = 0.0f;
      }
  }
  if (e->abre.r >= 0) {
    if (pausado) {
      if (e->abre.p < 0.999f) e->desde = agora;
    } else if (e->desde && agora - e->desde >= atrasoMs) {
      e->abre.p = reduzido ? 1.0f : anim_mola2_tempo(&e->abre.v, e->abre.p, 1.0f, dt, EXP_W_ABRE);
    }
  }
  for (i = 0; i < EXP_SAI_N; i++) {
    if (e->sai[i].r < 0) continue;
    e->sai[i].p = reduzido ? 0.0f : anim_mola2_tempo(&e->sai[i].v, e->sai[i].p, 0.0f, dt, EXP_W_FECHA);
    if (e->sai[i].p <= 0.003f) { e->sai[i].r = e->sai[i].c = -1; e->sai[i].p = e->sai[i].v = 0.0f; }
  }
}

// Abertura propria do cartao (r, c), 0..1 (o do foco ou um que fecha).
static inline float exp_propria(const ExpEstado *e, int r, int c) {
  float p = 0.0f;
  int i;
  if (e->abre.r == r && e->abre.c == c) p += e->abre.p;
  for (i = 0; i < EXP_SAI_N; i++) if (e->sai[i].r == r && e->sai[i].c == c) p += e->sai[i].p;
  return p > 1.0f ? 1.0f : p;
}

// Quanto o cartao (r, c) e empurrado para a direita: a soma das aberturas dos
// cartoes ANTES dele na mesma fileira, cada uma vezes a largura que ela ganha
// aberta (unidade(coluna)). `unidade` pode ser NULL: largura constante `un`.
typedef float (*ExpUnidade)(int r, int c, void *ctx);
static inline float exp_empurra(const ExpEstado *e, int r, int c, ExpUnidade unidade, void *ctx, float un) {
  float d = 0.0f;
  int i;
  if (e->abre.r == r && e->abre.c >= 0 && e->abre.c < c && e->abre.p > 0.0f)
    d += (unidade ? unidade(r, e->abre.c, ctx) : un) * e->abre.p;
  for (i = 0; i < EXP_SAI_N; i++)
    if (e->sai[i].r == r && e->sai[i].c >= 0 && e->sai[i].c < c && e->sai[i].p > 0.0f)
      d += (unidade ? unidade(r, e->sai[i].c, ctx) : un) * e->sai[i].p;
  return d;
}

// Ha algum cartao aberto ou fechando nesta fileira?
static inline int exp_fileira_ativa(const ExpEstado *e, int r) {
  int i;
  if (e->abre.r == r && e->abre.p > 0.0f) return 1;
  for (i = 0; i < EXP_SAI_N; i++) if (e->sai[i].r == r) return 1;
  return 0;
}

// Peso da arte DEITADA sobre o cartaz, pela abertura: 0 ate 30%, cruza ate 75%
// (suave nas duas pontas), 1 dali em diante. O cartaz fica por baixo enquanto
// o peso nao chega a 1 — nunca um quadro sem arte.
static inline float exp_mistura_arte(float p) {
  return anim_suave((p - 0.30f) / 0.45f);
}

// Hora de pedir a arte deitada (na largura FINAL do cartao aberto): metade do
// atraso parado, entre 200 e 600 ms. Antes disso o foco ainda esta passando.
static inline int exp_pre_pedir(const ExpEstado *e, unsigned agora, unsigned atrasoMs) {
  unsigned m = atrasoMs / 2;
  if (m < 200) m = 200;
  if (m > 600) m = 600;
  return e->abre.r >= 0 && e->desde && agora - e->desde >= m;
}
#endif
