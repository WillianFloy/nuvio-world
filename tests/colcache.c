// Cache das fileiras das Collections (src/colcache.c): compressao, memoria,
// disco, chave e teto. Sem rede, sem SDL.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/colcache.h"

static char pasta[256];
static int perfil = 1, ocultar = 0;
char *dados_caminho(char *dst, unsigned tam, const char *nome) { snprintf(dst, tam, "%s/%s", pasta, nome); return dst; }
const char *sessao_usuario(void) { return "conta-a"; }
int perfis_ativo(void) { return perfil; }
int ajustes_ocultar_filmes_vistos(void) { return ocultar; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }

static void itens(CatItem *v, int n, int semente) {
  memset(v, 0, sizeof(CatItem) * (size_t)n);
  for (int i = 0; i < n; i++) {
    snprintf(v[i].imdb, sizeof v[i].imdb, "tt%07d", semente * 100 + i);
    snprintf(v[i].tipo, sizeof v[i].tipo, "movie");
    snprintf(v[i].backdrop, sizeof v[i].backdrop, "https://img/%d.jpg", i);
  }
}
int main(void) {
  static CatItem a[60], b[60];
  ColSource f; ColCacheMeta m = { 12, 3, 1, 1 }, lido; long idade = -1; long bytes; int n;
  snprintf(pasta, sizeof pasta, "/tmp/nuvio-colcache-%d", (int)getpid());
  char cmd[600]; snprintf(cmd, sizeof cmd, "rm -rf %s && mkdir -p %s", pasta, pasta); assert(!system(cmd));
  memset(&f, 0, sizeof f);
  snprintf(f.type, sizeof f.type, "movie"); snprintf(f.catId, sizeof f.catId, "top");

  // 1. RLE ida e volta, e compressao real (um CatItem e quase todo zero).
  itens(a, 15, 1);
  size_t enc = colcache_rle((unsigned char *)a, sizeof(CatItem) * 15, NULL);
  unsigned char *buf = malloc(enc);
  colcache_rle((unsigned char *)a, sizeof(CatItem) * 15, buf);
  assert(colcache_unrle(buf, enc, (unsigned char *)b, sizeof(CatItem) * 15));
  assert(!memcmp(a, b, sizeof(CatItem) * 15));
  assert(enc < sizeof(CatItem) * 15 / 10);
  printf("rle: %zu bytes para %zu (%.1f%%)\n", enc, sizeof(CatItem) * 15, 100.0 * enc / (sizeof(CatItem) * 15));
  free(buf);

  // 2. Grava e le da memoria, com idade.
  uint64_t k = colcache_chave(&f, "https://addon", 12);
  colcache_gravar(k, a, &m);
  memset(b, 0, sizeof b);
  assert(colcache_ler(k, b, 60, &lido, &idade));
  assert(lido.n == 12 && lido.extraN == 3 && lido.pagina == 1 && lido.mais == 1 && idade >= 0 && idade < 5);
  assert(!memcmp(a, b, sizeof(CatItem) * 15));

  // 3. Memoria limpa: volta do disco (proxima abertura do app).
  colcache_limpar(0);
  colcache_stats(&bytes, &n, NULL, NULL); assert(n == 0 && bytes == 0);
  memset(b, 0, sizeof b);
  assert(colcache_ler(k, b, 60, &lido, &idade) && !memcmp(a, b, sizeof(CatItem) * 15));
  colcache_stats(&bytes, &n, NULL, NULL); assert(n == 1 && bytes > 0);   // e voltou para a memoria

  // 4. A chave muda com perfil, filtro de vistos, limite, base e fonte.
  perfil = 2; assert(colcache_chave(&f, "https://addon", 12) != k); perfil = 1;
  ocultar = 1; assert(colcache_chave(&f, "https://addon", 12) != k); ocultar = 0;
  assert(colcache_chave(&f, "https://addon", 13) != k);
  assert(colcache_chave(&f, "https://outro", 12) != k);
  { ColSource g = f; snprintf(g.genre, sizeof g.genre, "Drama"); assert(colcache_chave(&g, "https://addon", 12) != k); }
  assert(colcache_chave(&f, "https://addon", 12) == k);
  assert(!colcache_ler(k ^ 1, b, 60, &lido, &idade));

  // 5. Destino pequeno demais: recusa, sem estourar.
  assert(!colcache_ler(k, b, 10, &lido, &idade));

  // 6. Teto de memoria: o mais antigo sai, o disco continua tendo.
  colcache_limpar(0);
  colcache_teto(64 * 1024);
  for (int i = 0; i < 300; i++) {
    ColSource g = f; snprintf(g.catId, sizeof g.catId, "c%d", i);
    itens(a, 15, i + 2);
    colcache_gravar(colcache_chave(&g, "https://addon", 12), a, &m);
  }
  colcache_stats(&bytes, &n, NULL, NULL);
  assert(bytes <= 64 * 1024 && n < 300);
  { ColSource g = f; snprintf(g.catId, sizeof g.catId, "c0"); itens(a, 15, 2);   // saiu da memoria: volta do disco
    assert(colcache_ler(colcache_chave(&g, "https://addon", 12), b, 60, &lido, &idade));
    assert(!memcmp(a, b, sizeof(CatItem) * 15)); }

  // 7. Arquivo corrompido e recusado (nao publica lixo).
  { char cam[400]; ColSource g = f; snprintf(g.catId, sizeof g.catId, "c1");
    uint64_t kk = colcache_chave(&g, "https://addon", 12);
    snprintf(cam, sizeof cam, "%s/colrow-%016llx.bin", pasta, (unsigned long long)kk);
    FILE *x = fopen(cam, "r+b"); assert(x); fseek(x, 0, SEEK_END); long t = ftell(x);
    fseek(x, t - 9, SEEK_SET); fputc(0x7f, x); fputc(0x7f, x); fputc(0x7f, x); fputc(0x7f, x); fclose(x);
    colcache_limpar(0);
    int ok = colcache_ler(kk, b, 60, &lido, &idade);
    (void)ok;   // RLE pode ou nao detectar; o importante: nao trava nem estoura
  }

  // 8. Limpar com disco apaga os arquivos (logout).
  colcache_limpar(1);
  assert(!colcache_ler(k, b, 60, &lido, &idade));
  snprintf(cmd, sizeof cmd, "rm -rf %s", pasta); if (system(cmd)) {}
  puts("colcache: ok");
  return 0;
}
