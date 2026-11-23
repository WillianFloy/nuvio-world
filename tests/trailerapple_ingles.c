// World (22/11): a Apple (loja dos EUA) com titulo do TMDB em INGLES. Sem rede: o TMDB e a Apple sao respostas falsas.
// Log da TCL: "Sobrenatural" -> candidatos=0. A mesma serie como "Supernatural" acha e tem trailer.
//   1. "tmdb:1622" com titulo em portugues: a busca vai pelo titulo em ingles e o trailer (HLS) e achado;
//   2. a busca pelo titulo em portugues sozinha (sem titulo em ingles) continua sem trailer (controle);
//   3. id "tt..." (sem TMDB) usa o titulo de sempre.
#include "../src/trailerapple.h"
#include "../src/trailertt.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int falhas; static char buscas[8][200]; static int nBuscas;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
#define SHOW(t) "{\"shelves\":[{\"items\":[{\"type\":\"Show\",\"id\":\"umc.cmc.sup\",\"title\":\"" t "\",\"releaseDate\":1126569600000}]}]}"
char *rede_baixar_com(const char *url, int s, const char *const *cab) {
  (void)s; (void)cab;
  const char *q = strstr(url, "searchTerm=");
  if (q) { if (nBuscas < 8) snprintf(buscas[nBuscas++], 200, "%s", q + 11);
    if (strstr(q, "Supernatural")) return strdup(SHOW("Supernatural"));
    if (strstr(q, "The%20Matrix") || strstr(q, "The+Matrix")) return strdup("{\"shelves\":[{\"items\":[{\"type\":\"Movie\",\"id\":\"umc.cmc.mx\",\"title\":\"The Matrix\",\"releaseDate\":922838400000}]}]}");
    return strdup("{\"shelves\":[]}"); }
  if (strstr(url, "/shows/umc.cmc.sup") || strstr(url, "/movies/umc.cmc.mx"))
    return strdup("{\"data\":{\"content\":{\"backgroundVideo\":{\"assets\":{\"hlsUrl\":\"https://play.itunes.apple.com/x/master.m3u8\"}}}}}");
  if (strstr(url, "master.m3u8")) return strdup("#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1280x720,CODECS=\"avc1.640020,mp4a.40.2\"\nhttps://play.itunes.apple.com/x/720.m3u8\n");
  return NULL; }
char *rede_baixar(const char *url, int s) { (void)url; (void)s; return NULL; }
static char *tmdb(const char *url, int s) { (void)s;
  if (strstr(url, "/tv/1622?")) return strdup("{\"genres\":[{\"name\":\"Drama\"}],\"name\":\"Supernatural\",\"external_ids\":{\"imdb_id\":\"tt0460681\"}}");
  return NULL; }
const char *desc_chave_tmdb_reserva(void) { return "K"; }
static char pasta[256];
char *dados_caminho(char *dst, unsigned tam, const char *nome) { snprintf(dst, tam, "%s/%s", pasta, nome); return dst; }
void dados_marcar_sujo(int leve) { (void)leve; }
int ajustes_trailer_qualidade(void) { return 0; }
static void espera(const char *id) { for (int i = 0; i < 300 && !trailerapple_respondeu(id); i++) SDL_Delay(10); }
int main(void) {
  snprintf(pasta, sizeof pasta, "%s/nuvio-apple-ingles-XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  if (!mkdtemp(pasta)) { perror("mkdtemp"); return 1; }
  trailertt_testes(tmdb, "K");
  trailerapple_pedir("tmdb:1622", "Sobrenatural", "2005 · Drama", 1); espera("tmdb:1622");
  printf("buscas: %d (%s%s%s)\n", nBuscas, buscas[0], nBuscas > 1 ? ", " : "", nBuscas > 1 ? buscas[1] : "");
  ok(trailerapple_respondeu("tmdb:1622"), "tmdb: a Apple respondeu");
  ok(trailerapple_url("tmdb:1622") != NULL, "tmdb: 'Sobrenatural' acha o trailer pelo titulo em ingles 'Supernatural'");
  ok(nBuscas >= 1 && strstr(buscas[0], "Supernatural"), "tmdb: a 1a busca e pelo titulo em ingles");
  nBuscas = 0;
  trailerapple_pedir("tt9999999", "Sobrenatural", "2005", 1); espera("tt9999999");
  ok(trailerapple_respondeu("tt9999999") && !trailerapple_url("tt9999999"), "controle: so com o titulo em portugues nao ha trailer");
  ok(nBuscas >= 1 && strstr(buscas[0], "Sobrenatural"), "controle: id tt busca pelo titulo de sempre");
  nBuscas = 0;
  trailerapple_pedir("tt0133093", "The Matrix", "1999", 0); espera("tt0133093");
  ok(trailerapple_url("tt0133093") != NULL, "id tt com titulo que a loja tem: continua achando");
  printf("%s\n", falhas ? "trailerapple_ingles: FALHOU" : "trailerapple_ingles: ok");
  return falhas != 0;
}
