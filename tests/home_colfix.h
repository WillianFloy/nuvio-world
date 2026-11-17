// Fixture comum dos testes das Collections da Home (2.0.43): um grupo do perfil com N pastas
// (fontes TMDB diretas, utilizaveis sem addon), no home.c de verdade. GLES2 por software.
// Incluido DEPOIS de "../src/home.c". NAO prova o desempenho da Mali-G31.
#ifndef HOME_COLFIX_H
#define HOME_COLFIX_H
#include <SDL2/SDL_image.h>
#include <assert.h>
#include "../src/colfileiras.h"
static SDL_Window *window; static Uint32 now = 1000; static int falhas;
static void key(SDL_Keycode code) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = code; home_evento(&e); e.type = SDL_KEYUP; home_evento(&e); }
static void frames(int n) { for (int i = 0; i < n; i++) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar(); } }
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
static void shot(const char *pre, const char *fase) { if (!pre) return; char path[1024]; unsigned char *px = malloc(1920*1080*4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32); glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, px);
  for (int y = 0; y < 1080; y++) memcpy((char*)s->pixels + y*s->pitch, px + (size_t)(1079-y)*1920*4, 1920*4);
  snprintf(path, sizeof path, "%s-%s.png", pre, fase); IMG_SavePNG(s, path); SDL_FreeSurface(s); free(px); }
static float fx(int r, int c) { TXT_INI(); float x = homeFileiraX(r, c); TXT_FIM(); return x; }
static float alvo(int r, int c) { TXT_INI(); float t = alvoScrollFil(r, c, scrollX[r], 0); TXT_FIM(); return t; }
static int achar(const char *chave) { for (int r = 0; r < nFileiras; r++) if (!strcmp(fileiras[r].chave, chave)) return r; return -1; }
// Sobe a Home com um grupo "Themes" ("Streaming" some da Home na Dinamica: vira secao da barra) de `pastas` pastas (a 1a com logo, a 2a sem; a 3a "Directors"),
// mais uma fileira de cartazes embaixo. `layout`: 0 Moderna, 1 Padrao, 2 Dinamica. Devolve a fileira do grupo.
static int colfix_iniciar(int layout, int pastas, int hero_transicao) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0); IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  window = SDL_CreateWindow("h", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window && SDL_GL_CreateContext(window));
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1)); dados_iniciar("deploy/app/art");
  tex_iniciar(64); gfx_icones_dir("deploy/app/art"); artehero_definir_falhou(tex_falhou);
  assert(home_iniciar("deploy/app/art")); fil_definir_limite(20);
  AddonRemoto addon = {.nome = "Offline", .url = "local", .ativo = 1}; assert(addons_definir_lista(&addon, 1));
  enum { ROWS = 2, ITEMS = 24 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "fixo_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, FIL_TIPO_CARTAZ)); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt90%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal %d\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\nheroTransicaoLocal %d\n", layout, hero_transicao); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  static char json[16384]; int n = 0;
  n += snprintf(json + n, sizeof json - n, "{\"collections\":[{\"id\":\"cx\",\"title\":\"Themes\",\"folders\":[");
  for (int i = 0; i < pastas; i++) {
    char hero[96]; snprintf(hero, sizeof hero, i == 5 ? "deploy/app/art/nao-existe.jpg" : "deploy/app/art/%02d.jpg", (i * 3) % 20);
    // A pasta 9 tem logo configurada que NAO existe (falha de vez); as demais i%3==0 tem a logo de verdade.
    char logo[160]; snprintf(logo, sizeof logo, ",\"titleLogoUrl\":\"deploy/app/art/marcas/%s.png\"", i == 9 ? "logo-que-nao-existe" : "logo-novo-horizontal");
    n += snprintf(json + n, sizeof json - n,
      "%s{\"id\":\"f%d\",\"title\":\"Pasta %d\",\"coverImageUrl\":\"deploy/app/art/poster/%02d.jpg\",\"heroBackdropUrl\":\"%s\"%s,"
      "\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"mediaType\":\"MOVIE\",\"title\":\"x%d\"}]}",
      i ? "," : "", i, i + 1, i % 8, hero,
      i % 3 == 0 ? logo : "", i);
  }
  n += snprintf(json + n, sizeof json - n, "]}]}");
  { int rcv = colfileiras_receber(json); printf("[fixture] colfileiras_receber=%d col_n=%d\n", rcv, col_n()); assert(rcv > 0); } colfileiras_sincronizar();
  home_ir_topo(); frames(150);
  int r = -1; for (int q = 0; q < nFileiras; q++) if (fileiras[q].tipo == FILEIRA_CATALOGOS) r = q;
  if (r < 0) for (int q = 0; q < nFileiras; q++) printf("[fixture] fileira %d chave=%s tipo=%d n=%d\n", q, fileiras[q].chave, (int)fileiras[q].tipo, fileiras[q].n);
  return r;
}
#endif
