// Trailers com id do TMDB (World 21/11): o "tt" do IMDb vem do TMDB, uma vez por titulo.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "trailertt.h"
static int chamadas; static char ultimaUrl[400];
static char *falso(const char *url, int s) {
  (void)s; chamadas++; snprintf(ultimaUrl, sizeof ultimaUrl, "%s", url);
  if (strstr(url, "/movie/1560520/")) return strdup("{\"id\":1560520,\"imdb_id\":\"tt31193180\",\"facebook_id\":null}");
  if (strstr(url, "/tv/1434/")) return strdup("{\"id\":1434,\"imdb_id\":\"tt0182576\",\"tvdb_id\":75978}");
  if (strstr(url, "/movie/999/")) return strdup("{\"id\":999,\"imdb_id\":null}");
  return NULL;
}
static const char *esperar(const char *id, int serie) {
  const char *r = trailertt_id(id, serie);
  for (int i = 0; i < 200 && !trailertt_respondeu(id); i++) usleep(5000);
  r = trailertt_id(id, serie);
  return r;
}
int main(void) {
  const char *r;
  trailertt_testes(falso, "CHAVE");
  // 1. "tt" passa direto, sem rede.
  r = trailertt_id("tt0111161", 0); assert(!strcmp(r, "tt0111161") && chamadas == 0);
  // 2. Filme do TMDB: devolve o proprio id enquanto pergunta, depois o tt.
  r = trailertt_id("tmdb:1560520", 0); assert(!strcmp(r, "tmdb:1560520"));
  r = esperar("tmdb:1560520", 0); assert(!strcmp(r, "tt31193180"));
  assert(strstr(ultimaUrl, "/3/movie/1560520/external_ids?api_key=CHAVE"));
  // 3. Serie vai a /tv/.
  r = esperar("tmdb:1434", 1); r = trailertt_id("tmdb:1434", 1); assert(!strcmp(r, "tt0182576"));
  assert(strstr(ultimaUrl, "/3/tv/1434/"));
  // 4. Uma consulta por titulo, por mais que o quadro pergunte.
  { int antes = chamadas;
    for (int i = 0; i < 500; i++) { trailertt_id("tmdb:1560520", 0); trailertt_id("tmdb:1434", 1); }
    assert(chamadas == antes); }
  // 5. O TMDB nao tem tt: responde (o IMDb nao fica esperando) e nao pergunta de novo tao cedo.
  trailertt_id("tmdb:999", 0); r = esperar("tmdb:999", 0);
  assert(!strcmp(r, "tmdb:999") && trailertt_respondeu("tmdb:999"));
  { int antes = chamadas; trailertt_id("tmdb:999", 0); assert(chamadas == antes); }
  // 6. Rede falhou: tambem responde (negativa), nao trava o botao.
  trailertt_id("tmdb:4242", 0); esperar("tmdb:4242", 0); assert(trailertt_respondeu("tmdb:4242"));
  // 7. Ids que nao sao do TMDB ficam como estao e contam como respondidos.
  assert(!strcmp(trailertt_id("kitsu:41370", 1), "kitsu:41370") && trailertt_respondeu("kitsu:41370"));
  // 8. Sem chave do TMDB: nao chama a rede e responde.
  trailertt_testes(falso, ""); { int antes = chamadas;
    trailertt_id("tmdb:1560520", 0); esperar("tmdb:1560520", 0);
    assert(chamadas == antes && trailertt_respondeu("tmdb:1560520")); }
  puts("trailertt: ok");
  return 0;
}
