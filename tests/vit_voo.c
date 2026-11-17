// 2.0.44 (B): VOO DO CARTAO — abrir uma Collection a partir da Home: o cartao cresce da posicao real ate a tela,
// a Home fica por baixo ate ele cobrir, e a revelacao do conteudo esta integrada ao voo (630 ms no total).
// home.c + vertudo.c reais, GLES2 por software (logica, NAO o desempenho da Mali-G31).
// Uso: vit_voo [layout 0|1|2] [prefixo-de-capturas]
#include "../src/home.c"
#include "home_colfix.h"
static int homeDesenhos;
// Um quadro do app (app.c): a Home so e desenhada se a Collection ainda nao cobre a tela.
static void quadro(void) {
  now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  if (!vertudo_aberta()) home_atualizar(1.f / 60, now);
  vertudo_atualizar(1.f / 60, now);
  if (!vertudo_cobre_tela()) { home_desenhar(now); homeDesenhos++; }
  vertudo_desenhar(now); gfx_ambiente_descarregar(); }
static void quadros(int n) { for (int i = 0; i < n; i++) quadro(); }
static void ajuste(int v) { char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "a"); assert(f);
  fprintf(f, "colEntradaLocal %d\n", v); fclose(f); ajustes_dir(getenv("NUVIO_DADOS")); }
static void esc(void) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = SDLK_ESCAPE; vertudo_evento(&e); e.type = SDL_KEYUP; vertudo_evento(&e); }
static int ate_pasta(int rc) { for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); } frames(90); return foco.fileira == rc; }
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0; const char *pre = argc > 2 ? argv[2] : NULL;
  int rc = colfix_iniciar(layout, 12, 1);
  ajuste(0);
  ok(rc >= 0 && ate_pasta(rc), "o foco esta na fileira de Collections");
  if (rc < 0) { puts("vit_voo: FALHOU"); return 1; }
  GfxRect orig; ok(home_ancora_foco(&orig) && orig.w > 8 && orig.h > 8, "a Home publica o retangulo real do cartao focado");
  // ---- 1. Voo ligado
  key(SDLK_RETURN);
  ok(vertudo_aberta(), "OK abre a Collection no clique (o carregamento comeca ja, em paralelo ao voo)");
  float p0 = -1; GfxRect r0;
  ok(vertudo_voo_ativo(&p0, &r0), "o voo do cartao comeca");
  ok(fabsf(r0.x - orig.x) < 1.0f && fabsf(r0.y - orig.y) < 1.0f && fabsf(r0.w - orig.w) < 1.0f && fabsf(r0.h - orig.h) < 1.0f,
     "continuidade: no clique o cartao do voo esta exatamente onde o cartao da Home estava");
  ok(vertudo_entrada_ms() < 1.0f, "o relogio do voo parte do clique");
  homeDesenhos = 0;
  float wAnt = r0.w, hAnt = r0.h; int mono = 1, homeAposCobrir = 0, coberto = -1, fim = -1, vooAcabou = -1, homeDuranteVoo = 0;
  GfxRect rm = {0}; float pm = -1, tAcabou = 0;
  for (int q = 0; q < 60; q++) {
    int homeAntes = homeDesenhos; quadro();
    float p; GfxRect r; int ativo = vertudo_voo_ativo(&p, &r);
    if (ativo) { if (r.w < wAnt - 0.01f || r.h < hAnt - 0.01f) mono = 0; wAnt = r.w; hAnt = r.h; if (!vertudo_cobre_tela() && homeDesenhos > homeAntes) homeDuranteVoo++; }
    if (vertudo_cobre_tela() && coberto < 0) coberto = q;
    if (coberto >= 0 && q > coberto && homeDesenhos > homeAntes) homeAposCobrir = 1;
    if (ativo && pm < 0 && vertudo_entrada_ms() >= 190.0f) { pm = p; rm = r; }
    if (!ativo && vooAcabou < 0 && q > 3) { vooAcabou = q; tAcabou = vertudo_entrada_ms(); }
    if (fim < 0 && vertudo_entrada_ms() >= HT_ENT_TOTAL_MS) fim = q;
    if (pre) { float t = vertudo_entrada_ms(); static int s0, s1, s2, s3, s4, s5;
      if (!s0 && t >= 0) { s0 = 1; shot(pre, "t000"); } if (!s1 && t >= 100) { s1 = 1; shot(pre, "t100"); } if (!s2 && t >= 200) { s2 = 1; shot(pre, "t200"); }
      if (!s3 && t >= 300) { s3 = 1; shot(pre, "t300"); } if (!s4 && t >= 420) { s4 = 1; shot(pre, "t420"); } if (!s5 && t >= 600) { s5 = 1; shot(pre, "t600"); } }
  }
  ok(mono, "o cartao so cresce (largura e altura) durante o voo");
  ok(homeDuranteVoo > 8, "a Home fica por baixo durante o voo (o cartao cresce a partir dela)");
  ok(coberto > 0 && !homeAposCobrir, "depois de cobrir a tela a Home nao e mais desenhada (poupa a GPU da TV)");
  ok(vooAcabou > 0 && tAcabou >= HT_VOO_FIM_MS - 1.0f && tAcabou <= HT_VOO_FIM_MS + 20.0f, "o cartao some do voo em ~560 ms (apaga dando lugar ao conteudo)");
  ok(fim > 0 && fim * 16 >= 600 - 16 && fim * 16 <= 650, "o tempo total desde o clique fica entre 600 e 650 ms");
  ok(pm > 0.3f && pm < 1.0f && rm.w > orig.w && rm.w < NV_TELA_W, "a revelacao comeca DENTRO do voo: o cabecalho entra com o cartao ainda crescendo");
  ok(vertudo_entrada_ms() >= HT_ENT_TOTAL_MS && !vertudo_voo_ativo(NULL, NULL) && vertudo_cobre_tela(), "no fim nada fica animando");
  // ---- 2. Voltar durante o voo: caminho de sempre, sem travar
  vertudo_fechar_seco(); quadros(20); frames(30);
  key(SDLK_RETURN); quadros(6);
  ok(vertudo_voo_ativo(NULL, NULL), "reabrir reinicia o voo");
  esc(); quadros(3);
  ok(!vertudo_voo_ativo(NULL, NULL) && !vertudo_cobre_tela(), "Voltar no meio do voo: o voo termina e a Home volta");
  quadros(60); ok(!vertudo_aberta(), "a Collection fecha pelo caminho de sempre");
  // ---- 3. Sem cartao de origem (Spotlight/menu/detalhe): so a revelacao, sem voo
  vertudo_fechar_seco(); quadros(5);
  { const ColFolder *fo = col_folder(fileiras[rc].folders[0]); vertudo_colecao(fo); }
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL), "origem sem cartao: abre sem voo");
  ok(fabsf(vertudo_entrada_ms() - HT_VOO_SEM_CARTAO_MS) < 1.0f, "origem sem cartao: so a revelacao (cabecalho entra de imediato)");
  // ---- 4. Ajuste desligado, efeitos minimos
  vertudo_fechar_seco(); quadros(5); ajuste(1);
  key(SDLK_RETURN);
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL) && vertudo_entrada_ms() >= HT_ENT_TOTAL_MS, "ajuste DESLIGADO: abre direto (instantaneo)");
  vertudo_fechar_seco(); quadros(5); ajuste(0); gfx_definir_efeitos_minimos(1);
  key(SDLK_RETURN);
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL) && vertudo_entrada_ms() >= HT_ENT_TOTAL_MS, "efeitos minimos: abre direto (instantaneo)");
  gfx_definir_efeitos_minimos(0);
  puts(falhas ? "vit_voo: FALHOU" : "vit_voo: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
