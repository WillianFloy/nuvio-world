#include "pedagio.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

int pedagio_ligado = 1;

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static int seguidas;            // falhas seguidas do servico
static unsigned abertoAte;      // disjuntor aberto ate este instante (ms)
static int disjuntorAberto;
static PedagioStats st;

#define PEDAGIO_FALHAS_MAX 3
#define PEDAGIO_PAUSA_MS (10u * 60u * 1000u)

static const int DEGRAUS[] = { 256, 384, 512, 768, 1024, 1280, 1920 };

static int comeca(const char *s, const char *p) { return !strncmp(s, p, strlen(p)); }

// O host da URL (sem porta) em `h`.
static int hostDe(const char *url, char *h, size_t n) {
  const char *p = strstr(url, "://"), *f;
  size_t k;
  if (!p) return 0;
  p += 3;
  f = p;
  while (*f && *f != '/' && *f != ':' && *f != '?' && *f != '#') f++;
  k = (size_t)(f - p);
  if (!k || k >= n) return 0;
  memcpy(h, p, k); h[k] = 0;
  for (k = 0; h[k]; k++) h[k] = (char)tolower((unsigned char)h[k]);
  return 1;
}

static int terminaCom(const char *s, const char *suf) {
  size_t a = strlen(s), b = strlen(suf);
  return a >= b && !strcmp(s + a - b, suf);
}

static int hostLocal(const char *h) {
  return !strcmp(h, "localhost") || comeca(h, "127.") || comeca(h, "10.") ||
         comeca(h, "192.168.") || comeca(h, "169.254.") || terminaCom(h, ".local") ||
         (comeca(h, "172.") && h[4] && h[5] && h[6] == '.' && ((h[4] == '1' && h[5] >= '6') || (h[4] == '2') || (h[4] == '3' && h[5] <= '1'))) ||
         strchr(h, ':') != NULL;
}

// Hosts com escada de tamanhos propria (artetamanho.h) ou que nao devem passar.
static int hostDireto(const char *h) {
  return !strcmp(h, "image.tmdb.org") || terminaCom(h, "metahub.space") ||
         !strcmp(h, "wsrv.nl") || terminaCom(h, ".wsrv.nl") || !strcmp(h, "images.weserv.nl");
}

static int ehGif(const char *url) {
  char base[512];
  const char *q = strpbrk(url, "?#");
  size_t n = q ? (size_t)(q - url) : strlen(url), i;
  if (n >= sizeof base) n = sizeof base - 1;
  memcpy(base, url, n); base[n] = 0;
  for (i = 0; base[i]; i++) base[i] = (char)tolower((unsigned char)base[i]);
  return terminaCom(base, ".gif");
}

int pedagio_eh(const char *url) {
  return url && comeca(url, PEDAGIO_BASE "?url=");
}

int pedagio_url(const char *url, int limite, char *saida, size_t tam, unsigned agoraMs) {
  char h[256];
  size_t o, i;
  int w = 0, k;
  if (!pedagio_ligado || !url || !saida || tam < 64 || limite <= 0) return 0;
  if (!comeca(url, "http://") && !comeca(url, "https://")) return 0;
  if (!hostDe(url, h, sizeof h) || hostLocal(h) || hostDireto(h) || ehGif(url)) return 0;
  pthread_mutex_lock(&trava);
  if (disjuntorAberto && (int)(agoraMs - abertoAte) >= 0) { disjuntorAberto = 0; seguidas = 0; }
  k = disjuntorAberto;
  pthread_mutex_unlock(&trava);
  if (k) return 0;
  for (k = 0; k < (int)(sizeof DEGRAUS / sizeof *DEGRAUS); k++) if (DEGRAUS[k] >= limite) { w = DEGRAUS[k]; break; }
  if (!w) return 0;   // maior que 1920: nada a ganhar
  o = (size_t)snprintf(saida, tam, "%s?url=", PEDAGIO_BASE);
  for (i = 0; url[i] && o + 4 < tam; i++) {
    unsigned char c = (unsigned char)url[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':')
      saida[o++] = (char)c;
    else { snprintf(saida + o, tam - o, "%%%02X", c); o += 3; }
  }
  if (url[i]) return 0;   // nao coube
  saida[o] = 0;
  if ((size_t)snprintf(saida + o, tam - o, "&w=%d&we&output=webp&q=85", w) >= tam - o) return 0;
  return 1;
}

void pedagio_resultado(int ok, unsigned agoraMs) {
  pthread_mutex_lock(&trava);
  st.pedidos++;
  if (ok) { st.ok++; seguidas = 0; }
  else {
    st.falhas++;
    if (++seguidas >= PEDAGIO_FALHAS_MAX && !disjuntorAberto) {
      disjuntorAberto = 1; abertoAte = agoraMs + PEDAGIO_PAUSA_MS; st.desligado++;
      printf("[pedagio] %d falhas seguidas: desligado por 10 min, a arte vai direto ao original\n", seguidas);
      fflush(stdout);
    }
  }
  pthread_mutex_unlock(&trava);
}

void pedagio_stats(PedagioStats *o, int zerar) {
  pthread_mutex_lock(&trava);
  if (o) *o = st;
  if (zerar) memset(&st, 0, sizeof st);
  pthread_mutex_unlock(&trava);
}
