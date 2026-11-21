#include "miniatura.h"
#include "jpegrapido.h"
#include "sdlcompat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#define MINI_RODAPE 32
#define MINI_VERSAO 1
#define MINI_JPEG   1
#define MINI_CRU    2
#define MINI_QUALIDADE 90
#define MINI_MAX_PX (1920L * 1088L)

static pthread_mutex_t travaStats = PTHREAD_MUTEX_INITIALIZER;
static MiniStats st;

static void contar(int lida, int gravada, int corrompida, int falha, double msL, double msG, long bytes) {
  pthread_mutex_lock(&travaStats);
  st.lidas += lida; st.gravadas += gravada; st.corrompidas += corrompida; st.falhas += falha;
  st.msLer += msL; st.msGravar += msG; st.bytesGravados += bytes;
  pthread_mutex_unlock(&travaStats);
}

void mini_stats(MiniStats *o, int zerar) {
  pthread_mutex_lock(&travaStats);
  if (o) *o = st;
  if (zerar) memset(&st, 0, sizeof st);
  pthread_mutex_unlock(&travaStats);
}

static uint32_t fnv(const unsigned char *p, size_t n, uint32_t h) {
  size_t i;
  for (i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
  return h;
}

void mini_nome(const char *dir, const char *url, int limite, char *dst, size_t n) {
  uint32_t h = fnv((const unsigned char *)(url ? url : ""), url ? strlen(url) : 0, 2166136261u);
  if (limite < 0) limite = 0;
  if (limite > 0xFFFF) limite = 0xFFFF;
  snprintf(dst, n, "%s/%08x%04x.nvm", dir ? dir : ".", (unsigned)h, (unsigned)limite);
}

static void p16(unsigned char *b, unsigned v) { b[0] = (unsigned char)v; b[1] = (unsigned char)(v >> 8); }
static void p32(unsigned char *b, uint32_t v) { p16(b, v & 0xFFFF); p16(b + 2, v >> 16); }
static unsigned g16(const unsigned char *b) { return (unsigned)b[0] | ((unsigned)b[1] << 8); }
static uint32_t g32(const unsigned char *b) { return (uint32_t)g16(b) | ((uint32_t)g16(b + 2) << 16); }

// Rodape: [0..3] versao, formato, 0, 0 | [4..15] w h srcW srcH limite limTam
// (u16) | [16..19] magica do original | [20..23] tamanho da carga | [24..27]
// conferencia (FNV de 0..23) | [28..31] "NVMI".
static void montarRodape(unsigned char *r, int formato, int w, int h, const MiniInfo *info, uint32_t carga) {
  memset(r, 0, MINI_RODAPE);
  r[0] = MINI_VERSAO; r[1] = (unsigned char)formato;
  p16(r + 4, (unsigned)w); p16(r + 6, (unsigned)h);
  p16(r + 8, (unsigned)(info->srcW > 0xFFFF ? 0xFFFF : info->srcW));
  p16(r + 10, (unsigned)(info->srcH > 0xFFFF ? 0xFFFF : info->srcH));
  p16(r + 12, (unsigned)info->limite); p16(r + 14, (unsigned)info->limTam);
  memcpy(r + 16, info->magica, 4);
  p32(r + 20, carga);
  p32(r + 24, fnv(r, 24, 2166136261u));
  memcpy(r + 28, "NVMI", 4);
}

static double ms(Uint64 t0) {
  return (double)(SDL_GetPerformanceCounter() - t0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

static SDL_Surface *invalida(const char *caminho, FILE *f, SDL_Surface *s, int *corrompida) {
  if (f) fclose(f);
  if (s) SDL_FreeSurface(s);
  remove(caminho);
  if (corrompida) *corrompida = 1;
  contar(0, 0, 1, 0, 0, 0, 0);
  return NULL;
}

SDL_Surface *mini_ler(const char *caminho, int limite, MiniInfo *info, int *corrompida) {
  unsigned char r[MINI_RODAPE];
  FILE *f;
  long tam;
  int formato, w, h;
  uint32_t carga;
  SDL_Surface *s = NULL;
  Uint64 t0 = SDL_GetPerformanceCounter();
  if (corrompida) *corrompida = 0;
  if (!caminho || !(f = fopen(caminho, "rb"))) return NULL;
  if (fseek(f, 0, SEEK_END) != 0 || (tam = ftell(f)) < MINI_RODAPE + 16 ||
      fseek(f, tam - MINI_RODAPE, SEEK_SET) != 0 || fread(r, 1, MINI_RODAPE, f) != MINI_RODAPE)
    return invalida(caminho, f, NULL, corrompida);
  if (memcmp(r + 28, "NVMI", 4) || r[0] != MINI_VERSAO || g32(r + 24) != fnv(r, 24, 2166136261u))
    return invalida(caminho, f, NULL, corrompida);
  formato = r[1]; w = (int)g16(r + 4); h = (int)g16(r + 6); carga = g32(r + 20);
  if (w < 1 || h < 1 || (long)carga != tam - MINI_RODAPE) return invalida(caminho, f, NULL, corrompida);
  // Teto diferente nao e corrupcao: so nao e este o arquivo (o nome ja tem o
  // teto; isto pega colisao de hash).
  if ((int)g16(r + 12) != limite) { fclose(f); return NULL; }
  if (formato == MINI_CRU) {
    int y;
    if ((long)carga != (long)w * h * 4) return invalida(caminho, f, NULL, corrompida);
    s = nv_superficie(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
    if (!s) { fclose(f); return NULL; }
    rewind(f);
    for (y = 0; y < h; y++)
      if (fread((unsigned char *)s->pixels + (size_t)y * (size_t)s->pitch, 1, (size_t)w * 4, f) != (size_t)w * 4)
        return invalida(caminho, f, s, corrompida);
    fclose(f);
  } else if (formato == MINI_JPEG) {
    int fw = 0, fh = 0;
    fclose(f);
    s = jpeg_rapido_carregar(caminho, w, &fw, &fh);
    if (!s || s->w != w || s->h != h) return invalida(caminho, NULL, s, corrompida);
  } else {
    return invalida(caminho, f, NULL, corrompida);
  }
  if (info) {
    info->srcW = (int)g16(r + 8); info->srcH = (int)g16(r + 10);
    info->limite = (int)g16(r + 12); info->limTam = (int)g16(r + 14);
    memcpy(info->magica, r + 16, 4);
  }
  contar(1, 0, 0, 0, ms(t0), 0, 0);
  return s;
}

int mini_vale_gravar(int srcW, int srcH, int outW, int outH, unsigned decodeMs,
                     const unsigned char *cab, int nCab) {
  long pxSrc = (long)srcW * srcH, pxOut = (long)outW * outH;
  int conhecido = 0;
  if (outW < 8 || outH < 8 || pxOut > MINI_MAX_PX || srcW < 1 || srcH < 1) return 0;
  if (cab && nCab >= 3 && cab[0] == 0xFF && cab[1] == 0xD8 && cab[2] == 0xFF) conhecido = 1;
  else if (cab && nCab >= 8 && !memcmp(cab, "\x89PNG\r\n\x1a\n", 8)) conhecido = 1;
  else if (cab && nCab >= 16 && !memcmp(cab, "RIFF", 4) && !memcmp(cab + 8, "WEBP", 4)) {
    // VP8 e VP8L sao sempre uma imagem so. VP8X traz a bandeira de animacao
    // no byte 20 (bit 1): animado fica com o leitor proprio (gif.c).
    if (!memcmp(cab + 12, "VP8 ", 4) || !memcmp(cab + 12, "VP8L", 4)) conhecido = 1;
    else if (!memcmp(cab + 12, "VP8X", 4) && nCab >= 21 && !(cab[20] & 0x02)) conhecido = 1;
  }
  if (!conhecido) return 0;
  return pxSrc >= 2 * pxOut || decodeMs >= 60;
}

long mini_gravar(const char *caminho, SDL_Surface *s, const MiniInfo *info) {
  char tmp[1100];
  unsigned char r[MINI_RODAPE];
  FILE *f;
  long carga, total;
  int formato = MINI_CRU, opaca = 1, y, ok;
  Uint64 t0 = SDL_GetPerformanceCounter();
  if (!caminho || !s || !info || !s->pixels || s->w < 1 || s->h < 1 ||
      !s->format || s->format->format != SDL_PIXELFORMAT_ABGR8888) return 0;
  for (y = 0; y < s->h && opaca; y++) {
    const unsigned char *ln = (const unsigned char *)s->pixels + (size_t)y * (size_t)s->pitch;
    int x;
    for (x = 0; x < s->w; x++) if (ln[x * 4 + 3] != 255) { opaca = 0; break; }
  }
  if (opaca && jpeg_rapido_codificador()) formato = MINI_JPEG;
  snprintf(tmp, sizeof tmp, "%s.parcial", caminho);
  if (!(f = fopen(tmp, "wb"))) { contar(0, 0, 0, 1, 0, 0, 0); return 0; }
  if (formato == MINI_JPEG) {
    ok = jpeg_rapido_gravar_rgba(f, (const unsigned char *)s->pixels, s->w, s->h, s->pitch, MINI_QUALIDADE);
  } else {
    ok = 1;
    for (y = 0; y < s->h && ok; y++)
      ok = fwrite((const unsigned char *)s->pixels + (size_t)y * (size_t)s->pitch, 1, (size_t)s->w * 4, f) == (size_t)s->w * 4;
  }
  carga = ok ? ftell(f) : -1;
  if (ok && carga > 0) {
    montarRodape(r, formato, s->w, s->h, info, (uint32_t)carga);
    ok = fwrite(r, 1, MINI_RODAPE, f) == MINI_RODAPE;
  } else ok = 0;
  if (fclose(f) != 0) ok = 0;
  if (!ok || rename(tmp, caminho) != 0) { remove(tmp); contar(0, 0, 0, 1, 0, 0, 0); return 0; }
  total = carga + MINI_RODAPE;
  contar(0, 1, 0, 0, 0, ms(t0), total);
  return total;
}
