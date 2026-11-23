// B5 (2.0.46) — ABRIR e FECHAR o detalhe a partir de um poster da Home (Moderna e Padrao). Teste de COMPORTAMENTO com o
// home.c e o detail.c de verdade, em GLES2 por software: estado + pixels, quadro a quadro, na ordem de app.c (Home, depois
// Detalhe). NAO prova nada sobre a Mali-G31 (tempo/fps) nem sobre a percepcao na TV.
//   1. nenhuma translacao vertical: o cartaz focado nao anda (antes: a Home descia 86 px);
//   2. sem quadro morto: a luminancia da tela nunca cai abaixo de 88% do menor dos extremos;
//   3. a Home aparece desde o comeco da volta (antes: o backdrop a escondia ate t ~ 0,33 e ela surgia de uma vez);
//   4. a ancora vertical do backdrop (GFX_DETALHE) bate com a da Home do Padrao (GFX_VITRINE, 0,30): antes a arte
//      saltava ~130 px na vertical porque uma recortava em 0,30 e a outra em 0,50;
//   5. foco e posicao preservados ao voltar; preenchimento por quadro dentro do orcamento.
#include "../src/home.c"
#include <SDL2/SDL_image.h>
#include <assert.h>
static SDL_Window *window; static Uint32 now = 1000; static int falhas;
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
static unsigned char *captura(void) { static unsigned char a[1920 * 1080 * 4], b[1920 * 1080 * 4]; static int k;
  unsigned char *p = (k++ & 1) ? b : a; glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, p); return p; }
static double lumReg(const unsigned char *a, int x0, int x1, int y0, int y1) {   // y em px de TELA (de cima para baixo)
  double s = 0; long n = 0;
  for (int y = y0; y < y1; y += 2) for (int x = x0; x < x1; x += 2) { const unsigned char *q = a + ((1079 - y) * 1920 + x) * 4; s += 0.3*q[0] + 0.59*q[1] + 0.11*q[2]; n++; }
  return s / n; }
static double dist(const unsigned char *a, const unsigned char *b, int y0, int y1) {   // |lum a - lum b| medio na faixa
  double s = 0; long n = 0;
  for (int y = y0; y < y1; y += 3) for (int x = 0; x < 1920; x += 3) { const unsigned char *p = a + ((1079 - y) * 1920 + x) * 4, *q = b + ((1079 - y) * 1920 + x) * 4;
    s += fabs((0.3*p[0] + 0.59*p[1] + 0.11*p[2]) - (0.3*q[0] + 0.59*q[1] + 0.11*q[2])); n++; }
  return s / n; }
static void frames(int n) { for (int i = 0; i < n; i++) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); home_desenhar(now); gfx_ambiente_descarregar(); } }
static int passo(void) {   // um quadro, na ordem de app.c: Home (se o detalhe nao cobre) -> Detalhe
  now += 16; tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now);
  if (!detail_aberto() && home_pediu_abrir()) { HomeItem it; if (home_item_focado(&it)) detail_abrir(&it); }
  detail_atualizar(1.f / 60, now);
  const int cobre = detail_cobre_tela();
  itemFoco.rect.w = 0;
  if (!cobre) home_desenhar(now);
  detail_desenhar(now);
  gfx_ambiente_descarregar();
  return cobre; }
static void ev(SDL_Keycode k) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = k; if (detail_aberto()) detail_evento(&e); else home_evento(&e); e.type = SDL_KEYUP; if (detail_aberto()) detail_evento(&e); else home_evento(&e); }
typedef struct { float pd, fill, lum, y, anc, ancHome; int cobre, temCartao; } QD;
float detail_ancora_dy(void);
// A faixa das fileiras (y 700..1000) de cada quadro, em luminancia 1/3 de resolucao: as distancias ate a Home e ate o
// detalhe ASSENTADOS saem depois, comparando com a Home de DEPOIS da volta (a de antes tem estado de animacao diferente).
enum { BW = 640, BH = 100, BN = BW * BH };
static unsigned char banda[2][90][BN], bandaHome[BN], bandaDet[BN];
static void tiraBanda(const unsigned char *c, unsigned char *dst) {
  for (int j = 0; j < BH; j++) for (int i = 0; i < BW; i++) { const unsigned char *q = c + ((1079 - (700 + j * 3)) * 1920 + i * 3) * 4;
    dst[j * BW + i] = (unsigned char)(0.3 * q[0] + 0.59 * q[1] + 0.11 * q[2]); } }
static double distB(const unsigned char *a, const unsigned char *b) { double s = 0; for (int i = 0; i < BN; i++) s += abs(a[i] - b[i]); return s / BN; }
static unsigned char homeP[1920*1080*4], detP[1920*1080*4];
static const char *tagDump = "x";
static void rodar(int n, QD *q, int sentido) {
  for (int i = 0; i < n; i++) {
    int cobre = passo(); unsigned char *c = captura();
    if (getenv("NV_SHOTS") && (i < 20 || i % 4 == 0) && i < 40) { char fn[256]; snprintf(fn, sizeof fn, "%s_%s_%02d.rgba", getenv("NV_SHOTS"), tagDump, i); FILE *fo = fopen(fn, "wb"); if (fo) { fwrite(c, 1, 1920*1080*4, fo); fclose(fo); } }
    tiraBanda(c, banda[sentido][i]);
    q[i] = (QD){ detail_progresso(), gfx_fill, (float)lumReg(c, 0, 1920, 0, 1080), itemFoco.rect.y, detail_ancora_dy(), home_hero_ancora(), cobre, itemFoco.rect.w > 8 }; } }

// 4. O recorte do GFX_DETALHE com a ancora da Home tem de ser o do GFX_VITRINE (a mesma arte, o mesmo retangulo, sem veu).
static void testeAncora(void) {
  const char *arte = "deploy/app/art/03.jpg"; GLuint tex = 0;
  for (int i = 0; i < 400 && !tex; i++) { tex_bombear(16); tex_novo_quadro(); tex = tex_obter_hero(arte); if (!tex) SDL_Delay(5); }
  ok(tex != 0, "ancora: a arte de teste carregou");
  if (!tex) return;
  GfxRect r = { 0, 0, 1920, 621 };
  unsigned char *img[3]; static unsigned char cp[3][1920*1080*4];
  for (int k = 0; k < 3; k++) {
    gfx_novo_quadro(); gfx_sem_recorte(); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    gfx_tex_aspect_atual = tex_aspecto(arte);
    if (k == 0) gfx_rect(r, tex, GFX_VITRINE, 0.0f, 0.30f, 0.0f, 0.0f, 0, 0, 0, 1.0f);
    else gfx_rect(r, tex, GFX_DETALHE, 0.0f, 0.0f, k == 1 ? (0.30f - 0.5f) : 0.0f, 0.0f, 0, 0, 0, 1.0f);
    gfx_tex_aspect_atual = 0.0f; gfx_ambiente_descarregar();
    memcpy(cp[k], captura(), 1920*1080*4); img[k] = cp[k]; }
  double dIgual = dist(img[0], img[1], 20, 600), dOutra = dist(img[0], img[2], 20, 600);
  printf("ancora: VITRINE(0,30) x DETALHE(ancora 0,30) = %.2f | x DETALHE(centro) = %.2f (luminancia media, 0..255)\n", dIgual, dOutra);
  ok(dIgual <= 3.0, "ancora: GFX_DETALHE com a ancora da Home reproduz o recorte do GFX_VITRINE");
  ok(dOutra >= 8.0, "ancora: o recorte centrado (o de antes) e DIFERENTE — o teste enxerga o desalinhamento");
}

static void bateria(int layout) {
  printf("\n##### layout %d\n", layout);
  home_ir_topo(); frames(150);
  ok(ajustes_home_layout() == layout, "o layout pedido esta ativo");
  { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = SDLK_DOWN; home_evento(&e); } frames(90);
  { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = SDLK_RIGHT; home_evento(&e); } frames(60);
  ok(!focoHero && foco.fileira >= 0, "o foco esta num poster de fileira");
  const int fil0 = foco.fileira, col0 = foco.coluna;
  passo(); memcpy(homeP, captura(), sizeof homeP);
  const float lumHome = (float)lumReg(homeP, 0, 1920, 0, 1080);
  float yHome = itemFoco.rect.y; (void)yHome;
  { GfxRect a; int tem = home_ancora_foco(&a); ok(tem, "ha ancora do cartaz focado"); yHome = a.y; }
  static QD abre[90], fecha[90];
  ev(SDLK_RETURN); tagDump = "x";
  rodar(90, abre, 0);
  for (int i = 0; i < 120; i++) passo();
  ok(detail_aberto() && detail_progresso() > 0.999f, "o detalhe abriu e assentou");
  memcpy(detP, captura(), sizeof detP); tiraBanda(detP, bandaDet);
  const float lumDet = (float)lumReg(detP, 0, 1920, 0, 1080);
  ev(SDLK_AC_BACK); tagDump = "y";
  rodar(90, fecha, 1);
  for (int i = 0; i < 90; i++) passo();
  ok(!detail_aberto(), "o Voltar fechou o detalhe");
  tiraBanda(captura(), bandaHome);

  // 1 + 2 + 5, para os dois sentidos
  for (int sentido = 0; sentido < 2; sentido++) {
    QD *q = sentido ? fecha : abre; const char *nome = sentido ? "voltar" : "abrir";
    float a = sentido ? lumDet : lumHome, b = sentido ? lumHome : lumDet, piso = fminf(a, b), min = 1e9f, fmax = 0, ymin = 1e9f, ymax = -1e9f;
    for (int i = 0; i < 90; i++) {
      if (q[i].lum < min) min = q[i].lum;
      if (i >= 1 && q[i].fill > fmax) fmax = q[i].fill;   // o 1o quadro paga o preparo das texturas
      if (q[i].temCartao) { if (q[i].y < ymin) ymin = q[i].y; if (q[i].y > ymax) ymax = q[i].y; } }
    printf("%s: luminancia minima %.1f (%.0f%% do menor extremo %.1f), preenchimento max %.2f telas, cartaz focado y %.0f..%.0f\n",
           nome, min, 100.0f * min / piso, piso, fmax, ymin, ymax);
    { char m[160]; snprintf(m, sizeof m, "%s: o cartaz focado nao anda na vertical (amplitude <= 2 px)", nome);
      ok(ymax < ymin + 2.0f || ymax < -1e8f, m); }
    { char m[160]; snprintf(m, sizeof m, "%s: sem quadro morto (luminancia >= 88%% do menor extremo)", nome); ok(min >= 0.88f * piso, m); }
    { char m[160]; snprintf(m, sizeof m, "%s: preenchimento por quadro <= 4,8 telas (fora o 1o quadro)", nome); ok(fmax <= 4.8f, m); }
  }
  // 4b. No detalhe de verdade, o backdrop usa a ancora da Home e a leva ao centro junto com o progresso.
  { double erro = 0; int n = 0; float ancHome = 0.5f;
    for (int sentido = 0; sentido < 2; sentido++) { QD *q = sentido ? fecha : abre;
      for (int i = 0; i < 90; i++) if (q[i].pd > 0.02f && q[i].pd < 0.98f) { double e = fabs(q[i].anc - (q[i].ancHome - 0.5f) * (1.0f - q[i].pd)); if (e > erro) erro = e; n++; ancHome = q[i].ancHome; } }
    printf("ancora do detalhe: ancora da Home %.2f, erro maximo da conta em %d quadros = %.4f\n", ancHome, n, erro);
    ok(n > 0 && erro < 0.005, "ancora: o backdrop do detalhe usa a ancora da Home e converge ao centro com o progresso");
    if (layout == 1) ok(fabsf(ancHome - 0.5f) > 0.1f, "ancora: no Padrao a ancora da Home NAO e o centro (o caso que saltava)"); }
  // 3. A Home aparece desde o comeco da volta: com o progresso em ~50%, as fileiras ja andaram >= 25% do caminho do
  //    detalhe para a Home (antes o backdrop as escondia ate ~33%, e elas surgiam de uma vez).
  { double dTotal = distB(bandaHome, bandaDet); int achou = 0; double melhor = 0;
    for (int i = 0; i < 90; i++) if (fecha[i].pd >= 0.40f && fecha[i].pd <= 0.60f) { achou = 1; double r = 1.0 - distB(banda[1][i], bandaHome) / dTotal; if (r > melhor) melhor = r; }
    printf("voltar: com o progresso ~50%%, as fileiras ja cobrem %.0f%% do caminho ate a Home (distancia total %.1f)\n", 100.0 * melhor, dTotal);
    ok(achou, "voltar: ha quadro com progresso entre 40% e 60%");
    ok(dTotal > 3.0 && melhor >= 0.25, "voltar: a Home ja aparece (>= 25% do caminho) na metade da saida");
    // simetria: ao ABRIR, no mesmo progresso, a Home ja se foi por >= 25% do caminho (a saida espelha a volta)
    double mAbre = 0; for (int i = 0; i < 90; i++) if (abre[i].pd >= 0.40f && abre[i].pd <= 0.80f) { double r = distB(banda[0][i], bandaDet) / dTotal; if (r > mAbre) mAbre = r; }
    printf("abrir: com o progresso entre 40%% e 80%%, as fileiras ja saram %.0f%% do caminho\n", 100.0 * mAbre);
    ok(mAbre >= 0.25, "abrir: a Home sai gradualmente (>= 25% do caminho entre 40% e 80% do progresso)"); }
  // 5. foco e posicao preservados
  ok(!focoHero && foco.fileira == fil0 && foco.coluna == col0, "voltar: o foco da Home e o mesmo de antes de abrir");
  { GfxRect a; int tem = home_ancora_foco(&a); ok(tem && fabsf(a.y - yHome) <= 1.0f, "voltar: o cartaz focado esta na mesma posicao vertical (+-1 px)"); }
  ok(lumReg(captura(), 0, 1920, 0, 1080) > 0.0, "voltar: a tela nao ficou vazia");
  (void)lumDet; (void)lumHome;
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
    snprintf(it[i].titulo, sizeof it[i].titulo, "Offline movie %d", i + 1); snprintf(it[i].sinopse, sizeof it[i].sinopse, "Local fixture synopsis for the featured area. A longer line so the detail page has something to lay out under the title and the buttons.");
    snprintf(it[i].poster, sizeof it[i].poster, "deploy/app/art/poster/%02d.jpg", i % 8); snprintf(it[i].backdrop, sizeof it[i].backdrop, "deploy/app/art/%02d.jpg", i % 8); }
  cat_definir_tudo(it, ROWS * ITEMS, rows, ROWS);
  char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "homeLayoutLocal %s\nvidroLocal 1\ntrailerHero 0\nsocialLocal 0\nidioma 0\n", getenv("NV_LAYOUT") ? getenv("NV_LAYOUT") : "1"); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  testeAncora();
  bateria(ajustes_home_layout());
  printf("\n%s\n", falhas ? "detalhe_transicao: FALHOU" : "detalhe_transicao: ok");
  return falhas ? 1 : 0;
}
