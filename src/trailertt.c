// Ver trailertt.h.
#include "trailertt.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "js.h"
#ifndef NV_TRAILERTT_TESTE
#include "descoberta.h"
#include "rede.h"
#endif

#define TT_MAX 192
#define TT_NEGATIVA_MS 600000u   // falhou: so pergunta de novo depois de 10 min
enum { TT_LIVRE = 0, TT_VOO, TT_OK, TT_FALHOU };
typedef struct { char id[40]; char tt[24]; char en[200]; int estado; unsigned quando; int serie; } Ent;
static Ent tab[TT_MAX];
static int proxima;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static char *(*baixarFn)(const char *, int);
static const char *chaveTeste;

static unsigned agoraMs(void) {
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned)(t.tv_sec * 1000u + t.tv_nsec / 1000000u);
}
static const char *chave(void) {
#ifdef NV_TRAILERTT_TESTE
  return chaveTeste ? chaveTeste : "";
#else
  if (chaveTeste) return chaveTeste;
  return desc_chave_tmdb_reserva();
#endif
}
static char *baixar(const char *url, int s) {
#ifdef NV_TRAILERTT_TESTE
  return baixarFn ? baixarFn(url, s) : NULL;
#else
  return baixarFn ? baixarFn(url, s) : rede_baixar(url, s);
#endif
}
// "tmdb:1234" (ou "tmdb:1234:2:5", episodio) -> 1234; 0 se nao e do TMDB.
static long numeroTmdb(const char *id) {
  long n;
  if (!id || strncmp(id, "tmdb:", 5)) return 0;
  n = strtol(id + 5, NULL, 10);
  return n > 0 ? n : 0;
}
static Ent *achar(const char *id) {
  for (int i = 0; i < TT_MAX; i++)
    if (tab[i].estado != TT_LIVRE && !strcmp(tab[i].id, id)) return &tab[i];
  return NULL;
}

typedef struct { char id[40]; long n; int serie; } Pedido;
// World (22/11): UMA consulta traz o "tt" e o titulo EM INGLES. A Apple busca pelo titulo na loja dos EUA e nao achava
// os titulos em portugues ("Sobrenatural", "Tipo Irmas": candidatos=0 no log da TCL); com o IMDb respondendo 403 a tudo,
// ela e a unica fonte de trailer do Android. /movie|tv/{id}?language=en-US&append_to_response=external_ids: o filme tem
// "imdb_id" e "title" na raiz; a serie tem "name" na raiz e o "imdb_id" dentro de "external_ids".
static void lerResposta(const char *resp, int serie, char *tt, size_t ttTam, char *en, size_t enTam) {
  tt[0] = en[0] = 0;
  if (!js_texto_raiz_em(resp, NULL, "imdb_id", tt, ttTam) || strncmp(tt, "tt", 2)) {
    const char *x = strstr(resp, "\"external_ids\"");
    tt[0] = 0;
    if (x && (x = strchr(x, '{'))) {
      const char *f = js_fim(x);
      if (!js_texto_raiz_em(x, f ? f + 1 : NULL, "imdb_id", tt, ttTam) || strncmp(tt, "tt", 2)) tt[0] = 0;
    }
  }
  if (!js_texto_raiz_em(resp, NULL, serie ? "name" : "title", en, enTam)) en[0] = 0;
}
static void *buscar(void *arg) {
  Pedido *p = (Pedido *)arg;
  char url[360], tt[24] = "", en[200] = "";
  const char *k = chave();
  char *resp = NULL;
  if (k && k[0]) {
    snprintf(url, sizeof url, "https://api.themoviedb.org/3/%s/%ld?api_key=%s&language=en-US&append_to_response=external_ids",
             p->serie ? "tv" : "movie", p->n, k);
    resp = baixar(url, 8);
  }
  if (resp) { lerResposta(resp, p->serie, tt, sizeof tt, en, sizeof en); free(resp); }
  pthread_mutex_lock(&trava);
  { Ent *e = achar(p->id);
    if (e) {
      snprintf(e->en, sizeof e->en, "%s", en);
      if (tt[0]) { snprintf(e->tt, sizeof e->tt, "%s", tt); e->estado = TT_OK; }
      else e->estado = TT_FALHOU;
      e->quando = agoraMs();
    } }
  pthread_mutex_unlock(&trava);
  printf("[trailer] id do IMDb para %s (%s): %s | titulo em ingles: %s\n", p->id, p->serie ? "serie" : "filme",
         tt[0] ? tt : (k && k[0] ? "TMDB nao tem" : "sem chave do TMDB"), en[0] ? en : "-");
  fflush(stdout);
  free(p);
  return NULL;
}

int trailertt_titulo_en(const char *id, char *dst, size_t tam) {
  int r = 0;
  Ent *e;
  if (!dst || !tam) return 0;
  dst[0] = 0;
  if (!numeroTmdb(id)) return 0;
  pthread_mutex_lock(&trava);
  e = achar(id);
  if (e && (e->estado == TT_OK || e->estado == TT_FALHOU) && e->en[0]) { snprintf(dst, tam, "%s", e->en); r = 1; }
  pthread_mutex_unlock(&trava);
  return r;
}

const char *trailertt_id(const char *id, int serie) {
  static __thread char saida[24];
  long n;
  Ent *e;
  if (!id || !id[0] || !strncmp(id, "tt", 2)) return id;
  n = numeroTmdb(id);
  if (!n) return id;   // kitsu/mal...: o detalhe ja resolve pelo ARM; aqui fica como esta
  pthread_mutex_lock(&trava);
  e = achar(id);
  if (e && e->estado == TT_OK) {
    snprintf(saida, sizeof saida, "%s", e->tt);
    pthread_mutex_unlock(&trava);
    return saida;
  }
  if (e && (e->estado == TT_VOO || agoraMs() - e->quando < TT_NEGATIVA_MS)) {
    pthread_mutex_unlock(&trava);
    return id;
  }
  if (!e) { e = &tab[proxima]; proxima = (proxima + 1) % TT_MAX; }
  memset(e, 0, sizeof *e);
  snprintf(e->id, sizeof e->id, "%s", id);
  e->estado = TT_VOO; e->serie = serie; e->quando = agoraMs();
  pthread_mutex_unlock(&trava);
  { Pedido *p = (Pedido *)calloc(1, sizeof *p);
    pthread_t f;
    if (!p) return id;
    snprintf(p->id, sizeof p->id, "%s", id); p->n = n; p->serie = serie;
    if (pthread_create(&f, NULL, buscar, p) == 0) pthread_detach(f);
    else {
      free(p);
      pthread_mutex_lock(&trava);
      if ((e = achar(id))) { e->estado = TT_FALHOU; e->quando = agoraMs(); }
      pthread_mutex_unlock(&trava);
    } }
  return id;
}

int trailertt_respondeu(const char *id) {
  int r;
  Ent *e;
  if (!numeroTmdb(id)) return 1;
  pthread_mutex_lock(&trava);
  e = achar(id);
  r = e && (e->estado == TT_OK || e->estado == TT_FALHOU);
  pthread_mutex_unlock(&trava);
  return r;
}

void trailertt_testes(char *(*f)(const char *, int), const char *k) {
  pthread_mutex_lock(&trava);
  baixarFn = f; chaveTeste = k;
  memset(tab, 0, sizeof tab); proxima = 0;
  pthread_mutex_unlock(&trava);
}
