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
static int achar(const char *chave) { for (int r = 0; r < nFileiras; r++) if (!strcmp(fileiras[r].chave, chave)) return r; return -1; }
static void ir_para(int r) { for (int i = 0; i < 12 && foco.fileira != r; i++) { key(foco.fileira < r ? SDLK_DOWN : SDLK_UP); frames(30); } frames(60); }
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
  enum { ROWS = 4, ITEMS = 24 };
  static CatItem it[ROWS * ITEMS]; static CatFileira rows[ROWS];
  static const int tipos[ROWS] = { FIL_TIPO_CARTAZ, FIL_TIPO_DESTAQUE, FIL_TIPO_PALCO, FIL_TIPO_HEROI_DIVIDIDO };
  for (int r = 0; r < ROWS; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "fixo_%d_movie", r); snprintf(rows[r].titulo, sizeof rows[r].titulo, "Catalogue %d", r + 1);
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie"); rows[r].ini = r * ITEMS; rows[r].n = ITEMS;
    fil_registrar(rows[r].chave, rows[r].titulo, "Offline", "Filmes", ITEMS); assert(fil_definir_tipo(rows[r].chave, tipos[r])); }
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt90%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal 1\nvidroLocal 1\nfocusedPosterBackdropExpandEnabled 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(120); key(SDLK_DOWN); frames(90);
  int rc = achar("fixo_0_movie"), rd = achar("fixo_1_movie"), rp = achar("fixo_2_movie"), rh = achar("fixo_3_movie");
  assert(rc >= 0);
  // 1. Quem tem foco fixo: so NORMAL/POSTER na TV.
  ok(homeFocoFixo(rc), "fileira de cartaz (NORMAL/POSTER) tem foco fixo");
  ok(rd >= 0 && homeFocoFixo(rd), "destaque (estilo escolhido em Ajustes) tambem tem foco fixo (18/11)");
  ok(rp >= 0 && !homeFocoFixo(rp), "palco fica como antes");
  ok(rh >= 0 && !homeFocoFixo(rh), "heroi dividido fica como antes");
  ok(home_foco_fixo_ativo() == (foco.fileira == rc), "home_foco_fixo_ativo segue o foco");
  // 2. (World 22/11) ESPIADA DO ANTERIOR: da coluna 1 em diante o foco fica no mesmo x, `esp` px a
  // direita do titulo, e o cartaz anterior aparece inteiro o bastante para ser reconhecido; o tempo
  // do deslize e o tempo-base (350 ms).
  TXT_INI(); float x0 = homeConteudoX() + bordaX(rc); int N = homeFixoN(rc), total = homeFixoTotal(rc);
  float passo = passoFil(rc), w = larguraFil(rc), esp = vitgeom_espia_ant(w); TXT_FIM();
  ok(N >= 3 && total > N + 3, "a fileira de teste e maior que a tela");
  ok(esp >= 64.0f && esp <= 150.0f && esp > passo - w + 40.0f, "a espiada mostra mais que o vao (o anterior aparece)");
  int fixoOk = 1, tempoOk = 1, anteriorOk = 1, fimOk = 1, kFim = 0;
  for (int k = 1; k <= total - 1; k++) {
    float s0 = scrollX[rc]; key(SDLK_RIGHT); float a = alvo(rc, foco.coluna); int t90 = -1;
    for (int i = 1; i <= 90; i++) { frames(1); float p = (scrollX[rc] - s0) / (a - s0); if (fabsf(a - s0) < 0.01f) p = 1; if (t90 < 0 && p >= 0.9f) t90 = i * 16; }
    float nominal = (float)foco.coluna * passo - esp;
    if (fabsf(a - nominal) < 0.75f) {   // meio da fileira: foco no x fixo, anterior na espiada
      if (fabsf(fx(rc, foco.coluna) - (x0 + esp)) > 0.75f) fixoOk = 0;
      if (fx(rc, foco.coluna - 1) + w < x0 + esp - (passo - w) - 0.75f) anteriorOk = 0;
      if (fabsf(a - s0) > 1.0f && (t90 < 320 || t90 > 380)) tempoOk = 0;
      kFim = k;
    } else {                            // fim: a fileira parou antes, com o ultimo junto da margem
      TXT_INI(); float dirUlt = homeFileiraX(rc, total - 1) + larguraFil(rc), md = homeConteudoDireita(); TXT_FIM();
      if (a > nominal || dirUlt > md + 1.0f || fx(rc, foco.coluna) < x0 + esp - 0.75f) fimOk = 0;
    }
  }
  ok(kFim >= 3, "a navegacao passou por varias colunas no meio da fileira");
  ok(fixoOk, "do 2o cartaz em diante o foco fica no mesmo x (titulo + espiada)");
  ok(anteriorOk, "o cartaz anterior aparece a esquerda do foco (a faixa da espiada)");
  ok(fimOk, "no fim a fileira para (nunca passa do ultimo) e o foco anda para a direita");
  ok(tempoOk, "a fileira chega a 90% em ~350 ms (320..380 ms)");
  // 3. No fim a fileira para (o ultimo cartaz encosta na margem direita) e o foco anda ate ele.
  for (int k = 0; k < total; k++) { key(SDLK_RIGHT); frames(3); } frames(90);
  ok(foco.coluna == foco.nColunas[rc] - 1 || foco.coluna == total - 1, "foco chegou ao ultimo cartaz");
  { TXT_INI(); float dirUlt = homeFileiraX(rc, total - 1) + larguraFil(rc);
    ok(dirUlt <= homeConteudoDireita() + 1.0f && dirUlt >= homeConteudoDireita() - passo, "no fim o ultimo cartaz fica junto da margem direita, sem passar dela"); TXT_FIM(); }
  // 4. Volta ao comeco: foco no 1o cartaz alinhado ao titulo, rolagem zero.
  for (int k = 0; k < total + 2; k++) { key(SDLK_LEFT); frames(3); } frames(80);
  ok(foco.coluna == 0 && fabsf(scrollX[rc]) < 0.75f && fabsf(fx(rc, 0) - x0) < 0.75f, "no inicio: coluna 0, rolagem 0, 1o cartaz alinhado ao titulo");
  key(SDLK_RIGHT); frames(90);
  ok(foco.coluna == 1 && fabsf(fx(rc, 1) - (x0 + esp)) < 0.75f && fx(rc, 0) < x0, "no 2o cartaz a fileira ja anda: o 1o vira a espiada");
  // 5. Descer e subir: a posicao horizontal da fileira e a coluna lembrada sobrevivem.
  for (int k = 0; k < 7; k++) { key(SDLK_RIGHT); frames(3); } frames(80);
  float antes = scrollX[rc]; int colAntes = foco.coluna;
  key(SDLK_DOWN); frames(90);
  ok(foco.fileira != rc && fabsf(scrollX[rc] - antes) < 0.75f, "ao descer a fileira anterior nao se mexe na horizontal");
  key(SDLK_UP); frames(90);
  frames(90);
  { TXT_INI(); float xf = homeFileiraX(rc, foco.coluna); TXT_FIM();
      (void)colAntes;   // o pouso e o #205 (o centro mais perto do cartaz de onde se veio)
    ok(foco.fileira == rc && fabsf(scrollX[rc] - antes) < 0.75f && xf >= x0 - 0.75f,
       "ao subir a fileira nao se mexe e o foco pousa num cartaz inteiro (nunca no da espiada)"); }
  // 6. Outros tipos: rolagem so na borda como antes (os 2 primeiros passos nao rolam).
  { int bate = 1; TXT_INI();
    for (int t = 1; t < 3; t++) { int r = t == 0 ? rd : t == 1 ? rp : rh; if (r < 0) continue;
      for (int c = 0; c < 12; c++) for (int a = 0; a < 2; a++)
        if (fabsf(alvoScrollFil(r, c, a ? 500.0f : 0.0f, 0.0f) - (fileiras[r].tipo == FILEIRA_HEROI_DIVIDIDO ? (float)c * passoFil(r) + passoFil(r)
              : fileiras[r].tipo == FILEIRA_PALCO ? alvoScrollFil(r, c, a ? 500.0f : 0.0f, 0.0f) : regraAntiga(r, c, a ? 500.0f : 0.0f))) > 0.01f) bate = 0; }
    TXT_FIM(); ok(bate, "palco e heroi dividido: alvo da rolagem igual a regra antiga"); }
  // 7. (18/11) TROCAR O ESTILO DA FILEIRA NAO TIRA O FOCO FIXO: cada estilo de Ajustes
  // aplicado a fileira 0, a fileira republicada, e o foco fica no mesmo x navegando.
  { int todos = 1; char falhou[256] = "";
    for (int tp = FIL_TIPO_CARTAZ; tp < FIL_TIPO_N; tp++) {
      if (tp == FIL_TIPO_PALCO || tp == FIL_TIPO_HEROI_DIVIDIDO) continue;
      if (!fil_definir_tipo("fixo_0_movie", tp)) continue;   // estilo que esta fileira nao oferece
      cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS); frames(30);
      int r = achar("fixo_0_movie"); if (r < 0) continue;
      ir_para(r);
      for (int k = 0; k < 8; k++) { key(SDLK_LEFT); frames(2); } frames(120);
      TXT_INI(); int N = homeFixoN(r), total = homeFixoTotal(r); TXT_FIM();
      key(SDLK_RIGHT); frames(90);     // World 22/11: o x fixo e o do 2o cartaz em diante (espiada)
      float x0 = fx(r, foco.coluna);   // (alguns estilos tem recuo proprio)
      int bom = homeFocoFixo(r);
      int passos = total - N - 1 < 6 ? total - N - 1 : 6;
      float dx = 0; int fk = -1;
      for (int k = 1; k <= passos && bom; k++) {
        key(SDLK_RIGHT); frames(90);
        TXT_INI(); float nominal = (float)foco.coluna * passoFil(r) - vitgeom_espia_ant(larguraFil(r)); float at = alvoScrollFil(r, foco.coluna, scrollX[r], 0); TXT_FIM();
        if (fabsf(at - nominal) > 0.75f) { if (k < 3) { bom = 0; fk = k; } break; }   // chegou ao fim da fileira
        dx = fx(r, foco.coluna) - x0; if (foco.fileira != r || fabsf(dx) > 1.0f) { bom = 0; fk = k; } }
      if (!bom) { todos = 0; snprintf(falhou + strlen(falhou), sizeof falhou - strlen(falhou), " %d(t%d fil=%d/%d col=%d k=%d dx=%.0f)", tp, (int)fileiras[r].tipo, foco.fileira, r, foco.coluna, fk, dx); }
    }
    if (!todos) printf("FAIL: (detalhe) estilos sem foco fixo:%s\n", falhou);
    ok(todos, "todo estilo de fileira de Ajustes mantem o foco fixo (o cartaz em foco nao sai do lugar)"); }
  puts(falhas ? "home_focofixo: FALHOU" : "home_focofixo: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
