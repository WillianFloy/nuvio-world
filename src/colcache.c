// Cache das fileiras das Collections: ver colcache.h.
#include "colcache.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include "dados.h"
#include "sessao.h"
#include "perfis.h"
#include "ajustes.h"
#include "descoberta.h"

#define CC_MAGIA  0x5243564Eu   // "NVCR"
#define CC_VERSAO 1u
#define CC_ZMIN   16u
#define CC_MAX_ENTRADAS 512

typedef struct {
  unsigned magia, versao, tamItem;
  unsigned long long chave;
  long long tempo;
  int n, extraN, pagina, mais;
  unsigned encN;
} CcCab;

typedef struct {
  uint64_t chave;
  long long tempo;
  ColCacheMeta meta;
  unsigned char *enc;
  size_t encN;
  unsigned long uso;
} CcEnt;

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static CcEnt ents[CC_MAX_ENTRADAS];
static int nEnts;
static long bytesMem, teto = 2L * 1024 * 1024;
static unsigned long usoRel;
static long acertos, faltas;

void colcache_teto(long bytes) {
  pthread_mutex_lock(&trava);
  if (bytes >= 64 * 1024) teto = bytes;
  pthread_mutex_unlock(&trava);
}

// --- RLE: pares {u32 zeros, u32 literal, bytes[literal]} ---------------------
size_t colcache_rle(const unsigned char *src, size_t n, unsigned char *dst) {
  size_t i = 0, saida = 0;
  while (i < n) {
    size_t z = 0, l = 0, j;
    unsigned zz, ll;
    while (i + z < n && src[i + z] == 0) z++;
    j = i + z;
    while (j + l < n) {
      size_t k = 0;
      if (src[j + l] == 0) {
        while (j + l + k < n && src[j + l + k] == 0 && k < CC_ZMIN) k++;
        if (k >= CC_ZMIN || j + l + k >= n) break;
        l += k;
        continue;
      }
      l++;
    }
    zz = (unsigned)z; ll = (unsigned)l;
    if (dst) { memcpy(dst + saida, &zz, 4); memcpy(dst + saida + 4, &ll, 4); memcpy(dst + saida + 8, src + j, l); }
    saida += 8 + l;
    i = j + l;
  }
  return saida;
}
int colcache_unrle(const unsigned char *enc, size_t encN, unsigned char *dst, size_t n) {
  size_t i = 0, o = 0;
  memset(dst, 0, n);
  while (i < encN) {
    unsigned zz, ll;
    if (i + 8 > encN) return 0;
    memcpy(&zz, enc + i, 4); memcpy(&ll, enc + i + 4, 4);
    i += 8;
    if (zz > n - o || ll > n - o - zz || ll > encN - i) return 0;
    o += zz;
    memcpy(dst + o, enc + i, ll);
    o += ll; i += ll;
  }
  return o == n;
}

// --- chave -----------------------------------------------------------------
static uint64_t fnv(uint64_t h, const void *p, size_t n) {
  const unsigned char *b = p;
  for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
  return h;
}
static uint64_t fnvs(uint64_t h, const char *s) {
  h = fnv(h, s ? s : "", s ? strlen(s) : 0);
  return fnv(h, "\x1f", 1);
}
uint64_t colcache_chave(const ColSource *f, const char *base, int limite) {
  uint64_t h = 14695981039346656037ULL;
  int perfil = perfis_ativo(), ocultar = ajustes_ocultar_filmes_vistos();
  unsigned tam = (unsigned)sizeof(CatItem);
  h = fnvs(h, sessao_usuario());
  h = fnv(h, &perfil, sizeof perfil);
  h = fnvs(h, desc_tmdb_idioma());
  h = fnv(h, &ocultar, sizeof ocultar);
  h = fnv(h, &limite, sizeof limite);
  h = fnv(h, &tam, sizeof tam);
  h = fnvs(h, base);
  if (f) {
    h = fnvs(h, f->type); h = fnvs(h, f->catId); h = fnvs(h, f->genre);
    h = fnvs(h, f->addonId); h = fnvs(h, f->prov); h = fnvs(h, f->tmdbTipo);
    h = fnv(h, &f->tmdbId, sizeof f->tmdbId);
    h = fnvs(h, f->midia); h = fnvs(h, f->ordenar); h = fnvs(h, f->ordem);
    h = fnv(h, &f->traktLista, sizeof f->traktLista);
    h = fnvs(h, f->filtros);
    h = fnv(h, &f->somenteLancados, sizeof f->somenteLancados);
  }
  return h ? h : 1;
}

// --- memoria ---------------------------------------------------------------
static int achar(uint64_t chave) {
  for (int i = 0; i < nEnts; i++) if (ents[i].chave == chave) return i;
  return -1;
}
static void tirar(int i) {
  bytesMem -= (long)ents[i].encN;
  free(ents[i].enc);
  ents[i] = ents[--nEnts];
}
static void caber(size_t novo) {
  while (nEnts && (bytesMem + (long)novo > teto || nEnts >= CC_MAX_ENTRADAS)) {
    int velho = 0;
    for (int i = 1; i < nEnts; i++) if (ents[i].uso < ents[velho].uso) velho = i;
    tirar(velho);
  }
}
// Guarda (copia) um bloco ja comprimido. Com a trava.
static void guardar(uint64_t chave, long long tempo, const ColCacheMeta *m,
                    const unsigned char *enc, size_t encN) {
  int i = achar(chave);
  unsigned char *copia;
  if (encN > (size_t)teto) return;
  if (i >= 0) tirar(i);
  caber(encN);
  if (!(copia = malloc(encN ? encN : 1))) return;
  memcpy(copia, enc, encN);
  ents[nEnts] = (CcEnt){ chave, tempo, *m, copia, encN, ++usoRel };
  nEnts++;
  bytesMem += (long)encN;
}

// --- disco -----------------------------------------------------------------
static char *caminho(char *dst, unsigned tam, uint64_t chave) {
  char nome[64];
  snprintf(nome, sizeof nome, "colrow-%016llx.bin", (unsigned long long)chave);
  return dados_caminho(dst, tam, nome);
}
static int lerDisco(uint64_t chave, CcCab *cab, unsigned char **enc) {
  char cam[700];
  FILE *f;
  *enc = NULL;
  if (!caminho(cam, sizeof cam, chave) || !(f = fopen(cam, "rb"))) return 0;
  if (fread(cab, sizeof *cab, 1, f) != 1 || cab->magia != CC_MAGIA || cab->versao != CC_VERSAO ||
      cab->tamItem != sizeof(CatItem) || cab->chave != chave || cab->n < 0 || cab->extraN < 0 ||
      cab->encN > 4u * 1024 * 1024 || !(*enc = malloc(cab->encN ? cab->encN : 1)) ||
      fread(*enc, 1, cab->encN, f) != cab->encN) {
    fclose(f); free(*enc); *enc = NULL; return 0;
  }
  fclose(f);
  return 1;
}
static void gravarDisco(uint64_t chave, const CcCab *cab, const unsigned char *enc) {
  char cam[700], tmp[720];
  FILE *f;
  if (!caminho(cam, sizeof cam, chave)) return;
  snprintf(tmp, sizeof tmp, "%s.tmp-%ld", cam, (long)getpid());
  if (!(f = fopen(tmp, "wb"))) return;
  int ok = fwrite(cab, sizeof *cab, 1, f) == 1 && fwrite(enc, 1, cab->encN, f) == cab->encN;
  if (fclose(f) != 0) ok = 0;
  if (!ok || rename(tmp, cam) != 0) remove(tmp);
}

// --- API -------------------------------------------------------------------
int colcache_ler(uint64_t chave, CatItem *itens, int max, ColCacheMeta *meta, long *idade) {
  long long tempo = 0;
  unsigned char *enc = NULL;
  size_t encN = 0;
  ColCacheMeta m;
  int i, ok = 0;
  if (!itens || max < 1 || !meta) return 0;
  pthread_mutex_lock(&trava);
  i = achar(chave);
  if (i >= 0) {
    m = ents[i].meta; tempo = ents[i].tempo; ents[i].uso = ++usoRel;
    if (m.n + m.extraN <= max)
      ok = colcache_unrle(ents[i].enc, ents[i].encN, (unsigned char *)itens,
                          sizeof(CatItem) * (size_t)(m.n + m.extraN));
  }
  pthread_mutex_unlock(&trava);
  if (!ok) {   // memoria nao tinha: o disco (fora da trava)
    CcCab cab;
    if (lerDisco(chave, &cab, &enc)) {
      m = (ColCacheMeta){ cab.n, cab.extraN, cab.pagina, cab.mais };
      tempo = cab.tempo; encN = cab.encN;
      if (m.n + m.extraN <= max &&
          colcache_unrle(enc, encN, (unsigned char *)itens, sizeof(CatItem) * (size_t)(m.n + m.extraN))) {
        ok = 1;
        pthread_mutex_lock(&trava);
        guardar(chave, tempo, &m, enc, encN);
        pthread_mutex_unlock(&trava);
      }
      free(enc);
    }
  }
  pthread_mutex_lock(&trava);
  if (ok) acertos++; else faltas++;
  pthread_mutex_unlock(&trava);
  if (!ok) return 0;
  *meta = m;
  if (idade) { long long d = (long long)time(NULL) - tempo; *idade = d < 0 ? 0 : (long)d; }
  return 1;
}

void colcache_gravar(uint64_t chave, const CatItem *itens, const ColCacheMeta *meta) {
  size_t bytes, encN;
  unsigned char *enc;
  CcCab cab;
  int total;
  if (!itens || !meta || meta->n < 0 || meta->extraN < 0) return;
  total = meta->n + meta->extraN;
  bytes = sizeof(CatItem) * (size_t)total;
  encN = colcache_rle((const unsigned char *)itens, bytes, NULL);
  if (!(enc = malloc(encN ? encN : 1))) return;
  colcache_rle((const unsigned char *)itens, bytes, enc);
  memset(&cab, 0, sizeof cab);
  cab.magia = CC_MAGIA; cab.versao = CC_VERSAO; cab.tamItem = (unsigned)sizeof(CatItem);
  cab.chave = chave; cab.tempo = (long long)time(NULL);
  cab.n = meta->n; cab.extraN = meta->extraN; cab.pagina = meta->pagina; cab.mais = meta->mais;
  cab.encN = (unsigned)encN;
  pthread_mutex_lock(&trava);
  guardar(chave, cab.tempo, meta, enc, encN);
  pthread_mutex_unlock(&trava);
  gravarDisco(chave, &cab, enc);
  free(enc);
}

void colcache_limpar(int disco) {
  pthread_mutex_lock(&trava);
  while (nEnts) tirar(nEnts - 1);
  pthread_mutex_unlock(&trava);
  if (disco) {
    char pasta[700], *barra;
    DIR *d;
    struct dirent *e;
    if (!dados_caminho(pasta, sizeof pasta, "colrow-0.bin") || !(barra = strrchr(pasta, '/'))) return;
    *barra = 0;
    if (!(d = opendir(pasta))) return;
    while ((e = readdir(d)))
      if (!strncmp(e->d_name, "colrow-", 7)) {
        char cam[1000];
        snprintf(cam, sizeof cam, "%s/%s", pasta, e->d_name);
        remove(cam);
      }
    closedir(d);
  }
}

void colcache_stats(long *b, int *n, long *a, long *f) {
  pthread_mutex_lock(&trava);
  if (b) *b = bytesMem;
  if (n) *n = nEnts;
  if (a) *a = acertos;
  if (f) *f = faltas;
  pthread_mutex_unlock(&trava);
}
