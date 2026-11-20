// RITMO DOS QUADROS (World, 20/11): FRAME TIME, FRAME PACING E LATENCIA.
//
// So MEDE. Nao segura, nao agenda, nao limita: o laco continua com o vsync de
// sempre (SDL_GL_SetSwapInterval(1)) e o governador automatico do vidro. Os
// modos de FPS da etapa 4 (30 por presentation-time, espera de CPU) pioraram
// a TCL e foram removidos; este modulo existe para que a proxima decisao de
// ritmo venha de numeros do aparelho, nao de hipotese.
//
// O QUE SE MEDE, a cada quadro apresentado (depois de SDL_GL_SwapWindow):
//   - intervalo entre apresentacoes (frame time real, o que o olho ve);
//   - CPU do quadro (do inicio do laco ate antes do swap);
//   - tempo dentro do swap (espera de vsync/compositor/driver);
//   - latencia do controle: da tecla (SDL_KEYDOWN) ao fim do swap do PRIMEIRO
//     quadro desenhado depois dela.
// E na janela de 3 s:
//   - FPS APRESENTADO = quadros / tempo total (nao media de FPS por amostra);
//   - p50, p95, p99 e maximo do frame time;
//   - quadros fora do prazo (intervalo > 1,5 periodo) e varreduras puladas
//     (soma de round(intervalo/periodo) - 1: quantas atualizacoes da tela
//     repetiram o quadro anterior);
//   - histograma por quantas varreduras cada quadro ocupou (1, 2, 3, 4+): uma
//     cadencia regular concentra tudo numa coluna; engasgo espalha.
//
// O PERIODO e o da VARREDURA DA TELA (swapint_referencia), nunca o ritmo
// medido do proprio app. Ate a auditoria de 20/11 entrava aqui a EMA do
// swapint, que converge para o periodo do quadro: a 36 fps em 60 Hz (cadencia
// 1-2-2 varreduras) o relato dava 0 fora do prazo, 0 varreduras puladas e o
// histograma todo na coluna 1 -- escondia exatamente o problema da Home
// (tests/ritmo_gpu.sh reproduz). O ritmo medido continua no log, como
// `ritmo-medio` (tempo total / quadros), ao lado da referencia.
//
// MEDIA HARMONICA (rq_media_harmonica): so para juntar GRUPOS COM O MESMO
// NUMERO DE QUADROS (ex.: 60 quadros a 30 fps + 60 quadros a 60 fps = 40 fps,
// porque o tempo e 2 s + 1 s = 3 s para 120 quadros). Para janelas de tempo
// igual a taxa certa e quadros/tempo, e e o que o log usa.
//
// CADENCIA UNIFORME (rq_cadencia_uniforme): fps casa com a tela quando cada
// quadro ocupa um numero INTEIRO de varreduras (hz/fps inteiro): 40 em 120 Hz
// (3 varreduras) sim; 40 em 60 Hz (1,5) nao. So diagnostico: nada aqui muda a
// frequencia da TV nem a do app.
#ifndef NV_RITMOQUADRO_H
#define NV_RITMOQUADRO_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RQ_MAX 1024
#define RQ_LAT_MAX 64

typedef struct {
  float iv[RQ_MAX], cpu[RQ_MAX], swp[RQ_MAX];
  int n;
  double ultApresent;     // ms do ultimo swap, 0 = nenhum
  double total;           // soma dos intervalos da janela
  float lat[RQ_LAT_MAX];
  int nLat;
  double teclaPend;       // ms da tecla ainda sem quadro, 0 = nenhuma
} RitmoQuadro;

static inline void rq_tecla(RitmoQuadro *r, double agora) {
  if (r->teclaPend <= 0.0) r->teclaPend = agora;   // a mais antiga sem resposta
}

// Depois do swap. `inicioQuadro` e o instante em que o quadro comecou a ser
// montado: a tecla so conta como respondida por um quadro que comecou depois dela.
static inline void rq_quadro(RitmoQuadro *r, double apresent, double inicioQuadro, double cpu, double swp) {
  if (r->ultApresent > 0.0) {
    double d = apresent - r->ultApresent;
    if (d > 0.0 && d < 1000.0 && r->n < RQ_MAX) {   // >1 s: suspensao, nao ritmo
      r->iv[r->n] = (float)d; r->cpu[r->n] = (float)cpu; r->swp[r->n] = (float)swp;
      r->n++; r->total += d;
    }
  }
  r->ultApresent = apresent;
  if (r->teclaPend > 0.0 && inicioQuadro >= r->teclaPend) {
    if (r->nLat < RQ_LAT_MAX) r->lat[r->nLat++] = (float)(apresent - r->teclaPend);
    r->teclaPend = 0.0;
  }
}

static int rq_cmp(const void *a, const void *b) {
  float x = *(const float *)a, y = *(const float *)b;
  return x < y ? -1 : x > y;
}
// Percentil p (0..100) por ordenacao de uma copia: nearest-rank.
static inline float rq_percentil(const float *v, int n, float p) {
  float tmp[RQ_MAX];
  int k;
  if (n <= 0) return 0.0f;
  if (n > RQ_MAX) n = RQ_MAX;
  memcpy(tmp, v, (size_t)n * sizeof *tmp);
  qsort(tmp, (size_t)n, sizeof *tmp, rq_cmp);
  k = (int)ceilf(p / 100.0f * (float)n) - 1;
  if (k < 0) k = 0;
  if (k >= n) k = n - 1;
  return tmp[k];
}

typedef struct {
  int n, foraPrazo, pulados, hist[4], nLat;
  double fps, p50, p95, p99, max, cpu50, cpu95, swp50, swp95, lat50, latMax, periodo, ritmoMedio;
} RqRelato;

// Fecha a janela (o que se mediu desde o ultimo relato) e zera. `periodo` =
// periodo de REFERENCIA da tela (swapint_referencia), em ms.
static inline RqRelato rq_relato(RitmoQuadro *r, double periodo) {
  RqRelato o;
  int i;
  memset(&o, 0, sizeof o);
  o.periodo = periodo > 0.5 ? periodo : 16.667;
  o.n = r->n;
  if (r->n > 0) {
    o.fps = r->total > 0.0 ? r->n * 1000.0 / r->total : 0.0;
    o.ritmoMedio = r->total / r->n;
    o.p50 = rq_percentil(r->iv, r->n, 50); o.p95 = rq_percentil(r->iv, r->n, 95);
    o.p99 = rq_percentil(r->iv, r->n, 99); o.max = rq_percentil(r->iv, r->n, 100);
    o.cpu50 = rq_percentil(r->cpu, r->n, 50); o.cpu95 = rq_percentil(r->cpu, r->n, 95);
    o.swp50 = rq_percentil(r->swp, r->n, 50); o.swp95 = rq_percentil(r->swp, r->n, 95);
    for (i = 0; i < r->n; i++) {
      int v = (int)lround(r->iv[i] / o.periodo);
      if (v < 1) v = 1;
      if (r->iv[i] > 1.5 * o.periodo) o.foraPrazo++;
      o.pulados += v - 1;
      o.hist[v >= 4 ? 3 : v - 1]++;
    }
  }
  o.nLat = r->nLat;
  if (r->nLat > 0) { o.lat50 = rq_percentil(r->lat, r->nLat, 50); o.latMax = rq_percentil(r->lat, r->nLat, 100); }
  r->n = 0; r->total = 0.0; r->nLat = 0;
  return o;
}

// `fonteRef`: de onde veio o periodo de referencia ("tela" = frequencia ativa
// informada; "piso" = menor ritmo sustentado medido, sem frequencia).
static inline int rq_formatar(const RqRelato *o, double hzAnunciado, const char *fonteRef, char *b, size_t nb) {
  return snprintf(b, nb,
    "[ritmo] tela=%.2fHz ref=%.2fms(%s) | apresentados=%d fps=%.1f ritmo-medio=%.2fms | frame p50=%.1f p95=%.1f p99=%.1f max=%.1fms"
    " | fora-do-prazo=%d(%.0f%%) varreduras-puladas=%d hist(1/2/3/4+)=%d/%d/%d/%d | cpu p50=%.1f p95=%.1f | swap p50=%.1f p95=%.1f"
    " | controle->tela n=%d p50=%.0f max=%.0fms",
    hzAnunciado, o->periodo, fonteRef ? fonteRef : "?", o->n, o->fps, o->ritmoMedio, o->p50, o->p95, o->p99, o->max,
    o->foraPrazo, o->n > 0 ? 100.0 * o->foraPrazo / o->n : 0.0, o->pulados, o->hist[0], o->hist[1], o->hist[2], o->hist[3],
    o->cpu50, o->cpu95, o->swp50, o->swp95, o->nLat, o->lat50, o->latMax);
}

// QUADRO LENTO para o [qd-lento] de main.c: acima de 2,5 varreduras da tela de
// referencia (60 Hz: 41,7 ms). Um quadro de 2 varreduras e atraso de cadencia
// (aparece em fora-do-prazo/hist); o [qd-lento] e para o tranco que se sente.
static inline int rq_quadro_lento(double dtms, double referencia) { return dtms > 2.5 * referencia; }

// Media harmonica de taxas de grupos com o MESMO numero de quadros.
static inline double rq_media_harmonica(const double *fps, int n) {
  double s = 0.0;
  int i;
  for (i = 0; i < n; i++) { if (fps[i] <= 0.0) return 0.0; s += 1.0 / fps[i]; }
  return n > 0 ? n / s : 0.0;
}
// Quantas varreduras por quadro se `fps` casa uniforme com `hz`; 0 se nao casa.
static inline int rq_cadencia_uniforme(double hz, double fps) {
  double k;
  if (hz <= 0.0 || fps <= 0.0 || fps > hz * 1.01) return 0;
  k = hz / fps;
  return fabs(k - floor(k + 0.5)) <= 0.01 * k ? (int)floor(k + 0.5) : 0;
}
#endif
