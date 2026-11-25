// FACETAS: geometria e curvas do menu de segurar OK (ver facetas.h).
#include "facetas.h"
#include <math.h>
#include <string.h>

static float cl(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static float seg(float p, float a, float b) { return cl((p - a) / (b - a), 0.0f, 1.0f); }
// Saida cubica: parte rapido e assenta sem repique (a mesma familia da curva de
// abertura 0,2 / 0,8 / 0,2 / 1 da prancha). Reversivel: o relogio e que anda.
static float eo(float t) { t = 1.0f - cl(t, 0.0f, 1.0f); return 1.0f - t * t * t; }

#define DOCK_VAO 14.0f
#define BULGE    14.0f

void faceta_geometria(GfxRect it, int n, const float *larg, int palco, float abaixo,
                      float W, float H, FacetaGeo *g) {
  const float T = FACETA_T;
  float cx = it.x + it.w * 0.5f, cy = it.y + it.h * 0.5f;
  float maxL = T, passo = FACETA_PASSO, rf, lf, need, bot, dockW;
  int i, dock, largo, ladoOk, cabeBaixo, cabeCima, lado;
  memset(g, 0, sizeof *g);
  if (n < 1) n = 1;
  if (n > FACETA_MAX) n = FACETA_MAX;
  g->n = n;
  if (larg) for (i = 0; i < n; i++) if (larg[i] > maxL) maxL = larg[i];
  rf = W - FACETA_BORDA_X - (it.x + it.w);
  lf = it.x - FACETA_BORDA_X;
  need = FACETA_AO_LADO + maxL;
  bot = it.y + it.h + abaixo;
  dockW = (float)n * T + (float)(n - 1) * DOCK_VAO;
  dock = palco && dockW <= it.w - 12.0f;
  largo = it.w >= (float)n * FACETA_PASSO - 10.0f;
  ladoOk = rf >= need || lf >= need;
  cabeBaixo = bot + 34.0f + T + 20.0f + 56.0f <= H - FACETA_BORDA_Y;
  cabeCima = it.y - 34.0f - T - 70.0f >= FACETA_BORDA_Y;

  if (dock) {
    float by = it.y + it.h - 26.0f - T;
    g->modo = FACETA_DOCK; g->lado = 1;
    for (i = 0; i < n; i++) { g->x[i] = cx - dockW * 0.5f + (float)i * (T + DOCK_VAO); g->y[i] = by; }
    g->ancX = cx; g->ancY = it.y + it.h;
    g->legX = cx; g->legY = by - 70.0f; g->legAcima = 1;
    return;
  }
  // O Palco nunca usa o arco lateral: a ficha de informacoes mora de um lado e
  // os vizinhos do outro. Sem lugar no dock, as fichas descem para baixo do nome.
  if ((largo || !ladoOk || palco) && (cabeBaixo || cabeCima)) {
    int abaixoOk = cabeBaixo;
    float base = abaixoOk ? bot + 34.0f : it.y - 34.0f - T;
    if (n > 1 && (float)(n - 1) * passo + T > W - 2.0f * FACETA_BORDA_X)
      passo = (W - 2.0f * FACETA_BORDA_X - T) / (float)(n - 1);
    g->modo = FACETA_H; g->lado = abaixoOk ? 1 : -1;
    for (i = 0; i < n; i++) {
      float d = n > 1 ? ((float)i - (float)(n - 1) * 0.5f) / ((float)(n - 1) * 0.5f) : 0.0f;
      g->x[i] = cl(cx + ((float)i - (float)(n - 1) * 0.5f) * passo - T * 0.5f, FACETA_BORDA_X, W - FACETA_BORDA_X - T);
      g->y[i] = base + (abaixoOk ? 14.0f : -14.0f) * (1.0f - d * d);
    }
    g->ancX = cx; g->ancY = abaixoOk ? it.y + it.h : it.y;
    g->legX = cx; g->legAcima = !abaixoOk;
    g->legY = abaixoOk ? base + T + 30.0f : base - 70.0f;
    return;
  }
  // arco vertical ao lado
  lado = (rf >= need || (lf < need && rf >= lf)) ? 1 : -1;
  { float avail = H - 2.0f * FACETA_BORDA_Y, top;
    if (n > 1 && (float)(n - 1) * passo + T > avail) passo = (avail - T) / (float)(n - 1);
    top = cl(cy - (float)(n - 1) * passo * 0.5f - T * 0.5f, FACETA_BORDA_Y,
             H - FACETA_BORDA_Y - ((float)(n - 1) * passo + T));
    g->modo = FACETA_V; g->lado = lado;
    for (i = 0; i < n; i++) {
      float d = n > 1 ? ((float)i - (float)(n - 1) * 0.5f) / ((float)(n - 1) * 0.5f) : 0.0f;
      float b = BULGE * (1.0f - d * d);
      g->y[i] = top + (float)i * passo;
      g->x[i] = lado > 0 ? it.x + it.w + FACETA_AO_LADO + b : it.x - FACETA_AO_LADO - T - b;
    }
    g->ancX = lado > 0 ? it.x + it.w : it.x; g->ancY = cy;
  }
}

GfxRect faceta_confirmacao(GfxRect it, float w, float h, int palco, float abaixo,
                           float W, float H, int *lado) {
  float rf = W - FACETA_BORDA_X - (it.x + it.w), lf = it.x - FACETA_BORDA_X;
  float cx = it.x + it.w * 0.5f, cy = it.y + it.h * 0.5f, bot = it.y + it.h + abaixo;
  GfxRect r = { 0, 0, w, h };
  int l = 0;
  if (!palco && (rf >= w + FACETA_AO_LADO || lf >= w + FACETA_AO_LADO)) {
    l = (rf >= w + FACETA_AO_LADO && (lf < w + FACETA_AO_LADO || rf >= lf)) ? 1 : -1;
    r.x = l > 0 ? it.x + it.w + FACETA_AO_LADO : it.x - FACETA_AO_LADO - w;
    r.y = cl(cy - h * 0.5f, FACETA_BORDA_Y, H - FACETA_BORDA_Y - h);
  } else {
    r.x = cl(cx - w * 0.5f, FACETA_BORDA_X, W - FACETA_BORDA_X - w);
    if (bot + 28.0f + h <= H - FACETA_BORDA_Y) { r.y = bot + 28.0f; l = 0; }
    else if (it.y - 28.0f - h >= FACETA_BORDA_Y) { r.y = it.y - 28.0f - h; l = 2; }
    else {   // nada sob nem sobre: o lado de maior espaco, preso na tela
      l = rf >= lf ? 1 : -1;
      r.x = cl(l > 0 ? it.x + it.w + FACETA_AO_LADO : it.x - FACETA_AO_LADO - w,
               FACETA_BORDA_X, W - FACETA_BORDA_X - w);
      r.y = cl(cy - h * 0.5f, FACETA_BORDA_Y, H - FACETA_BORDA_Y - h);
    }
  }
  if (lado) *lado = l;
  return r;
}

float faceta_p_cartaz(float p) { return 1.0f + (FACETA_ESCALA_CARTAZ - 1.0f) * eo(seg(p, 0.0f, 0.70f)); }
float faceta_p_veu(float p)    { return FACETA_VEU * eo(seg(p, 0.0f, 0.60f)); }
float faceta_p_ficha(float p, int i, int n) {
  float stag = n > 1 ? fminf(0.07f, 0.40f / (float)(n - 1)) : 0.0f;
  float ini = 0.10f + stag * (float)i;
  return eo(seg(p, ini, ini + 0.50f));
}
float faceta_p_rotulo(float p) { return eo(seg(p, 0.70f, 1.0f)); }
float faceta_p_orbita(float p) { return eo(seg(p, 0.04f, 0.80f)); }

// Catmull-Rom entre os centros das fichas, amostrado fino e depois re-amostrado
// a cada ~9 px de comprimento de arco.
int faceta_orbita(const FacetaGeo *g, float prog, FacetaPonto *out, int max) {
  FacetaPonto c[FACETA_MAX + 2], s[(FACETA_MAX + 2) * 16];
  int i, k, ns = 0, no = 0;
  float total = 0.0f, alvo, acc, prox;
  if (!g || g->n < 2 || g->modo == FACETA_DOCK || prog <= 0.0f || max <= 0) return 0;
  for (i = 0; i < g->n; i++) { c[i + 1].x = g->x[i] + FACETA_T * 0.5f; c[i + 1].y = g->y[i] + FACETA_T * 0.5f; }
  c[0] = c[1]; c[g->n + 1] = c[g->n];
  for (i = 1; i < g->n; i++)
    for (k = 0; k < 16; k++) {
      float t = (float)k / 16.0f, t2 = t * t, t3 = t2 * t;
      FacetaPonto p0 = c[i - 1], p1 = c[i], p2 = c[i + 1], p3 = c[i + 2];
      s[ns].x = 0.5f * ((2 * p1.x) + (-p0.x + p2.x) * t + (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 + (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3);
      s[ns].y = 0.5f * ((2 * p1.y) + (-p0.y + p2.y) * t + (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 + (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3);
      ns++;
    }
  s[ns++] = c[g->n];
  for (i = 1; i < ns; i++) total += hypotf(s[i].x - s[i - 1].x, s[i].y - s[i - 1].y);
  if (total < 1.0f) return 0;
  alvo = total * (prog > 1.0f ? 1.0f : prog);
  acc = 0.0f; prox = 0.0f;
  out[no++] = s[0];
  for (i = 1; i < ns && no < max; i++) {
    float d = hypotf(s[i].x - s[i - 1].x, s[i].y - s[i - 1].y);
    acc += d;
    if (acc > alvo) break;
    if (acc >= prox + 9.0f) { out[no++] = s[i]; prox = acc; }
  }
  return no;
}
