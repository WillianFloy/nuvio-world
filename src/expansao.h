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
// Frequencias da mola (rad/s): abre em ~560 ms, fecha em ~400 ms (95%).
// 21/11 (dono, na TCL): "melhorou, mas da pra ser mais suave" — eram 11 e 15,5
// (~440 e ~320 ms).
#define EXP_W_ABRE  8.5f
#define EXP_W_FECHA 12.0f

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

// A TROCA DE ARTE (21/11, dono: "a troca do poster vertical pro horizontal e
// muito perceptivel, tem que ser uma transicao perfeita"). Antes o cartaz era
// esticado em "cover" na moldura que alarga (o corte mudava a cada quadro) e a
// deitada cruzava por cima entre 30% e 75%. Agora:
//   - o CARTAZ fica parado, no tamanho e no lugar em que estava fechado (nada
//     de corte mudando), e se dissolve entre 10% e 50%;
//   - a DEITADA ocupa a moldura toda e aparece por baixo desde o inicio,
//     inteira aos 35% — o que a moldura ganha ja nasce com ela.
// Em 0 o desenho e exatamente o cartao fechado; de 50% em diante, so a
// deitada. Fechar e o mesmo caminho ao contrario (mesma funcao de p).
static inline float exp_alfa_deitada(float p) { return anim_suave(p / 0.35f); }
static inline float exp_alfa_cartaz(float p) { return 1.0f - anim_suave((p - 0.10f) / 0.40f); }
// A partir daqui so a deitada aparece: ela pode virar a arte principal.
#define EXP_SO_DEITADA 0.50f
// Compatibilidade (testes e quem ainda pede o peso da deitada).
static inline float exp_mistura_arte(float p) { return exp_alfa_deitada(p); }

// A ARTE DEITADA PREENCHE A MOLDURA (cover) EM TODO O CAMINHO (World, 21/11).
// Com a deitada como arte principal (p >= EXP_SO_DEITADA, abrindo ou fechando)
// o GFX_CARD caia no "contain" enquanto a moldura ainda nao tinha proporcao
// perto da arte (ra < 0,80, gfx.c): a imagem encolhia para dentro do cartao com
// a propria arte escurecida em volta, e voltava a cover perto do fim. Era o
// "bug" da troca de arte no video da TCL, na entrada e na saida. Na camada por
// baixo (p < 0,5) ela ja era cover; agora e cover dos dois lados da troca.
static inline int exp_arte_cobre(float p, int deitadaPrincipal) {
  return deitadaPrincipal && p > 0.0f;
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
