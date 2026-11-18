// TRANSFORMACAO HOME -> COLLECTION (2.0.45). Relogio unico e adaptativo, puro (sem estado global, sem GL):
// testavel sozinho (tests/transicao.c). Substitui o voo de tempo fixo da 2.0.44 quando ha cartao de origem.
//
// Um progresso `p` (0..1) conduz a geometria e o fundo; `r` (0..1) conduz a revelacao do conteudo.
//   SAIDA     0 .. 300 ms   p 0 -> TR_P_SAIDA, linear no tempo (as curvas de cada elemento dao a forma);
//                           o cartao chega ao retangulo do destaque da Vitrine em TR_P_SAIDA.
//   CRUZEIRO  0 .. 400 ms   enquanto o conteudo nao esta pronto: p se aproxima de TR_P_CRUZ_MAX com a
//                           velocidade caindo exponencialmente — desacelera, NUNCA para (dono, 17/11).
//   REVELA    300 ms        p -> 1 (Hermite, comeca com a velocidade que tinha: sem degrau) e r 0 -> 1.
// O portao (`pronto`) so e lido a partir do fim da SAIDA; passado TR_TETO_MS - TR_REVELA_MS a revelacao
// comeca de qualquer jeito (teto de 1000 ms), e o que faltar entra cartaz a cartaz com fade.
// Tudo pronto no clique: 600 ms. Nada aqui sabe de tela: so numeros.
#ifndef NV_TRANSICAO_H
#define NV_TRANSICAO_H
#include <math.h>

#define TR_SAIDA_MS    300.0f
#define TR_REVELA_MS   300.0f
#define TR_TETO_MS    1000.0f
#define TR_P_SAIDA       0.45f
#define TR_P_CRUZ_MAX    0.62f
#define TR_V_SAIDA     (TR_P_SAIDA / TR_SAIDA_MS)                 // p por ms na saida (= entrada do cruzeiro)
#define TR_TAU_MS      ((TR_P_CRUZ_MAX - TR_P_SAIDA) / TR_V_SAIDA) // ~113 ms: constante da desaceleracao

enum { TR_PARADO = 0, TR_SAIDA, TR_CRUZEIRO, TR_REVELA, TR_FIM };

typedef struct {
  int fase;
  float ms;          // desde o inicio
  float p, v;        // progresso e velocidade (p por ms)
  float r;           // revelacao 0..1 (0 antes da fase REVELA)
  float tCruz;       // ms no cruzeiro
  float tRev;        // ms na revelacao
  float pRev, vRev;  // estado na entrada da revelacao (Hermite)
  float msRevela;    // quando a revelacao comecou (diagnostico/teste); -1 = ainda nao
} TrRelogio;

static inline float tr_clamp01(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
// Suave nas duas pontas (smoothstep), para as entradas/saidas de cada elemento.
static inline float tr_suave(float x) { x = tr_clamp01(x); return x * x * (3.0f - 2.0f * x); }
// Saida rapida, chegada macia (cubica): a geometria do cartao.
static inline float tr_saida(float x) { x = tr_clamp01(x); float u = 1.0f - x; return 1.0f - u * u * u; }

static inline void tr_iniciar(TrRelogio *c) {
  c->fase = TR_SAIDA; c->ms = 0.0f; c->p = 0.0f; c->v = TR_V_SAIDA; c->r = 0.0f;
  c->tCruz = c->tRev = 0.0f; c->pRev = c->vRev = 0.0f; c->msRevela = -1.0f;
}
static inline void tr_parar(TrRelogio *c) { c->fase = TR_PARADO; c->p = 1.0f; c->r = 1.0f; c->v = 0.0f; }
static inline int  tr_ativo(const TrRelogio *c) { return c->fase != TR_PARADO && c->fase != TR_FIM; }

static inline void tr_entrar_revela(TrRelogio *c) {
  c->fase = TR_REVELA; c->tRev = 0.0f; c->pRev = c->p; c->vRev = c->v; c->msRevela = c->ms;
}

// Um quadro. `pronto` = o portao do conteudo abriu (so conta depois da saida).
static inline void tr_passo(TrRelogio *c, float dtms, int pronto) {
  if (!tr_ativo(c) || dtms <= 0.0f) return;
  c->ms += dtms;
  if (c->fase == TR_SAIDA) {
    if (c->ms < TR_SAIDA_MS) { c->p = TR_V_SAIDA * c->ms; c->v = TR_V_SAIDA; return; }
    float sobra = c->ms - TR_SAIDA_MS;
    c->p = TR_P_SAIDA; c->v = TR_V_SAIDA; c->fase = TR_CRUZEIRO; c->tCruz = 0.0f;
    if (pronto || c->ms >= TR_TETO_MS - TR_REVELA_MS) { tr_entrar_revela(c); dtms = sobra; }
    else dtms = sobra;
  }
  if (c->fase == TR_CRUZEIRO) {
    if (pronto || c->ms >= TR_TETO_MS - TR_REVELA_MS) { tr_entrar_revela(c); }
    else {
      c->tCruz += dtms;
      float e = expf(-c->tCruz / TR_TAU_MS);
      c->p = TR_P_CRUZ_MAX - (TR_P_CRUZ_MAX - TR_P_SAIDA) * e;
      c->v = TR_V_SAIDA * e;   // sempre > 0: desacelera sem congelar
      return;
    }
    dtms = 0.0f;   // a revelacao comeca neste quadro, do ponto em que o cruzeiro estava
  }
  if (c->fase == TR_REVELA) {
    c->tRev += dtms;
    float s = tr_clamp01(c->tRev / TR_REVELA_MS), T = TR_REVELA_MS;
    float s2 = s * s, s3 = s2 * s;
    // Hermite: p(0) = pRev, p'(0) = vRev, p(1) = 1, p'(1) = 0. Monotona porque vRev*T <= 3*(1 - pRev).
    c->p = (2*s3 - 3*s2 + 1) * c->pRev + (s3 - 2*s2 + s) * T * c->vRev + (-2*s3 + 3*s2);
    c->v = ((6*s2 - 6*s) * c->pRev + (3*s2 - 4*s + 1) * T * c->vRev + (-6*s2 + 6*s)) / T;
    c->r = s;
    if (s >= 1.0f) { c->fase = TR_FIM; c->p = 1.0f; c->v = 0.0f; c->r = 1.0f; }
  }
}

// --- O que cada elemento faz com p e r -----------------------------------------------------------
// Geometria do cartao: 0 na origem, 1 no retangulo do destaque (chega em TR_P_SAIDA).
static inline float tr_cartao(float p) { return tr_saida(p / TR_P_SAIDA); }
// Escala do destaque depois da chegada: 1,02 -> 1,00 (assenta devagar enquanto espera o conteudo).
#define TR_ESCALA_CHEGADA 1.02f
static inline float tr_escala(float p) {
  return TR_ESCALA_CHEGADA - (TR_ESCALA_CHEGADA - 1.0f) * tr_suave((p - TR_P_SAIDA) / (1.0f - TR_P_SAIDA));
}
// Capa do cartao -> banner da pasta (a mesma textura do fundo do hero da Home).
static inline float tr_banner(float p) { return tr_suave((tr_cartao(p) - 0.15f) / 0.50f); }
// O cartao passa o lugar a camada de destaque da Vitrine (mesma imagem, mesmo retangulo; muda o veu) no
// comeco da REVELACAO (r 0 -> 0,45, ~135 ms), junto com a entrada das fileiras. Enquanto espera o conteudo
// so o cartao esta em cena (uma camada de tela cheia, nao duas).
static inline float tr_cartao_alfa(float r) { return 1.0f - tr_suave(r / 0.45f); }
// Home recua (escala 1 -> 0,97 e opacidade) durante a saida; o texto do hero apaga primeiro.
#define TR_HOME_ESCALA 0.03f
static inline float tr_home_recuo(float p) { return tr_suave(p / TR_P_SAIDA); }
static inline float tr_home_texto(float p) { return 1.0f - tr_suave(p / 0.25f); }
// Fundo escuro da Vitrine (so quando o destaque nao cobre a tela inteira: Padrao).
static inline float tr_fundo(float p) { return tr_suave((p - 0.15f) / 0.30f); }
// Cabecalho: entra durante o movimento (nao depende dos cartazes).
static inline float tr_cabecalho(float p) { return tr_suave((p - 0.40f) / 0.20f); }
// Fileiras: em cascata dentro da revelacao (40 ms entre fileiras ate a 4a, 180 ms cada).
#define TR_LIN_PASSO_MS 40.0f
#define TR_LIN_MS      180.0f
#define TR_LIN_MAX        3
#define TR_LIN_PX       22.0f   // assentamento aprovado na 2.0.43
static inline float tr_fileira(int k, float r) {
  if (k > TR_LIN_MAX) k = TR_LIN_MAX;
  return tr_suave((r * TR_REVELA_MS - TR_LIN_PASSO_MS * (float)k) / TR_LIN_MS);
}
#endif
