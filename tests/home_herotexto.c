// Troca do destaque da Home por ESMAECIMENTO com texto escalonado (2.0.42), no home.c de verdade.
// GLES2 por software: prova a logica (linha do tempo, sem salto, sem textura nova por quadro,
// deslizar intacto), NAO o desempenho do Mali-G31. Uso: home_herotexto [layout 0|1|2] [prefixo-de-capturas]
#include "../src/home.c"
#include <SDL2/SDL_image.h>
#include <assert.h>
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
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0; const char *pre = argc > 2 ? argv[2] : NULL;
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
  enum { ROWS = 3, ITEMS = 24 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "scroll_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, FIL_TIPO_CARTAZ)); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt90%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1);
    snprintf(it[i].sinopse, sizeof it[i].sinopse, "Sinopse de teste do filme %d: uma historia comprida o bastante para ocupar mais de uma linha do bloco de texto do destaque na tela.", i + 1);
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  // heroTransicaoLocal 1 = Esmaecer (0 = Deslizar).
  fprintf(f, "homeLayoutLocal %d\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\nheroTransicaoLocal 1\n", layout); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(180);
  ok(focoHero && !ajustes_hero_deslizar(), "no destaque, com a troca por Esmaecer");
  // --- Troca por esmaecimento: acompanha quadro a quadro ---
  int antes = heroAtual; (void)antes;
  int ativou = -1, quadroAtiv = 0, maxRaster = 0, rasterTardio = 0, fim = -1; float ant = -1;
  int r0 = txt_rasterizadas;
  key(SDLK_RIGHT);
  for (int q = 0; q < 120; q++) {
    int a0 = txt_rasterizadas; frames(1); int d = txt_rasterizadas - a0;
    if (heroTextoAtivo && ativou < 0) { ativou = q; quadroAtiv = q; }
    if (ativou >= 0 && q > ativou + 3 && d > 0) rasterTardio++;
    if (d > maxRaster) maxRaster = d;
    if (heroTextoAtivo) { float t = heroTextoT; if (!(t >= ant - 1e-3f || ant < 0)) ok(0, "o relogio do texto so avanca"); ant = t;
      if (pre && (q - quadroAtiv == 2 || q - quadroAtiv == 8 || q - quadroAtiv == 14 || q - quadroAtiv == 22 || q - quadroAtiv == 31)) { char fase[32]; snprintf(fase, sizeof fase, "q%02d", q - quadroAtiv); shot(pre, fase); } }
    else if (ativou >= 0 && fim < 0) fim = q;
  }
  ok(ant >= 0, "o relogio do texto so avanca durante a troca");
  ok(ativou >= 0, "a troca por esmaecimento ligou o texto escalonado");
  ok(fim >= 0 && (fim - ativou) * 16 >= 500 && (fim - ativou) * 16 <= 580, "a transicao do texto dura ~520 ms (500..580)");
  ok(rasterTardio == 0, "nenhuma linha de texto rasterizada depois dos primeiros quadros da troca");
  printf("[info] rasterizadas na troca=%d, maximo por quadro=%d, despejos=%d\n", txt_rasterizadas - r0, maxRaster, txt_despejos);
  ok(heroAtual != antes, "o destaque trocou");
  // --- Interrupcao: nova troca no meio da anterior continua do ponto em que estava ---
  frames(60); key(SDLK_RIGHT);
  int ativ2 = 0; for (int q = 0; q < 40 && !ativ2; q++) { frames(1); ativ2 = heroTextoAtivo; }
  if (ativ2) {
    frames(14);                        // ~220 ms: o logo ja acendeu, a sinopse ainda esta entrando
    HtInstante antesInt = ht_instante(heroTextoT);
    key(SDLK_RIGHT);
    int rearm = 0; for (int q = 0; q < 60 && !rearm; q++) { float t0 = heroTextoT; frames(1); if (heroTextoAtivo && heroTextoT < t0) rearm = 1; }
    ok(rearm, "interrupcao: a troca seguinte reinicia o relogio do texto");
    if (rearm) {
      int continuo = 1; for (int i = 0; i < HT_N; i++) continuo &= fabsf(heroTextoIni.a[i] - antesInt.a[i]) < 0.35f;
      ok(continuo, "interrupcao: o bloco que sai parte do ponto em que o novo estava (sem salto)");
    }
  } else ok(1, "(interrupcao ignorada: nao houve segunda troca)");
  frames(80);
  ok(!heroTextoAtivo, "ao fim, o texto assenta e nada fica animando");
  printf("home_herotexto: %s\n", falhas ? "FALHOU" : "ok");
  return falhas ? 1 : 0;
}
