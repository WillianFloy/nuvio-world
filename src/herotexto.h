#ifndef HEROTEXTO_H
#define HEROTEXTO_H
// TRANSICAO ESCALONADA DO TEXTO DO DESTAQUE (2.0.42, estilo "streaming"): logo/titulo ->
// meta (legenda, amigos, selos, nota) -> sinopse, em vez de o bloco inteiro trocar de
// uma vez. So POSICAO (Y) e OPACIDADE de texto/logo ja em cache: nenhuma camada de
// tela cheia e nenhuma textura nova por quadro (o alfa nao entra na chave do cache de
// texto; ver HT_PISO).
//
// Linha do tempo, em ms desde a troca do destaque (aprovada pelo dono):
//   saida  : o bloco ANTIGO inteiro apaga e sobe HT_SOBE_PX em HT_SAIDA_MS;
//   entrada: cada elemento novo leva HT_ENTRA_MS, o 1o comecando em HT_ATRASO_MS (a
//            saida esta no fim) e cada seguinte HT_ESCALA_MS depois, entrando de
//            baixo (HT_ENTRA_PX). Fim: 100 + 2*60 + 300 = 520 ms.
// Funcoes puras (sem GL), testadas em tests/herotexto.sh.
#define HT_N          3       // 0 logo/titulo, 1 meta, 2 sinopse
#define HT_SAIDA_MS   150.0f
#define HT_ENTRA_MS   300.0f
#define HT_ATRASO_MS  100.0f
#define HT_ESCALA_MS  60.0f
#define HT_TOTAL_MS   (HT_ATRASO_MS + HT_ESCALA_MS * (float)(HT_N - 1) + HT_ENTRA_MS)
#define HT_SOBE_PX    12.0f
#define HT_ENTRA_PX   16.0f
// Texto com alfa ~0 NAO e rasterizado (text.c: com alpha 0 os blocos so medem). Para o
// bloco novo ja ter as linhas prontas quando comecar a acender — em vez de rasterizar no
// meio da animacao —, ele e desenhado desde o quadro da troca com este alfa minimo
// (invisivel: ~1/255), o mesmo piso que a Vitrine ja usa.
#define HT_PISO       0.006f

static inline float ht_clamp01(float p) { return p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p; }
static inline float ht_ease(float p) { p = ht_clamp01(p); return p * p * (3.0f - 2.0f * p); }

// Progresso 0..1 do elemento `i` que ENTRA, `t` ms apos a troca.
static inline float ht_entra_p(int i, float t) {
  return ht_clamp01((t - HT_ATRASO_MS - HT_ESCALA_MS * (float)i) / HT_ENTRA_MS);
}
static inline float ht_entra_alfa(int i, float t) { return ht_ease(ht_entra_p(i, t)); }
static inline float ht_entra_dy(int i, float t) { return HT_ENTRA_PX * (1.0f - ht_ease(ht_entra_p(i, t))); }

// O bloco ANTIGO: nasce do ponto em que o novo estava quando uma nova troca o interrompeu
// (`a0`, `dy0` por elemento; 1 e 0 num bloco assentado) e apaga/sobe em HT_SAIDA_MS. `t` ms
// apos a troca que o substituiu.
typedef struct { float a[HT_N], dy[HT_N]; } HtInstante;
static inline HtInstante ht_instante(float t) {   // o bloco novo no tempo t (para guardar)
  HtInstante s;
  for (int i = 0; i < HT_N; i++) { s.a[i] = ht_entra_alfa(i, t); s.dy[i] = ht_entra_dy(i, t); }
  return s;
}
static inline float ht_saida_alfa(const HtInstante *ini, int i, float t) {
  return ini->a[i] * (1.0f - ht_ease(t / HT_SAIDA_MS));
}
static inline float ht_saida_dy(const HtInstante *ini, int i, float t) {
  float e = ht_ease(t / HT_SAIDA_MS);
  return ini->dy[i] * (1.0f - e) - HT_SOBE_PX * e;
}
// A troca acabou (nada mais a animar): saida concluida e todos os elementos assentados.
static inline int ht_acabou(float t) { return t >= HT_TOTAL_MS; }

// VITRINE (Collections): a mesma ideia DENTRO da janela de 350 ms da pilha de camadas
// (vitHeroEntra: 0..1). O bloco entra a partir de 30% e cada elemento comeca 12% depois do
// anterior, todos terminando em 100%: logo 30-100%, meta 42-100%, sinopse 54-100%.
#define HT_VIT_INI   0.30f
#define HT_VIT_ESC   0.12f
static inline float ht_vit_entra_de(int i, float p, float base) {
  float ini = base + HT_VIT_ESC * (float)i;
  return ht_ease((ht_clamp01(p) - ini) / (1.0f - ini));
}
static inline float ht_vit_entra(int i, float p) { return ht_vit_entra_de(i, p, HT_VIT_INI); }
#endif
