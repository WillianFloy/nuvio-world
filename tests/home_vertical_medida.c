// MEDIDA (nao e guarda): o que acontece nas trocas VERTICAIS de fileira (poster/Collection). Antes: o destaque cruza (nao corta) quando o foco vai de um poster comum para uma Collection e de volta. home.c de verdade.
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
// Luminancia media da arte (lado direito, faixa de cima da tela) e do bloco de texto (lado esquerdo, y do logo/sinopse).
static double lumReg(const unsigned char *a, int x0, int x1, int y0, int y1) {   // y em px de TELA (de cima para baixo)
  double s = 0; long n = 0;
  for (int y = y0; y < y1; y += 2) for (int x = x0; x < x1; x += 2) { const unsigned char *q = a + ((1079 - y) * 1920 + x) * 4; s += 0.3*q[0] + 0.59*q[1] + 0.11*q[2]; n++; }
  return s / n; }
static double lumArteIni, lumArteFim, lumArteMin, lumTxtMin, lumTxtMax; static int txtSobeDepoisDeCair;
static unsigned char guarda[1920*1080*4];
// Roda ate o foco assentar medindo o passo maximo quadro a quadro e a diferenca total; devolve m intermediarios vistos.
static void mede(int tecla, int quadros, long *passoMax, long *total, int *intermed) {
  desenhar_um(); memcpy(guarda, captura(), sizeof guarda); unsigned char *ini = malloc(sizeof guarda); memcpy(ini, guarda, sizeof guarda);
  lumArteIni = lumReg(guarda, 1150, 1800, 120, 420); lumArteMin = 1e9; lumTxtMin = 1e9; lumTxtMax = 0;
  key(tecla); *passoMax = 0; *intermed = 0;
  for (int i = 0; i < quadros; i++) { now += 16; tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar();
    unsigned char *c = captura(); { double la = lumReg(c, 1150, 1800, 120, 420), lt = lumReg(c, 110, 900, 140, 560); if (la < lumArteMin) lumArteMin = la; if (lt < lumTxtMin) lumTxtMin = lt; if (lt > lumTxtMax) lumTxtMax = lt; }
    if (getenv("NV_SHOTS") && i % 3 == 0 && i < 36 && quadros > 8) { char fn[256]; snprintf(fn, sizeof fn, "%s_%d_%02d.rgba", getenv("NV_SHOTS"), tecla, i); FILE *fo = fopen(fn, "wb"); if (fo) { fwrite(c, 1, 1920*1080*4, fo); fclose(fo); } }
    long d = faixa(c, guarda); if (d > *passoMax) *passoMax = d; memcpy(guarda, c, sizeof guarda);
    if (heroModoPasta > 0.05f && heroModoPasta < 0.95f) (*intermed)++; }
  lumArteFim = lumReg(guarda, 1150, 1800, 120, 420); *total = faixa(guarda, ini); free(ini); }

// Espera pelo modo novo: enquanto ele nao esta pronto, fica o que ja esta na tela, INTEIRO (sem escurecer), e a troca so
// comeca quando pronto — ou ao estourar NV_HERO_ESPERA_MS.
static void espera(int tecla, unsigned segura, int *quadroInicio, double *desvioMax) {
  heroXTesteSegura = segura;
  desenhar_um(); unsigned char *c0 = captura(); double l0 = lumReg(c0, 1150, 1800, 120, 420);
  key(tecla); *quadroInicio = -1; *desvioMax = 0;
  for (int i = 0; i < 60; i++) { now += 16; tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar();
    double l = lumReg(captura(), 1150, 1800, 120, 420);
    if (*quadroInicio < 0 && (heroModoPasta > 0.02f && heroModoPasta < 0.98f)) *quadroInicio = i;
    if (*quadroInicio < 0 && fabs(l - l0) > *desvioMax) *desvioMax = fabs(l - l0); }
  heroXTesteSegura = 0; }

typedef struct { float sy, fy, la, lt; long d; float m; int heroAtual, fil; } Q;
static const char *nomeTipo(int r) { return r < 0 || r >= nFileiras ? "?" : fileiras[r].tipo == FILEIRA_CATALOGOS ? "COLLECTION" : fileiras[r].tipo == FILEIRA_SOCIAL ? "social" : "poster"; }
static void medir(int a, int b, int tecla, const char *rotulo) {
  // vai ate a fileira a (sem medir) e assenta
  for (int i = 0; i < 20 && foco.fileira != a; i++) { key(foco.fileira < a ? SDLK_DOWN : SDLK_UP); frames(70); }
  frames(90);
  if (foco.fileira != a || focoHero) { printf("[pulo] %s: nao consegui parar na fileira %d (foco=%d)\n", rotulo, a, foco.fileira); return; }
  enum { N = 60 };
  static Q q[N + 1];
  desenhar_um(); unsigned char *c0 = captura(); memcpy(guarda, c0, sizeof guarda);
  q[0] = (Q){ scrollY, itemFoco.rect.y, (float)lumReg(guarda, 1150, 1800, 120, 420), (float)lumReg(guarda, 110, 900, 140, 560), 0, heroModoPasta, heroAtual, foco.fileira };
  const int heroAntes = heroAtual;
  key(tecla);
  for (int i = 1; i <= N; i++) {
    now += 16; tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar();
    unsigned char *c = captura();
    q[i] = (Q){ scrollY, itemFoco.rect.y, (float)lumReg(c, 1150, 1800, 120, 420), (float)lumReg(c, 110, 900, 140, 560), faixa(c, guarda), heroModoPasta, heroAtual, foco.fileira };
    memcpy(guarda, c, sizeof guarda); }
  // rolagem e cartao focado
  float totS = q[N].sy - q[0].sy, maxS = 0, recS = 0, totF = q[N].fy - q[0].fy, maxF = 0, recF = 0;
  int assS = 0, assF = 0, qMaxS = 0, nS = 0; float semS = 0;
  for (int i = 1; i <= N; i++) {
    float ds = q[i].sy - q[i-1].sy, df = q[i].fy - q[i-1].fy;
    if (fabsf(ds) > maxS) { maxS = fabsf(ds); qMaxS = i; }
    if (fabsf(df) > maxF) maxF = fabsf(df);
    if (totS != 0 && ds * totS < 0 && fabsf(ds) > recS) recS = fabsf(ds);
    if (totF != 0 && df * totF < 0 && fabsf(df) > recF) recF = fabsf(df);
    if (fabsf(ds) < 0.5f) { if (++nS >= 3 && !assS) assS = i - 2; } else nS = 0;
  }
  { int n = 0; for (int i = 1; i <= N; i++) { if (fabsf(q[i].fy - q[i-1].fy) < 0.5f) { if (++n >= 3 && !assF) assF = i - 2; } else n = 0; } }
  // destaque (arte + texto)
  float la0 = q[0].la, la1 = q[N].la, laMin = 1e9; long soma = 0, pico = 0; int ini = -1, fim = -1;
  for (int i = 1; i <= N; i++) { if (q[i].la < laMin) laMin = q[i].la; soma += q[i].d; if (q[i].d > pico) pico = q[i].d; if (q[i].d > 40000 && ini < 0) ini = i; if (q[i].d > 40000) fim = i; }
  float ltMin = 1e9, ltMax = 0; for (int i = 0; i <= N; i++) { if (q[i].lt < ltMin) ltMin = q[i].lt; if (q[i].lt > ltMax) ltMax = q[i].lt; }
  const float baixo = fminf(la0, la1);
  printf("\n=== %s: fileira %d (%s) -> %d (%s), tecla %s ===\n", rotulo, a, nomeTipo(a), foco.fileira, nomeTipo(foco.fileira), tecla == SDLK_DOWN ? "BAIXO" : "CIMA");
  printf("  rolagem da Home : %+7.1f px no total | maior passo %5.1f px (quadro %d) | recuo (sentido contrario) %4.1f px | assenta em %d quadros (%.0f ms)\n", totS, maxS, qMaxS, recS, assS, assS * 16.7f);
  printf("  cartao focado   : y %6.1f -> %6.1f (%+.1f px) | maior passo %5.1f px | recuo %4.1f px | assenta em %d quadros (%.0f ms)\n", q[0].fy, q[N].fy, totF, maxF, recF, assF, assF * 16.7f);
  printf("  destaque (arte) : luminancia %5.1f -> %5.1f | minimo no meio %5.1f (%s %.0f%% abaixo do menor extremo)\n", la0, la1, laMin, laMin < baixo * 0.95f ? "BURACO" : "sem buraco", baixo > 0 ? (1 - laMin / baixo) * 100 : 0);
  printf("  destaque (texto): luminancia min %5.1f / max %5.1f (extremos %5.1f e %5.1f)\n", ltMin, ltMax, q[0].lt, q[N].lt);
  printf("  mudanca de pixels do destaque: comeca no quadro %d (%.0f ms), termina no %d (%.0f ms) | pico %.1f%% da soma\n", ini, ini * 16.7f, fim, fim * 16.7f, soma ? 100.0 * pico / soma : 0);
  printf("  destaque troca de titulo: %s | modo pasta: %.2f -> %.2f\n", q[N].heroAtual != heroAntes ? "sim" : "nao", q[0].m, q[N].m);
  frames(60);
}
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
  for (int i = 0; i < ROWS * ITEMS; i++) { snprintf(it[i].imdb, sizeof it[i].imdb, "tt91%05d", i); snprintf(it[i].tipo, sizeof it[i].tipo, "movie");
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture synopsis for the featured area.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  #define PASTA(id, t, hero, cover) "{\"id\":\"" id "\",\"title\":\"" t "\",\"heroBackdropUrl\":\"deploy/app/art/" hero ".jpg\",\"coverImageUrl\":\"deploy/app/art/poster/" cover ".jpg\",\"sources\":[{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"title\":\"Popular\"}]}"
  assert(col_definir_json("{\"collections\":["
    "{\"id\":\"c1\",\"title\":\"Streaming\",\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":[" PASTA("a","Pasta A","03","12") "," PASTA("b","Pasta B","05","14") "]},"
    "{\"id\":\"c2\",\"title\":\"Generos\",\"backdropImageUrl\":\"deploy/app/art/09.jpg\",\"folders\":[" PASTA("c","Pasta C","11","16") "," PASTA("d","Pasta D","13","18") "]}]}") == 4);
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal %s\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n", getenv("NV_LAYOUT") ? getenv("NV_LAYOUT") : "1"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  home_ir_topo(); frames(150);
  printf("layout=%d | fileiras:", ajustes_home_layout()); for (int r = 0; r < nFileiras; r++) printf(" %d=%s", r, nomeTipo(r)); printf("\n");
  // pares adjacentes (a, a+1) de cada combinacao
  int pp = -1, cc = -1, pc = -1, cp = -1;
  for (int r = 0; r + 1 < nFileiras; r++) {
    int A = fileiras[r].tipo == FILEIRA_CATALOGOS, B = fileiras[r+1].tipo == FILEIRA_CATALOGOS;
    int sA = fileiras[r].tipo == FILEIRA_SOCIAL, sB = fileiras[r+1].tipo == FILEIRA_SOCIAL;
    if (sA || sB) continue;
    if (!A && !B && pp < 0) pp = r; if (A && B && cc < 0) cc = r; if (!A && B && pc < 0) pc = r; if (A && !B && cp < 0) cp = r; }
  printf("pares adjacentes: poster-poster=%d collection-collection=%d poster-collection=%d collection-poster=%d\n", pp, cc, pc, cp);
  if (pp >= 0) { medir(pp, pp + 1, SDLK_DOWN, "POSTER -> POSTER (descendo)"); medir(pp + 1, pp, SDLK_UP, "POSTER -> POSTER (subindo)"); }
  if (cc >= 0) { medir(cc, cc + 1, SDLK_DOWN, "COLLECTION -> COLLECTION (descendo)"); medir(cc + 1, cc, SDLK_UP, "COLLECTION -> COLLECTION (subindo)"); }
  if (pc >= 0) { medir(pc, pc + 1, SDLK_DOWN, "POSTER -> COLLECTION (descendo)"); medir(pc + 1, pc, SDLK_UP, "COLLECTION -> POSTER (subindo)"); }
  if (cp >= 0) { medir(cp, cp + 1, SDLK_DOWN, "COLLECTION -> POSTER (descendo)"); medir(cp + 1, cp, SDLK_UP, "POSTER -> COLLECTION (subindo)"); }
  return 0;
}
