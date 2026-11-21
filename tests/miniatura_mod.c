// MINIATURAS LOCAIS, so o modulo (miniatura.c): roda no CI, que nao tem
// SDL2_image. O caminho completo, com o tex_cache real, e tests/miniatura.sh.
#include "../src/miniatura.h"
#include "../src/jpegrapido.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static char DIR_[160];
static SDL_Surface *cartaz(int w, int h, int comAlfa) {
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR8888);
  int x, y;
  assert(s);
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      unsigned char *p = (unsigned char *)s->pixels + (size_t)y * s->pitch + (size_t)x * 4;
      p[0] = (unsigned char)(x * 255 / w);
      p[1] = (unsigned char)(y * 255 / h);
      p[2] = (unsigned char)(((x / 40) + (y / 40)) % 2 ? 200 : 60);
      p[3] = comAlfa ? (unsigned char)((x * 7 + y * 3) % 256) : 255;
    }
  return s;
}
static double diferencaMedia(SDL_Surface *a, SDL_Surface *b) {
  long soma = 0, n = 0;
  int x, y, c;
  assert(a->w == b->w && a->h == b->h);
  for (y = 0; y < a->h; y++)
    for (x = 0; x < a->w; x++) {
      const unsigned char *p = (const unsigned char *)a->pixels + (size_t)y * a->pitch + (size_t)x * 4;
      const unsigned char *q = (const unsigned char *)b->pixels + (size_t)y * b->pitch + (size_t)x * 4;
      for (c = 0; c < 4; c++) { soma += abs((int)p[c] - (int)q[c]); n++; }
    }
  return (double)soma / (double)n;
}
static void criterios(void) {
  const unsigned char png[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  const unsigned char jpg[4] = {0xFF, 0xD8, 0xFF, 0xE0};
  const unsigned char gif[6] = {'G', 'I', 'F', '8', '9', 'a'};
  unsigned char webp[32], webpAnim[32];
  memset(webp, 0, sizeof webp); memcpy(webp, "RIFF", 4); memcpy(webp + 8, "WEBPVP8 ", 8);
  memset(webpAnim, 0, sizeof webpAnim); memcpy(webpAnim, "RIFF", 4); memcpy(webpAnim + 8, "WEBPVP8X", 8);
  webpAnim[20] = 0x02;
  assert(mini_vale_gravar(1000, 1500, 192, 288, 20, png, 8));
  assert(mini_vale_gravar(2560, 1440, 320, 180, 20, jpg, 4));
  assert(mini_vale_gravar(3840, 2160, 1280, 720, 20, webp, 32));
  assert(!mini_vale_gravar(1000, 1500, 192, 288, 900, gif, 6));
  assert(!mini_vale_gravar(3840, 2160, 1280, 720, 900, webpAnim, 32));
  webpAnim[20] = 0x10;                       // VP8X com alfa, parado: vale
  assert(mini_vale_gravar(3840, 2160, 1280, 720, 20, webpAnim, 32));
  assert(!mini_vale_gravar(250, 375, 192, 288, 10, png, 8));   // pouco a ganhar
  assert(mini_vale_gravar(250, 375, 192, 288, 80, png, 8));    // mas lento: vale
  assert(!mini_vale_gravar(3840, 2160, 3840, 2160, 900, jpg, 4)); // tela cheia 4K: nao
  puts("ok  criterio: GIF e WebP animado nunca; pequeno e rapido nao; reduzido ou lento sim");
}
static void transparencia(void) {
  char cam[700];
  SDL_Surface *a = cartaz(416, 138, 1), *b;
  MiniInfo mi, lido;
  int corr = 0;
  memset(&mi, 0, sizeof mi);
  mi.srcW = 1200; mi.srcH = 400; mi.limite = 416; memcpy(mi.magica, "\x89PNG", 4);
  mini_nome(DIR_, "https://exemplo/logo.png", 416, cam, sizeof cam);
  assert(mini_gravar(cam, a, &mi) == 416L * 138 * 4 + 32);
  b = mini_ler(cam, 416, &lido, &corr);
  assert(b && !corr && diferencaMedia(a, b) == 0.0);
  assert(lido.srcW == 1200 && lido.srcH == 400 && !memcmp(lido.magica, "\x89PNG", 4));
  SDL_FreeSurface(b);
  // Outro teto com o mesmo nome (colisao): nao serve, mas nao e apagada.
  assert(!mini_ler(cam, 480, &lido, &corr) && !corr && access(cam, F_OK) == 0);
  // Um byte trocado no rodape: invalida, apagada.
  { FILE *f = fopen(cam, "r+b"); assert(f); fseek(f, -10, SEEK_END); fputc('x', f); fclose(f); }
  assert(!mini_ler(cam, 416, &lido, &corr) && corr && access(cam, F_OK) != 0);
  SDL_FreeSurface(a);
  puts("ok  logo com transparencia volta exato; rodape adulterado apaga o arquivo");
}

static void opaca(void) {
  char cam[700];
  SDL_Surface *a = cartaz(320, 180, 0), *b;
  MiniInfo mi, lido;
  int corr = 0;
  long n;
  memset(&mi, 0, sizeof mi);
  mi.srcW = 2560; mi.srcH = 1440; mi.limite = 320; mi.limTam = 0; memcpy(mi.magica, "\x89PNG", 4);
  mini_nome(DIR_, "https://exemplo/pasta.png", 320, cam, sizeof cam);
  n = mini_gravar(cam, a, &mi);
  assert(n > 0);
  b = mini_ler(cam, 320, &lido, &corr);
  assert(b && !corr && b->w == 320 && b->h == 180);
  assert(lido.srcW == 2560 && lido.srcH == 1440 && lido.limite == 320);
  printf("     opaca %s: %ld bytes, diferenca media %.2f\n", jpeg_rapido_codificador() ? "JPEG" : "crua", n, diferencaMedia(a, b));
  assert(diferencaMedia(a, b) < 3.0);
  // Truncada: apagada.
  assert(truncate(cam, n / 2) == 0);
  SDL_FreeSurface(b);
  assert(!mini_ler(cam, 320, &lido, &corr) && corr && access(cam, F_OK) != 0);
  SDL_FreeSurface(a);
  puts("ok  arte opaca vai e volta com o tamanho do original; truncada e apagada");
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  snprintf(DIR_, sizeof DIR_, "%s/nuvio-miniatura-mod-%d", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", (int)getpid());
  mkdir(DIR_, 0755);
  criterios();
  transparencia();
  opaca();
  { char cmd[300]; snprintf(cmd, sizeof cmd, "rm -rf '%s'", DIR_); if (system(cmd)) {} }
  puts("miniatura (modulo): tudo ok");
  return 0;
}
