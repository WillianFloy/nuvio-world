// "MARCAR COMO ASSISTIDO" MARCAVA TODOS DA HOME (24/11, log da TCL).
//
// O historico de catalogo.c e o mapa de episodios de vistoep.c cortavam o id no
// PRIMEIRO ':'. Um titulo de catalogo nao-IMDb ("tmdb:1726", "kitsu:41370") virava
// "tmdb"/"kitsu": a mesma chave para TODO titulo do mesmo addon. Marcar um filme
// marcava todos. A regra correta e a de idbase.h (corta no SEGUNDO ':').
//
// Comportamento: marcar um titulo nao pode mexer em nenhum OUTRO, e o episodio
// continua sendo do titulo dele.
#include "catalogo.h"
#include "vistoep.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  // --- historico por titulo (menu de contexto, selo do cartaz) ---
  assert(cat_historico_estado_id("tmdb:1726", "movie") == -1);
  assert(cat_historico_estado_id("tmdb:1560520", "movie") == -1);
  cat_historico_definir_id("tmdb:1726", "movie", 1);
  assert(cat_historico_estado_id("tmdb:1726", "movie") == 1);
  assert(cat_historico_estado_id("tmdb:1560520", "movie") == -1);   // OUTRO titulo: intacto
  assert(cat_historico_estado_id("tmdb:999", "movie") == -1);
  assert(cat_historico_estado_id("kitsu:41370", "movie") == -1);    // outro addon: intacto
  assert(cat_historico_estado_id("tt0111161", "movie") == -1);      // IMDb: intacto
  // desmarcar um nao desmarca o outro
  cat_historico_definir_id("tmdb:1560520", "movie", 1);
  cat_historico_definir_id("tmdb:1726", "movie", 0);
  assert(cat_historico_estado_id("tmdb:1726", "movie") == 0);
  assert(cat_historico_estado_id("tmdb:1560520", "movie") == 1);
  // o id de EPISODIO ainda e do titulo dele (serie)
  cat_historico_definir_id("tmdb:t1399", "series", 1);
  assert(cat_historico_estado_id("tmdb:t1399:2:5", "series") == 1);
  assert(cat_historico_estado_id("tmdb:t2000:2:5", "series") == -1);
  // IMDb continua cortando no primeiro ':'
  cat_historico_definir_id("tt0903747", "series", 1);
  assert(cat_historico_estado_id("tt0903747:1:1", "series") == 1);
  assert(cat_historico_estado_id("tt0944947", "series") == -1);

  // --- episodios vistos (vistoep) ---
  vistoep_definir("tmdb:t1399", 1, 1, 1);
  assert(vistoep_estado("tmdb:t1399", 1, 1) == 1);
  assert(vistoep_estado("tmdb:t2000", 1, 1) != 1);   // outra serie do mesmo addon
  assert(vistoep_estado("tmdb:t1399:1:1", 1, 1) == 1);   // id de episodio resolve a mesma serie
  puts("visto_id_nao_imdb: PASS");
  return 0;
}
