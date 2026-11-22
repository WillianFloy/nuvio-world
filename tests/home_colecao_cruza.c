// World 22/11: o destaque cruza (nao corta) quando o foco vai de um poster comum para uma Collection e de volta. home.c de verdade.
// GLES2 por software (SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1): prova a logica
// (posicao, tempo, fim da fileira, outros tipos), NAO o desempenho do Mali-G31.
#include "../src/home.c"
#include <SDL2/SDL_image.h>
#include <assert.h>
static SDL_Window *window; static Uint32 now = 1000; static int falhas;
static void key(SDL_Keycode code) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = code; home_evento(&e); e.type = SDL_KEYUP; home_evento(&e); }
static void frames(int n) { for (int i = 0; i < n; i++) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar(); } }
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
static float fx(int r, int c) { TXT_INI(); float x = homeFileiraX(r, c); TXT_FIM(); return x; }
static float alvo(int r, int c) { TXT_INI(); float t = alvoScrollFil(r, c, scrollX[r], 0); TXT_FIM(); return t; }
// A regra ANTIGA (rolar so na borda), copiada de alvoScrollFil antes da 2.0.41: os tipos que
// nao ganharam foco fixo tem de continuar devolvendo exatamente isto.
static float regraAntiga(int r, int col, float atual) {
  float passo = passoFil(r), esq = (float)col * passo, dir = esq + larguraFil(r) + xOffTipo(fileiras[r].tipo);
  float util = homeLarguraUtil(), folga = sobraDireitaFoco(r, 0.0f), a = atual;
  if (dir + folga - a > util) a = dir + folga - util;
  if (esq - folga - a < 0.0f) a = esq - folga;
  return a < 0 ? 0 : a;
}

static unsigned char *captura(void) { static unsigned char a[1920 * 1080 * 4], b[1920 * 1080 * 4]; static int k;
  unsigned char *p = (k++ & 1) ? b : a; glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, p); return p; }
// Pixels diferentes entre duas capturas e a caixa que os contem (px do GL: y de baixo para cima).
static int difere(const unsigned char *a, const unsigned char *b, int *x0, int *x1, int *y0, int *y1) {
  int n = 0; *x0 = *y0 = 1 << 30; *x1 = *y1 = -1;
  for (int y = 0; y < 1080; y++) for (int x = 0; x < 1920; x++) {
    const unsigned char *pa = a + (y * 1920 + x) * 4, *pb = b + (y * 1920 + x) * 4;
    if (abs(pa[0] - pb[0]) + abs(pa[1] - pb[1]) + abs(pa[2] - pb[2]) > 24) {
      n++; if (x < *x0) *x0 = x; if (x > *x1) *x1 = x; if (y < *y0) *y0 = y; if (y > *y1) *y1 = y; } }
  return n; }
static void desenhar_um(void) { now += 16; tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(0.0f, now); home_desenhar(now); gfx_ambiente_descarregar(); }

// Soma de |dif| na faixa do destaque (metade de cima), entre a captura atual e a anterior.
static long faixa(const unsigned char *a, const unsigned char *b) {
  long s = 0; for (int y = 540; y < 1080; y += 3) for (int x = 0; x < 1920; x += 3) { const unsigned char *pa = a + (y*1920+x)*4, *pb = b + (y*1920+x)*4;
    s += abs(pa[0]-pb[0]) + abs(pa[1]-pb[1]) + abs(pa[2]-pb[2]); } return s; }
static unsigned char guarda[1920*1080*4];
// Roda ate o foco assentar medindo o passo maximo quadro a quadro e a diferenca total; devolve m intermediarios vistos.
static void mede(int tecla, int quadros, long *passoMax, long *total, int *intermed) {
  desenhar_um(); memcpy(guarda, captura(), sizeof guarda); unsigned char *ini = malloc(sizeof guarda); memcpy(ini, guarda, sizeof guarda);
  key(tecla); *passoMax = 0; *intermed = 0;
  for (int i = 0; i < quadros; i++) { now += 16; tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar();
    unsigned char *c = captura(); long d = faixa(c, guarda); if (d > *passoMax) *passoMax = d; memcpy(guarda, c, sizeof guarda);
    if (heroModoPasta > 0.05f && heroModoPasta < 0.95f) (*intermed)++; }
  *total = faixa(guarda, ini); free(ini); }
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
  enum { ROWS = 2, ITEMS = 12 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "btn_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, FIL_TIPO_CARTAZ)); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt91%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture synopsis for the featured area.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  assert(col_definir_json("{\"collections\":[{\"id\":\"cs\",\"title\":\"Streaming\",\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":["
    "{\"id\":\"a\",\"title\":\"Pasta A\",\"heroBackdropUrl\":\"deploy/app/art/03.jpg\",\"coverImageUrl\":\"deploy/app/art/poster/12.jpg\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"title\":\"Popular\"}]},"
    "{\"id\":\"b\",\"title\":\"Pasta B\",\"heroBackdropUrl\":\"deploy/app/art/05.jpg\",\"coverImageUrl\":\"deploy/app/art/poster/14.jpg\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"title\":\"Popular\"}]}]}]}") == 2);
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal 1\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(150);
  int rc = -1; for (int r = 0; r < nFileiras; r++) if (fileiras[r].tipo == FILEIRA_CATALOGOS) rc = r;
  printf("[medida] nFileiras=%d col_n=%d:", nFileiras, col_n()); for (int r = 0; r < nFileiras; r++) printf(" %d(t%d,n%d)", r, fileiras[r].tipo, fileiras[r].n); printf("\n");
  ok(rc > 0, "a Home tem uma fileira de Collection");
  long passo, total; int inter;
  // Desce ate a ultima fileira de poster (a de antes da Collection).
  for (int i = 0; i < 10 && (focoHero || foco.fileira < rc - 1); i++) { key(SDLK_DOWN); frames(60); }
  printf("[medida] foco: hero=%d fileira=%d coluna=%d rc=%d m=%.2f\n", focoHero, foco.fileira, foco.coluna, rc, heroModoPasta);
  ok(!focoHero && foco.fileira == rc - 1 && pastaFocoAtual() < 0 && heroModoPasta < 0.01f, "foco no poster comum: destaque de titulo assentado");
  // 1. poster -> Collection
  mede(SDLK_DOWN, 40, &passo, &total, &inter);
  printf("[medida] poster->colecao: passo max %ld total %ld intermediarios %d\n", passo, total, inter);
  ok(pastaFocoAtual() >= 0 && heroModoPasta > 0.99f, "chegou na Collection: destaque de pasta assentado");
  ok(inter >= 4, "poster->Collection passa por varios quadros intermediarios (nao pula de 0 a 1)");
  ok(total > 0 && passo < total * 0.6, "poster->Collection: nenhum quadro concentra a mudanca (nao e corte seco)");
  // 2. Collection -> poster
  mede(SDLK_UP, 40, &passo, &total, &inter);
  printf("[medida] colecao->poster: passo max %ld total %ld intermediarios %d\n", passo, total, inter);
  ok(pastaFocoAtual() < 0 && heroModoPasta < 0.01f, "voltou ao poster: destaque de titulo assentado");
  ok(inter >= 4, "Collection->poster tambem passa por quadros intermediarios");
  ok(total > 0 && passo < total * 0.6, "Collection->poster: nenhum quadro concentra a mudanca");
  // 3. Animacoes reduzidas: direto.
  anim_politica_reduzida = 1;
  mede(SDLK_DOWN, 8, &passo, &total, &inter);
  ok(inter == 0 && heroModoPasta > 0.99f, "animacoes reduzidas: troca direto, sem intermediarios");
  anim_politica_reduzida = 0;
  printf("home_colecao_cruza: %s\n", falhas ? "FALHOU" : "ok");
  return falhas ? 1 : 0;
}
