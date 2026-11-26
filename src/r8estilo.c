// Ver r8estilo.h.
#include "r8estilo.h"
#include "layout.h"
#include "escala.h"

static float raioNorm(GfxRect r, float raioPx) { return r.h > 0.0f ? raioPx / r.h : 0.0f; }

void r8_superficie_cor(float *r, float *g, float *b, float *a) {
  if (gfx_tema_preto) { *r = *g = *b = 0.0f; *a = 1.0f; return; }
  *r = 14.0f / 255.0f; *g = 14.0f / 255.0f; *b = 16.0f / 255.0f; *a = R8_SUPERFICIE_A;
}

void r8_sombra(GfxRect r, float raioPx, float a) { r8_sombra_desl(r, raioPx, a, 0.0f, R8_SOMBRA_DY); }
void r8_sombra_desl(GfxRect r, float raioPx, float a, float dx, float dy) {
  // box-shadow dx dy 80: a mancha vai 80 px para fora (deslocada 30 para baixo)
  // e cai em ~110 px a partir da borda dela; na borda da peca fica ~0,5 do alfa,
  // como o desfoque gaussiano do CSS.
  const float b = R8_SOMBRA_BLUR;
  GfxRect s = { r.x - b + dx, r.y - b + dy, r.w + 2.0f * b, r.h + 2.0f * b };
  float cr, cg, cb, ca;
  r8_superficie_cor(&cr, &cg, &cb, &ca);
  gfx_sombra_sob(s, 1.0f, b * 1.4f, raioNorm(s, raioPx + b), 0, 0, 0, R8_SOMBRA_A * a,
                 r, raioPx, ca * a);
}

void r8_material(GfxRect r, float raioPx, float a) { r8_material_sombra(r, raioPx, a, 0.0f, R8_SOMBRA_DY); }
void r8_material_sombra(GfxRect r, float raioPx, float a, float dx, float dy) {
  float cr, cg, cb, ca;
  if (a <= 0.0f || r.w <= 0.0f || r.h <= 0.0f) return;
  r8_sombra_desl(r, raioPx, a, dx, dy);
  r8_superficie_cor(&cr, &cg, &cb, &ca);
  gfx_cor(r, raioNorm(r, raioPx), cr, cg, cb, ca * a);
  gfx_anel_fora(r, raioNorm(r, raioPx), 0.0f, R8_FIO_PX, 1, 1, 1, R8_FIO_A * a);
}

void r8_foco_acao(GfxRect r, float raioPx, float a) {
  if (a <= 0.0f) return;
  gfx_cor(r, raioNorm(r, raioPx), R8_OSSO, R8_FOCO_VEU_A * a);
  gfx_anel(r, raioNorm(r, raioPx), R8_FOCO_ACAO_PX, R8_OSSO, a);
}

void r8_foco_linha(GfxRect r, float a) {
  if (a <= 0.0f) return;
  gfx_cor(r, raioNorm(r, R8_RAIO_LINHA), R8_OSSO, R8_FOCO_LINHA_VEU * a);
  gfx_anel(r, raioNorm(r, R8_RAIO_LINHA), R8_FOCO_LINHA_PX, R8_OSSO, R8_FOCO_LINHA_A * a);
}

// Sem um modo de degrade lateral no gfx: faixas de 4 px, cada uma na forca do
// seu meio (o passo de 0,8 / 70 fica abaixo de 1 nivel de 8 bits no escuro).
void r8_escuro_lateral(float x0, float x1, float forca) {
  float w = x1 - x0, x;
  int n, i;
  if (forca <= 0.0f || w <= 1.0f) return;
  n = (int)(w / 4.0f + 0.999f);
  for (i = 0; i < n; i++) {
    float a = x0 + w * (float)i / (float)n, b = x0 + w * (float)(i + 1) / (float)n;
    x = (a + b) * 0.5f;
    gfx_cor((GfxRect){ a, 0.0f, b - a, NV_TELA_H }, 0.0f, 0, 0, 0, forca * (x - x0) / w);
  }
}

void r8_foco_conteudo(GfxRect r, float raioPx, float a) {
  if (a <= 0.0f) return;
  gfx_anel(r, raioNorm(r, raioPx), R8_FOCO_CONT_PX, R8_OSSO, a);
}

void r8_veu(float forca) {
  if (forca <= 0.0f) return;
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, forca);
}
