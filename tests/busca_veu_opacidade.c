// VEU DA BUSCA (24/11), medido em pixels: sobre branco puro o veu tem de escurecer de forma
// perceptivel e moderada. (Resto do cabecalho herdado de home_itemfoco_arte.c, nao se aplica.)
// Antes aqui: o menu de segurar OK e a transicao leem
// home_item_focado() DEPOIS do quadro. A arte vinha de um buffer estatico que
// o laco reescreve a cada card, entao o menu abria com a arte do ULTIMO card
// pintado ("Jogos Vorazes" no lugar do titulo do Batman). Teste de
// COMPORTAMENTO: cada titulo tem arte propria; apos o quadro, o item focado
// devolve a arte DELE, em qualquer coluna e fileira, em repouso e com o cartaz
// expandido (a deitada tambem precisa ser a dele no cartao repintado).
#include "../src/home.c"
#include "../src/ctxmenu.h"
#include "../src/spotlight.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
static SDL_Window *window; static Uint32 now = 1000; static int falhas;
static void key(SDL_Keycode code) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = code; home_evento(&e); e.type = SDL_KEYUP; home_evento(&e); }
static void frames(int n) { for (int i = 0; i < n; i++) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar(); } }
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
enum { ITEMS = 24, NART = 8 };
static CatItem it[ITEMS]; static CatFileira rows[1];
static float lum(int x, int y) { unsigned char p[4]; glReadPixels(x, 1079 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p); return (0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2]); }
static void quadro(void) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); home_desenhar(now); ctx_atualizar(1.f / 300, now); ctx_desenhar(now); gfx_ambiente_descarregar(); }
int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0); IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  window = SDL_CreateWindow("h", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window && SDL_GL_CreateContext(window));
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1)); dados_iniciar("deploy/app/art");
  tex_iniciar(64); gfx_icones_dir("deploy/app/art"); artehero_definir_falhou(tex_falhou);
  assert(home_iniciar("deploy/app/art"));
  AddonRemoto addon = {.nome = "Offline", .url = "local", .ativo = 1}; assert(addons_definir_lista(&addon, 1));
  snprintf(rows[0].chave, sizeof rows[0].chave, "arte_0_movie"); snprintf(rows[0].titulo, sizeof rows[0].titulo, "Catalogue");
  snprintf(rows[0].tipo, sizeof rows[0].tipo, "movie"); rows[0].ini = 0; rows[0].n = ITEMS;
  fil_registrar(rows[0].chave, rows[0].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[0].chave, FIL_TIPO_CARTAZ));
  for (int i = 0; i < ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt90%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % NART);
    snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", (i + 3) % NART); }
  cat_definir_tudo(it, ITEMS, rows, 1);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal 1\nvidroLocal 0\nfocusedPosterBackdropExpandEnabled 0\ntrailerHero 0\nsocialLocal 0\nidioma 0\n"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(120); key(SDLK_DOWN); frames(90);
  for (int k = 0; k < 3; k++) { key(SDLK_RIGHT); frames(20); } frames(60);

  // 1. VEU DA BUSCA: sobre fundo claro, escurece de forma perceptivel (>= 25 %) e
  //    moderada (a imagem mantem >= 45 % do brilho: os cartazes nao somem).
  { float antes, depois;
    glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT); gfx_sem_recorte(); antes = lum(900, 500);
    gfx_novo_quadro(); gfx_opacidade_grupo = 1.0f; spot_veu(); gfx_ambiente_descarregar(); depois = lum(900, 500);
    printf("[veu] branco %.0f -> %.0f (%.0f%% do brilho)\n", antes, depois, 100.0f * depois / antes);
    ok(depois / antes <= 0.75f, "o veu da Busca escurece de forma perceptivel (>= 25 %)");
    ok(depois / antes >= 0.45f, "o veu da Busca e moderado: os cartazes seguem visiveis (>= 45 % do brilho)"); }

  puts(falhas ? "busca_veu_opacidade: FALHOU" : "busca_veu_opacidade: ok");
  return falhas != 0;
}
