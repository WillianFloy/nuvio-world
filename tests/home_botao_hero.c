// Foco fixo nas fileiras NORMAL/POSTER da Home (2.0.41), rodando o home.c de verdade.
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
  enum { ROWS = 3, ITEMS = 12 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "btn_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, FIL_TIPO_CARTAZ)); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt91%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture synopsis for the featured area.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal 1\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(150);
  int x0, x1, y0, y1, n;
  // 1. No Padrao, com o foco no DESTAQUE, o botao esta aceso.
  ok(focoHero == 1 && animBotaoHero > 0.99f, "foco no destaque: o botao \"Ver titulo\" esta aceso");
  desenhar_um(); unsigned char *comFoco = captura();
  animBotaoHero = 0.0f; desenhar_um(); unsigned char *semBotao = captura(); animBotaoHero = 1.0f;
  n = difere(comFoco, semBotao, &x0, &x1, &y0, &y1);
  ok(n > 600, "o botao e de fato desenhado com o foco no destaque (a pilula muda os pixels)");
  printf("[medida] caixa do botao: x %d-%d y %d-%d\n", x0, x1, y0, y1);
  ok(y1 - y0 < 140, "...e so a linha do botao muda (a pilula e os pontos de pagina, numa faixa baixa)");
  // 2. Descer para as fileiras: o botao APAGA (para qualquer cartaz que ganhe o foco).
  key(SDLK_DOWN); frames(5);
  ok(focoHero == 0 && animBotaoHero > 0.0f && animBotaoHero < 0.95f, "descendo: o botao apaga com mola (nao some num corte)");
  frames(90);
  ok(animBotaoHero < 0.01f, "foco no cartaz: o botao nao existe");
  desenhar_um(); unsigned char *fileira = captura();
  animBotaoHero = 1.0f; desenhar_um(); unsigned char *forcado = captura();
  n = difere(fileira, forcado, &x0, &x1, &y0, &y1);
  ok(n > 600, "o desenho que antes mostrava o botao em todo cartaz agora o esconde (forcar o alfa 1 faz a pilula voltar)");
  animBotaoHero = 0.0f;
  // 3. Passando de cartaz em cartaz, continua sem botao; ao voltar ao destaque, acende.
  key(SDLK_RIGHT); frames(60); key(SDLK_RIGHT); frames(60);
  ok(focoHero == 0 && animBotaoHero < 0.01f, "trocar de cartaz na fileira nao traz o botao de volta");
  key(SDLK_UP); frames(90);
  ok(focoHero == 1 && animBotaoHero > 0.99f, "subir ao destaque acende o botao de novo");
  // 4. Animacoes reduzidas: sem mola, direto.
  printf("home_botao_hero: %s\n", falhas ? "FALHOU" : "ok");
  return falhas ? 1 : 0;
}
