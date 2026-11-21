// O ANEL DE FOCO ACOMPANHA O CARTAZ (World, 21/11; fotos da TCL dentro das
// Collections: "o contorno do foco fica meio cortado").
//
// O anel e um bloco solido (gfx_cor) NV_ANEL_FOCO px maior que o cartaz, atras
// dele; o que aparece e a faixa entre os dois. Aqui o desenho e avaliado com o
// MESMO sdf do shader (FS_SDF em gfx.c, raio pela altura do rect) e a faixa
// visivel e medida em pixels: nos lados e na diagonal do canto ela tem de ter a
// espessura do anel, em cartaz em pe, deitado, aberto pela expansao e com o
// raio grande dos Ajustes (que bate no teto de meio lado).
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "focoprof.h"

// FS_SDF, em pixels (o shader trabalha em uv com a altura = 1).
static float sdfPx(GfxRect r, float raio, float x, float y) {
  float asp = r.w / r.h, u = (x - r.x) / r.h, v = (y - r.y) / r.h;
  float px = (u - 0.5f * asp), py = v - 0.5f;
  float qx = fabsf(px) - (0.5f * asp - raio), qy = fabsf(py) - (0.5f - raio);
  float fora = hypotf(fmaxf(qx, 0.0f), fmaxf(qy, 0.0f));
  return (fminf(fmaxf(qx, qy), 0.0f) + fora - raio) * r.h;
}
// Raio da ARTE como home.c (raioDe) e vertudo.c (raioCard): px pela altura, teto meio lado.
static float raioArte(float w, float h, float px) {
  float r = px / h, teto = 0.5f * w / h;
  if (r > 0.5f) r = 0.5f;
  return r > teto ? teto : r;
}
// Espessura visivel do anel ao longo de uma semirreta saindo do centro do cartaz
// na direcao (dx, dy): da borda da arte ate a borda do bloco do anel.
static float faixa(GfxRect c, float raio, float raioAnel, float dx, float dy) {
  GfxRect a = { c.x - NV_ANEL_FOCO, c.y - NV_ANEL_FOCO, c.w + 2 * NV_ANEL_FOCO, c.h + 2 * NV_ANEL_FOCO };
  float cx = c.x + c.w * 0.5f, cy = c.y + c.h * 0.5f, n = hypotf(dx, dy), t, tArte = -1, tAnel = -1;
  dx /= n; dy /= n;
  for (t = 0.0f; t < c.w + c.h; t += 0.01f) {
    float x = cx + dx * t, y = cy + dy * t;
    if (tArte < 0 && sdfPx(c, raio, x, y) > 0.0f) tArte = t;
    if (tAnel < 0 && sdfPx(a, raioAnel, x, y) > 0.0f) tAnel = t;
    if (tArte >= 0 && tAnel >= 0) break;
  }
  assert(tArte >= 0 && tAnel >= 0);
  return tAnel - tArte;   // negativo: a arte passa do anel (canto sem contorno)
}
// `wFechado`: a largura do cartaz FECHADO. A Vitrine pedia o raio do anel como
// px / menor lado da caixa fechada, e foco_anel o tomava como fracao do menor
// lado da caixa desenhada; `antes` reproduz essa conta.
static void caso(const char *nome, float w, float h, float raioPx, float wFechado) {
  GfxRect c = { 100, 100, w, h };
  float raio = raioArte(w, h, raioPx), ra = foco_anel_raio(c, raio);
  // diagonal que passa pelo centro do arco do canto de cima a direita
  float rpx = raio * h, dx = w * 0.5f - rpx + rpx * 0.70710678f, dy = -(h * 0.5f - rpx + rpx * 0.70710678f);
  float lado = faixa(c, raio, ra, 1, 0), topo = faixa(c, raio, ra, 0, -1), canto = faixa(c, raio, ra, dx, dy);
  // a regra antiga, so para registrar o que se via na TV
  float menor = w < h ? w : h, rVelho = raioPx / (wFechado < h ? wFechado : h);
  float antigo = faixa(c, raio, (rVelho * menor + NV_ANEL_FOCO) / (menor + 2 * NV_ANEL_FOCO), dx, dy);
  printf("%-22s %4.0fx%-4.0f raio %2.0f px: lado %.2f topo %.2f canto %.2f (antes: canto %.2f)\n",
         nome, w, h, raioPx, lado, topo, canto, antigo);
  assert(fabsf(lado - NV_ANEL_FOCO) < 0.05f);
  assert(fabsf(topo - NV_ANEL_FOCO) < 0.05f);
  assert(fabsf(canto - NV_ANEL_FOCO) < 0.5f);   // concentrico: a mesma espessura no canto
}
int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  caso("cartaz em pe", 214, 322, 24, 214);
  caso("cartaz em pe, raio 40", 214, 322, 40, 214);
  caso("deitado", 573, 322, 24, 573);
  caso("aberto pela expansao", 573, 322, 24, 214);
  caso("pasta quadrada", 300, 300, 24, 300);
  caso("raio no teto", 120, 300, 90, 120);   // raio maior que meio lado: o teto da arte vale para o anel
  caso("raio zero", 214, 322, 0, 214);
  puts("anel_foco: ok");
  return 0;
}
