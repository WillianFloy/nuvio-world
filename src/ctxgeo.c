#include "ctxgeo.h"

static float lim(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

float ctxgeo_dec(float k) {
  float u;
  k = lim(k, 0.0f, 1.0f);
  u = 1.0f - k;
  return 1.0f - u * u * u;
}

float ctxgeo_fase(float t, float a, float b) {
  if (b <= a) return t >= b ? 1.0f : 0.0f;
  return ctxgeo_dec((t - a) / (b - a));
}

void ctxgeo_folha(const GfxRect *item, float telaW, float telaH, CtxFolhaGeo *g) {
  float w = CTXG_FOLHA_W, m = CTXG_MARGEM;
  int lado = 1;
  if (item) {
    float cx = item->x + item->w * 0.5f;
    float livreDir = telaW - m - (item->x + item->w);
    float livreEsq = item->x - m;
    // A folha vai para o lado oposto ao item; se o item e largo (destaque,
    // card grande) fica do lado com mais espaco.
    lado = cx < telaW * 0.62f ? 1 : -1;
    if (lado > 0 && livreDir < w && livreEsq > livreDir) lado = -1;
    if (lado < 0 && livreEsq < w && livreDir > livreEsq) lado = 1;
  }
  if (w > telaW - 2.0f * m) w = telaW - 2.0f * m;
  g->lado = lado;
  g->folha = (GfxRect){ lado > 0 ? telaW - m - w : m, m, w, telaH - 2.0f * m };
  g->arte = (GfxRect){ g->folha.x, g->folha.y, w, w * CTXG_FOLHA_ARTE / CTXG_FOLHA_W };
}

void ctxgeo_desdobrar(GfxRect it, float alturaConteudo, int palco, float fichaVao, float fichaW,
                      float telaW, float telaH, CtxDesdGeo *g) {
  float pw = CTXG_DESD_W, m = CTXG_MARGEM, h, y;
  int lado;
  if (palco) {
    // O painel ocupa o lugar da ficha: comeca no vao dela, com a largura dela.
    pw = fichaW > 0.0f ? fichaW : pw;
    lado = 1;
    g->painel = (GfxRect){ it.x + it.w + fichaVao, it.y, pw, it.h };
    g->cont = (GfxRect){ it.x, it.y, it.w + fichaVao + pw, it.h };
    g->empurra = 0.0f;
    g->faixa = (GfxRect){ 0, 0, 0, 0 };
    g->lado = 1;
    if (alturaConteudo > it.h) { g->painel.h = alturaConteudo; g->cont.h = alturaConteudo; }
    return;
  }
  lado = it.x + it.w + pw <= telaW - m ? 1 : -1;
  if (lado < 0 && it.x - pw < m) lado = (telaW - m - (it.x + it.w)) >= (it.x - m) ? 1 : -1;
  h = it.h > alturaConteudo ? it.h : alturaConteudo;
  y = it.y;
  // Mais alto que o cartaz: cresce para baixo, mas nunca sai da tela.
  if (y + h > telaH - m) y = telaH - m - h;
  if (y < m) y = m;
  g->lado = lado;
  if (lado > 0) {
    g->painel = (GfxRect){ it.x + it.w, y, pw, h };
    g->cont = (GfxRect){ it.x, y, it.w + pw, h };
    g->faixa = (GfxRect){ it.x + it.w, it.y - 16.0f, telaW - (it.x + it.w), it.h + 32.0f };
  } else {
    g->painel = (GfxRect){ it.x - pw, y, pw, h };
    g->cont = (GfxRect){ it.x - pw, y, it.w + pw, h };
    g->faixa = (GfxRect){ 0, it.y - 16.0f, it.x, it.h + 32.0f };
  }
  g->empurra = pw;
}
