// Ver r8tempo.h. As formulas sao as de tools/r8 (base.py, panels.py, menus.py,
// fase1.py); tests/r8_tempo.sh confere com a especificacao exportada.
#include "r8tempo.h"
#include <math.h>
#include <string.h>

static float prender(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
float r8_seg(float t, float a, float b) { return b > a ? prender((t - a) / (b - a), 0.0f, 1.0f) : (t >= b ? 1.0f : 0.0f); }
float r8_lerp(float a, float b, float k) { return a + (b - a) * k; }
float r8_dec(float k) { k = prender(k, 0.0f, 1.0f); return 1.0f - powf(1.0f - k, 3.2f); }
float r8_acc(float k) { k = prender(k, 0.0f, 1.0f); return powf(k, 2.4f); }
float r8_mola(float k) {
  k = prender(k, 0.0f, 1.0f);
  if (k >= 1.0f) return 1.0f;
  return 1.0f - expf(-7.5f * k) * cosf(10.5f * k) * (1.0f - k);
}

// ---------------------------------------------------------------- abertura
void r8ab_iniciar(R8Abertura *a, float dAbrir, float dFechar) {
  memset(a, 0, sizeof *a);
  a->dAbrir = dAbrir; a->dFechar = dFechar;
}
float r8ab_linear(const R8Abertura *a, double agora) {
  float t;
  switch (a->estado) {
    case 1: t = (float)(agora - a->t0); return a->v0 + (1.0f - a->v0) * r8_seg(t, 0.0f, a->dAbrir);
    case 2: t = (float)(agora - a->t0); return a->v0 * (1.0f - r8_seg(t, 0.0f, a->dFechar));
    default: return 0.0f;
  }
}
float r8ab_valor(const R8Abertura *a, double agora) {
  float t;
  switch (a->estado) {
    case 1: t = (float)(agora - a->t0); return a->v0 + (1.0f - a->v0) * r8_dec(r8_seg(t, 0.0f, a->dAbrir));
    case 2: t = (float)(agora - a->t0); return a->v0 * (1.0f - r8_acc(r8_seg(t, 0.0f, a->dFechar)));
    default: return 0.0f;
  }
}
void r8ab_abrir(R8Abertura *a, double agora) {
  float v = r8ab_valor(a, agora);
  if (a->estado == 1) return;
  // Reabrir com a superficie ainda quase aberta mantem o conteudo (nao pisca).
  if (a->estado == 0 || v < 0.5f) a->tAbriu = agora;
  a->v0 = v; a->t0 = agora; a->estado = 1;
}
void r8ab_fechar(R8Abertura *a, double agora) {
  if (a->estado != 1) return;
  a->tConteudo = agora;
  a->v0 = r8ab_valor(a, agora + R8_CONTEUDO_ANTES);
  a->t0 = agora + R8_CONTEUDO_ANTES;
  a->estado = 2;
}
int r8ab_fechada(const R8Abertura *a, double agora) {
  return a->estado == 0 || (a->estado == 2 && agora - a->t0 >= a->dFechar);
}
int r8ab_assentada(const R8Abertura *a, double agora) {
  return a->estado == 1 && agora - a->t0 >= a->dAbrir;
}
float r8ab_cascata(const R8Abertura *a, double agora, float i, float inicio, float passo, float dur) {
  float t, v;
  if (a->estado == 0) return 0.0f;
  t = (float)(agora - a->tAbriu);
  v = r8_seg(t, inicio + i * passo, inicio + i * passo + dur);
  if (a->estado == 2) v *= 1.0f - r8_seg((float)(agora - a->tConteudo), 0.0f, R8_CONTEUDO_SAIR);
  return v;
}

// ---------------------------------------------------------------- mola
void r8mola_iniciar(R8Mola *m, const float *v, int n) {
  memset(m, 0, sizeof *m);
  m->n = n > 5 ? 5 : n;
  for (int i = 0; i < m->n; i++) m->de[i] = m->para[i] = v[i];
  m->t0 = -1e9;
}
void r8mola_valor(const R8Mola *m, double agora, float *out) {
  float k = r8_mola(r8_seg((float)(agora - m->t0), 0.0f, R8_MOLA_DUR));
  for (int i = 0; i < m->n; i++) out[i] = r8_lerp(m->de[i], m->para[i], k);
}
void r8mola_ir(R8Mola *m, const float *alvo, double agora) {
  float atual[5];
  int igual = 1;
  for (int i = 0; i < m->n; i++) if (alvo[i] != m->para[i]) igual = 0;
  if (igual) return;
  r8mola_valor(m, agora, atual);
  for (int i = 0; i < m->n; i++) { m->de[i] = atual[i]; m->para[i] = alvo[i]; }
  m->t0 = agora;
}
int r8mola_parada(const R8Mola *m, double agora) { return agora - m->t0 >= R8_MOLA_DUR; }

// ---------------------------------------------------------------- troca
void r8troca_iniciar(R8Troca *x, int atual) { x->atual = atual; x->anterior = -1; x->t0 = -1e9; }
void r8troca_ir(R8Troca *x, int novo, double agora) {
  if (novo == x->atual) return;
  x->anterior = x->atual; x->atual = novo; x->t0 = agora;
}
float r8troca_peso(const R8Troca *x, double agora, int k, float *dx) {
  float p = x->anterior < 0 ? 1.0f : r8_seg((float)(agora - x->t0), 0.0f, R8_TROCA_DUR);
  float d = 0.0f, o = 0.0f;
  if (k == x->atual) { o = r8_seg(p, 0.25f, 1.0f); d = R8_TROCA_DX * (1.0f - r8_dec(p)); }
  else if (k == x->anterior && p < 1.0f) { o = 1.0f - r8_seg(p, 0.0f, 0.55f); d = -R8_TROCA_DX * r8_dec(p); }
  if (dx) *dx = d;
  return o;
}

// ---------------------------------------------------------------- segurar
void r8seg_iniciar(R8Segurar *s) { memset(s, 0, sizeof *s); }
void r8seg_apertar(R8Segurar *s, double agora) { s->estado = 1; s->t0 = agora; }
static float progSegurando(const R8Segurar *s, double agora) {
  return r8_seg((float)(agora - s->t0), R8_SEGURAR_MORTO, R8_SEGURAR_MORTO + R8_SEGURAR_ENCHER);
}
int r8seg_passo(R8Segurar *s, double agora) {
  if (s->estado == 1 && agora - s->t0 >= R8_SEGURAR_MORTO + R8_SEGURAR_ENCHER) {
    s->estado = 3; s->tSolto = s->t0 + R8_SEGURAR_MORTO + R8_SEGURAR_ENCHER;
    return 1;
  }
  return 0;
}
int r8seg_soltar(R8Segurar *s, double agora) {
  r8seg_passo(s, agora);
  if (s->estado == 1) {
    s->pSolto = progSegurando(s, agora);
    s->tSolto = agora; s->estado = 2;
    return 1;
  }
  return 0;
}
float r8seg_progresso(const R8Segurar *s, double agora) {
  switch (s->estado) {
    case 1: return progSegurando(s, agora);
    case 2: return s->pSolto * (1.0f - r8_dec(r8_seg((float)(agora - s->tSolto), 0.0f, R8_SEGURAR_ESVAZIA)));
    case 3: return agora - s->tSolto < 0.25 ? 1.0f : 0.0f;
    default: return 0.0f;
  }
}
// Na prancha: o fio aparece de 200 a 300 ms; completo, some de 700 a 800 ms;
// solto antes, some de 50 a 250 ms depois de soltar.
float r8seg_visivel(const R8Segurar *s, double agora) {
  float t = (float)(agora - s->t0);
  float entra = r8_seg(t, R8_SEGURAR_MORTO, R8_SEGURAR_MORTO + 0.1f);
  switch (s->estado) {
    case 1: return entra;
    case 3: return entra * (1.0f - r8_seg(t, 0.7f, 0.8f));
    case 2:
      // Soltou ainda na zona morta: nunca apareceu e nao aparece.
      if (s->tSolto - s->t0 < R8_SEGURAR_MORTO) return 0.0f;
      return entra * (1.0f - r8_seg((float)(agora - s->tSolto), 0.05f, 0.25f));
    default: return 0.0f;
  }
}
