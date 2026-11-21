// FOCO COMUM DOS CARTOES (Home e Detalhe): a mesma regra, os mesmos Ajustes.
//
//  - Anel ligado (Ajustes > Foco no cartaz): o cartao mantem a caixa e leva o
//    anel de NV_ANEL_FOCO no realce (ou o cartao de vidro). Desligado: nenhum
//    contorno, e quem marca o foco e o crescimento (FOCO_ZOOM).
//  - Profundidade (AJ_PROF + AJ_PROF_BRILHO): o reflexo que passa e o clarear
//    do cartao em foco, no proprio GFX_CARD (gfx.c), em todo tipo de cartao.
// `f` e a animacao de foco 0..1 que cada tela ja tem; nada aqui guarda estado.
#ifndef NV_FOCOPROF_H
#define NV_FOCOPROF_H
#include "gfx.h"
#include "ajustes.h"
#include "layout.h"

#define FOCO_ZOOM 0.06f   // o mesmo 6 % da Home com o anel desligado

// Caixa do cartao em foco: cresce em volta do centro so com o anel desligado.
static inline GfxRect foco_zoom(GfxRect r, float f) {
  float s;
  if (ajustes_borda_foco() || f <= 0.0f) return r;
  s = FOCO_ZOOM * f;
  return (GfxRect){ r.x - r.w * s * 0.5f, r.y - r.h * s * 0.5f,
                    r.w * (1.0f + s), r.h * (1.0f + s) };
}

// Raio do ANEL concentrico com o cartao `r` de raio `raio`: o canto de fora e
// o de dentro (em px) mais a espessura, na convencao do gfx_cor — fracao da
// ALTURA do rect (sdf em gfx.c: p = (uv - 0.5) * (asp, 1)). Teto do canto =
// metade do menor lado, como raioDe/raioCard.
static inline float foco_anel_raio(GfxRect r, float raio) {
  float px = raio * r.h, teto = 0.5f * (r.w < r.h ? r.w : r.h);
  if (px > teto) px = teto;
  if (px < 0.0f) px = 0.0f;
  return (px + NV_ANEL_FOCO) / (r.h + 2 * NV_ANEL_FOCO);
}
// Anel de foco. `raio` na convencao do gfx_cor: fracao da ALTURA de r.
// World 21/11: era a fracao do MENOR lado. Num cartaz em pe (menor = largura) o
// gfx_cor le o mesmo numero pela altura e o canto do anel saia ~1,5x mais
// redondo que o da arte; a arte passava por cima do anel nos cantos e o
// contorno parecia CORTADO (fotos da TCL, dentro das Collections).
static inline void foco_anel(GfxRect r, float raio, float f, float a) {
  float ar, ag, ab;
  if (f <= 0.01f || !ajustes_borda_foco()) return;
  if (ajustes_vidro()) { gfx_vidro_cartao(r, raio, f, a); return; }
  ajustes_acento(&ar, &ag, &ab);
  gfx_cor((GfxRect){ r.x - NV_ANEL_FOCO, r.y - NV_ANEL_FOCO,
                     r.w + 2 * NV_ANEL_FOCO, r.h + 2 * NV_ANEL_FOCO },
          foco_anel_raio(r, raio), ar, ag, ab, f * a);
}

#endif
