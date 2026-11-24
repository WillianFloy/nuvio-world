// Geometria do menu de segurar OK (Folha e Desdobrar), pura: bash tests/ctxgeo.sh
#include "ctxgeo.h"
#include <stdio.h>
static int falhas, checagens;
static void ok(int c, const char *m) { checagens++; if (!c) { falhas++; if (falhas < 20) printf("FALHA: %s\n", m); } }
static int dentro(GfxRect r, float W, float H) { return r.x >= -0.01f && r.y >= -0.01f && r.x + r.w <= W + 0.01f && r.y + r.h <= H + 0.01f; }
static int cruza(GfxRect a, GfxRect b) { return a.x < b.x + b.w - 0.01f && b.x < a.x + a.w - 0.01f && a.y < b.y + b.h - 0.01f && b.y < a.y + a.h - 0.01f; }
int main(void) {
  const float W = 1920, H = 1080;
  CtxFolhaGeo f; CtxDesdGeo d;
  GfxRect it;
  float x, y, w, h;
  // Folha: lado oposto, dentro da tela, nunca sobre o item
  it = (GfxRect){ 100, 500, 212, 318 }; ctxgeo_folha(&it, W, H, &f);
  ok(f.lado == 1 && f.folha.x + f.folha.w <= W - CTXG_MARGEM + 0.01f, "folha: item a esquerda -> folha a direita");
  it = (GfxRect){ 1600, 500, 212, 318 }; ctxgeo_folha(&it, W, H, &f);
  ok(f.lado == -1 && f.folha.x == CTXG_MARGEM, "folha: item a direita -> folha a esquerda");
  ctxgeo_folha(NULL, W, H, &f);
  ok(f.lado == 1, "folha: sem item, a direita");
  ok(f.arte.x == f.folha.x && f.arte.w == f.folha.w && f.arte.h > 300.0f && f.arte.h < f.folha.h, "folha: a arte e a faixa de cima");
  for (w = 140; w <= 700; w += 37)
    for (h = 140; h <= 520; h += 41)
      for (x = 40; x + w <= W - 40; x += 23)
        for (y = 60; y + h <= H - 30; y += 97) {
          it = (GfxRect){ x, y, w, h }; ctxgeo_folha(&it, W, H, &f);
          ok(dentro(f.folha, W, H), "folha: dentro da tela");
          if (w <= 520.0f) ok(!cruza(f.folha, it), "folha: nao cobre o item");
        }
  // Desdobrar
  it = (GfxRect){ 300, 520, 224, 336 }; ctxgeo_desdobrar(it, 300, 0, 36, 480, W, H, &d);
  ok(d.lado == 1 && d.painel.x == it.x + it.w && d.cont.x == it.x && d.cont.w == it.w + CTXG_DESD_W, "desdobrar: abre para a direita colado ao item");
  ok(d.empurra == CTXG_DESD_W && d.faixa.x == it.x + it.w && d.faixa.x + d.faixa.w == W, "desdobrar: a faixa a direita desliza a largura do painel");
  it = (GfxRect){ 1500, 520, 224, 336 }; ctxgeo_desdobrar(it, 300, 0, 36, 480, W, H, &d);
  ok(d.lado == -1 && d.painel.x + d.painel.w == it.x && d.faixa.x == 0 && d.faixa.w == it.x, "desdobrar: na borda direita abre para a esquerda");
  it = (GfxRect){ 150, 420, 276, 419 }; ctxgeo_desdobrar(it, 300, 1, 36, 480, W, H, &d);
  ok(d.empurra == 0.0f && d.painel.x == it.x + it.w + 36 && d.painel.w == 480, "desdobrar: no Palco o painel toma a ficha e nada desliza");
  it = (GfxRect){ 300, 760, 224, 236 }; ctxgeo_desdobrar(it, 420, 0, 36, 480, W, H, &d);
  ok(d.cont.y + d.cont.h <= H - CTXG_MARGEM + 0.01f && d.cont.h == 420, "desdobrar: conteudo mais alto sobe para caber");
  for (w = 140; w <= 520; w += 29)
    for (h = 140; h <= 480; h += 43)
      for (x = 40; x + w <= W - 40; x += 31)
        for (y = 60; y + h <= H - 40; y += 89) {
          it = (GfxRect){ x, y, w, h }; ctxgeo_desdobrar(it, 330, 0, 36, 480, W, H, &d);
          ok(!cruza(d.painel, it), "desdobrar: o painel nao cobre o item");
          ok(d.cont.x <= it.x + 0.01f && d.cont.x + d.cont.w >= it.x + it.w - 0.01f, "desdobrar: a superficie contem o item");
          if (x + w + CTXG_DESD_W <= W - CTXG_MARGEM || x - CTXG_DESD_W >= CTXG_MARGEM)
            ok(d.painel.x >= CTXG_MARGEM - 0.01f && d.painel.x + d.painel.w <= W - CTXG_MARGEM + 0.01f, "desdobrar: painel dentro da margem quando cabe");
        }
  // curvas
  ok(ctxgeo_dec(0) == 0.0f && ctxgeo_dec(1) == 1.0f && ctxgeo_dec(0.5f) > 0.5f, "dec: desacelera");
  { float a = -1, t; int mono = 1; for (t = 0; t <= 1.0f; t += 0.01f) { float v = ctxgeo_fase(t, .2f, .7f); if (v < a) mono = 0; a = v; } ok(mono, "fase: monotona"); }
  ok(ctxgeo_fase(.1f, .2f, .7f) == 0.0f && ctxgeo_fase(.8f, .2f, .7f) == 1.0f, "fase: limites");
  printf("%s: %d verificacoes, %d falhas\n", falhas ? "FAIL" : "PASS", checagens, falhas);
  return falhas ? 1 : 0;
}
