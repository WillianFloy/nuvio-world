// World (22/11): a API do IMDb respondendo 403 a TODA consulta nao pode virar uma tempestade de pedidos (log da TCL: ~400
// em 2 min). Sem rede: rede_baixar_com e substituido e conta as chamadas. Comportamento:
//   1. sem resposta -> o titulo conta como respondido (o hero/detalhe seguem para a proxima fonte);
//   2. pedir de novo o mesmo titulo logo em seguida NAO faz outra consulta;
//   3. 3 titulos seguidos sem resposta pausam o IMDb: um 4o titulo nao consulta;
//   4. o pedido vai ao endereco que ainda responde (caching.graphql.imdb.com) com o cabecalho que ele exige.
#include "../src/trailerimdb.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
static atomic_int chamadas; static int temUA, falhas;
char *rede_baixar_com(const char *url, int s, const char *const *cab) {
  (void)url; (void)s;
  for (int i = 0; cab && cab[i]; i++) if (!strcmp(cab[i], "x-imdb-client-name: imdb-web-next")) temUA = 1;
  if (strncmp(url, "https://caching.graphql.imdb.com/", 33)) temUA = 0;
  atomic_fetch_add(&chamadas, 1); return NULL; }   // HTTP 403: rede_baixar_com devolve NULL
char *rede_baixar(const char *url, int s) { (void)url; (void)s; return NULL; }
char *dados_caminho(char *dst, unsigned tam, const char *nome) { (void)dst; (void)tam; (void)nome; return NULL; }
void dados_marcar_sujo(int leve) { (void)leve; }
const char *dados_dir(void) { return NULL; }
void tex_orcamento_info(int *mb, long *memTotal, int *fixo, int *slots) { (void)mb; (void)fixo; (void)slots; if (memTotal) *memTotal = 0; }
int ajustes_trailer_qualidade(void) { return 0; }
int memlog_android_fraco(void) { return 0; }
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
static void espera(const char *id) { for (int i = 0; i < 100 && !trailerimdb_respondeu(id); i++) usleep(10000); usleep(20000); }
int main(void) {
  trailerimdb_pedir("tt0000001"); espera("tt0000001");
  ok(trailerimdb_respondeu("tt0000001") && !trailerimdb_url("tt0000001", NULL), "sem resposta: respondido e sem trailer");
  ok(temUA, "o pedido vai a caching.graphql.imdb.com com x-imdb-client-name (api.graphql responde 403)");
  for (int i = 0; i < 50; i++) { trailerimdb_pedir("tt0000001"); usleep(2000); }
  ok(atomic_load(&chamadas) == 1, "50 pedidos seguidos do mesmo titulo: 1 consulta so");
  ok(trailerimdb_respondeu("tt0000001"), "continua respondido (o hero nao fica esperando)");
  trailerimdb_pedir("tt0000002"); espera("tt0000002");
  trailerimdb_pedir("tt0000003"); espera("tt0000003");
  int antes = atomic_load(&chamadas);
  trailerimdb_pedir("tt0000004"); espera("tt0000004");
  printf("consultas: %d antes do 4o titulo, %d depois\n", antes, atomic_load(&chamadas));
  ok(antes == 3 && atomic_load(&chamadas) == 3, "3 falhas seguidas pausam o IMDb: o 4o titulo nao consulta");
  ok(trailerimdb_respondeu("tt0000004"), "titulo pedido na pausa conta como respondido");
  printf("%s\n", falhas ? "trailerimdb_falha: FALHOU" : "trailerimdb_falha: ok");
  return falhas != 0;
}
