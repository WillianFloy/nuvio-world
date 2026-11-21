// A TRAVA DE 140-170 MS A CADA CICLO DE SYNC (logs da TCL, 20/11/2026): a conta
// mandava o MESMO JSON das Collections (755 pastas, 3531 fontes),
// definirJson ja o ignorava ("account snapshot identical: kept"), e mesmo
// assim aplicarEstruturaDaConta chamava colfileiras_sincronizar no fio
// principal, que percorre tudo de novo. Com o aplicador de verdade (sync.c):
//   1. Collections novas: reconcilia (uma vez);
//   2. o mesmo JSON de novo: NAO reconcilia, e conta como poupada;
//   3. ordem de catalogos que mudou: reconcilia;
//   4. ordem igual: nao reconcilia.
#define ADDON_PULL_FIXTURE
#define main syncaddons_fixture_main
#define col_revisao fixture_col_revisao
#define colfileiras_sincronizar fixture_colfileiras_sincronizar
#define colfileiras_receber fixture_colfileiras_receber
#include "syncaddons.c"
#undef main
#undef col_revisao
#undef colfileiras_sincronizar
#undef colfileiras_receber
#include <assert.h>

static unsigned revisao;
static int reconciliacoes;
static char ultimo[256];
unsigned col_revisao(void) { return revisao; }
void colfileiras_sincronizar(void) { reconciliacoes++; }
int colfileiras_receber(const char *j) {
  // Como colecoes.c: o mesmo JSON nao muda a revisao.
  if (strcmp(j, ultimo)) { snprintf(ultimo, sizeof ultimo, "%s", j); revisao++; }
  return 1;
}

#include "../src/sync.c"

static void entregarColecoes(const char *json) { colBlob = strdup(json); temColBlob = 1; }
static void entregarOrdem(const char *json) { catHomeBlob = strdup(json); temCatHomeBlob = 1; }

int main(void) {
  const char *A = "{\"collections\":[{\"id\":\"a\",\"title\":\"Servicos\"}]}";
  entregarColecoes(A);
  aplicarEstruturaDaConta(0);
  confere("collections novas: reconcilia uma vez", reconciliacoes == 1);
  entregarColecoes(A);
  aplicarEstruturaDaConta(0);
  confere("o mesmo JSON de novo: nao reconcilia", reconciliacoes == 1);
  confere("e conta como poupada", sync_reconciliacoes_poupadas() == 1);
  entregarColecoes("{\"collections\":[{\"id\":\"b\",\"title\":\"Outra\"}]}");
  aplicarEstruturaDaConta(0);
  confere("collections diferentes: reconcilia", reconciliacoes == 2);
  entregarOrdem("[{\"catalog_order_keys\":[\"x\",\"y\"]}]");
  aplicarEstruturaDaConta(0);
  { int r1 = reconciliacoes;
    entregarOrdem("[{\"catalog_order_keys\":[\"x\",\"y\"]}]");
    aplicarEstruturaDaConta(0);
    confere("ordem igual: nao reconcilia", reconciliacoes == r1);
    entregarOrdem("[{\"catalog_order_keys\":[\"y\",\"x\"]}]");
    aplicarEstruturaDaConta(0);
    confere("ordem que mudou: reconcilia", reconciliacoes == r1 + 1); }
  printf("%s\n", falhas ? "sync_colecoes_iguais: FALHOU" : "sync_colecoes_iguais: tudo ok");
  return falhas != 0;
}
