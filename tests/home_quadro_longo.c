// B4 (2.0.46) — UM QUADRO LONGO NAO E UMA AUSENCIA. Teste de COMPORTAMENTO com o home.c de verdade (GLES2 por software):
// estado + posicao dos cartoes quadro a quadro. NAO prova nada sobre a Mali-G31.
//
// O sintoma (B4, 21/11): na 1a descida de algumas fileiras a fileira de cima ia ~9 px PARA BAIXO no 1o quadro e so depois
// subia. Causa: `reentra` (cascata de entrada) media a ausencia da Home so pelo RELOGIO (> 1,5 s sem desenhar). Um unico
// quadro longo (texturas, GC, o GL por software do harness leva ~1,7 s no 1o quadro) parecia uma volta de outra tela:
// toda fileira visivel recomecava a entrada (cartao 22 px abaixo, subindo em 300 ms) em cima da rolagem.
//   1. quadro longo com a Home sendo a tela: NENHUMA fileira recomeca a cascata e o topo das fileiras nunca anda para baixo;
//   2. ausencia de verdade (outra tela desenha entre dois quadros da Home, > 1,5 s): a cascata continua recomecando;
//   3. a Home PARADA por muito tempo (quadros seguidos, mas > 10 s) tambem continua recomecando.
#include "../src/home.c"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <unistd.h>
static SDL_Window *window; static Uint32 now = 1000; static int falhas;
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
static void quadro(Uint32 extra, int desenhaHome) {
  now += 16 + extra; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); if (desenhaHome) home_desenhar(now); gfx_ambiente_descarregar(); }
static void frames(int n) { for (int i = 0; i < n; i++) quadro(0, 1); }
static void tecla(SDL_Keycode k) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = k; home_evento(&e); e.type = SDL_KEYUP; home_evento(&e); }
static int entrando(void) { for (int r = 0; r < MAX_FIL; r++) if (filEntraEm[r]) return 1; return 0; }

// y do topo das fileiras (menor y entre os cartoes) em cada um dos n quadros apos DOWN; o quadro `longo` leva `extra` ms.
static int topos(int n, int longo, Uint32 extra, float *y) {
  static char nome[512]; snprintf(nome, sizeof nome, "%s/rastro.txt", getenv("NUVIO_DADOS"));
  fflush(stdout); int salvo = dup(1); FILE *fo = freopen(nome, "w", stdout); assert(fo);
  home_rastro_cartoes = 1; int emCascata = 0;
  printf("[q]\n"); quadro(0, 1);   // quadro de REFERENCIA, antes da tecla: y[0]
  tecla(SDLK_DOWN);
  for (int i = 1; i < n; i++) { printf("[q]\n"); quadro(i == longo ? extra : 0, 1); if (entrando()) emCascata = 1; }
  home_rastro_cartoes = 0; fflush(stdout); dup2(salvo, 1); close(salvo); clearerr(stdout);
  FILE *fi = fopen(nome, "r"); assert(fi); char l[256]; int q = -1; float min = 1e9f;
  for (int i = 0; i < n; i++) y[i] = 1e9f;
  while (fgets(l, sizeof l, fi)) {
    if (!strncmp(l, "[q]", 3)) { q++; continue; }
    float x0, y0, w, h; int foco;
    if (q >= 0 && q < n && sscanf(l, "[cartao] %f %f %f %f foco=%d", &x0, &y0, &w, &h, &foco) == 5 && y0 < y[q]) y[q] = y0; }
  fclose(fi); (void)min; return emCascata; }
static float maiorSubida(const float *y, int n) {   // o maior passo PARA BAIXO (y crescendo) entre quadros com cartoes
  float m = 0; for (int i = 1; i < n; i++) if (y[i] < 1e8f && y[i - 1] < 1e8f && y[i] - y[i - 1] > m) m = y[i] - y[i - 1]; return m; }

static void preparar(void) {
  home_ir_topo(); frames(150);
  tecla(SDLK_DOWN); frames(90);     // destaque -> 1a fileira
  tecla(SDLK_RIGHT); frames(60);
  ok(!focoHero && foco.fileira == 0, "o foco esta na 1a fileira, assentado");
  ok(!entrando(), "nenhuma cascata de entrada em curso antes do teste"); }

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
  AddonRemoto addon = {.nome = "Fixture", .url = "http://127.0.0.1:1/manifest.json", .ativo = 1}; assert(addons_definir_lista(&addon, 1));
  enum { ROWS = 3, ITEMS = 12 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "btn_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Fixture", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, FIL_TIPO_CARTAZ)); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt92%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1);
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal %s\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n", getenv("NV_LAYOUT") ? getenv("NV_LAYOUT") : "1"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));

  static float y[40];
  // 0. controle: descida normal, sem quadro longo
  preparar();
  int casc0 = topos(24, -1, 0, y); float sub0 = maiorSubida(y, 24);
  printf("controle (sem quadro longo): maior passo para baixo %.1f px, cascata=%d, y[0..5]=%.1f %.1f %.1f %.1f %.1f %.1f\n", sub0, casc0, y[0], y[1], y[2], y[3], y[4], y[5]);
  ok(!casc0 && sub0 <= 1.0f, "controle: a descida e monotonica e nao recomeca a cascata");
  // 1. quadro longo (1,8 s) logo no 1o quadro da descida
  tecla(SDLK_UP); frames(120); preparar();
  int cascL = topos(24, 1, 1800, y); float subL = maiorSubida(y, 24);
  printf("quadro longo de 1,8 s: maior passo para baixo %.1f px, cascata=%d, y[0..3]=%.1f %.1f %.1f %.1f\n", subL, cascL, y[0], y[1], y[2], y[3]);
  ok(!cascL, "quadro longo com a Home na tela: nenhuma fileira recomeca a cascata de entrada");
  ok(subL <= 1.0f, "quadro longo: o topo das fileiras nunca anda para baixo (antes: ~9 px no 1o quadro)");
  // 2. ausencia de verdade: outra tela desenha por 2 s e a Home volta
  frames(120);
  quadro(0, 0); quadro(2000, 0); quadro(0, 0);
  quadro(0, 1);
  ok(entrando(), "ausencia real (outra tela por 2 s): a cascata de entrada recomeca na volta");
  // 3. Home parada por mais de 10 s (quadros seguidos)
  frames(120); ok(!entrando(), "a cascata anterior terminou");
  quadro(11000, 1);
  ok(entrando(), "Home sem quadros por > 10 s: a cascata recomeca");
  printf("\n%s\n", falhas ? "home_quadro_longo: FALHOU" : "home_quadro_longo: ok");
  return falhas ? 1 : 0;
}
