// DE ONDE OS PAINEIS DO CH NASCEM, com a Home real, o menu do topo e a ilha
// (World 24/11). Confere comportamento, nao so existencia:
//   - topo fixo + relogio: Busca nasce da LUPA (retangulo pequeno no alto),
//     Salvos da BARRA (largo), Central da PILULA (relogio);
//   - relogio desligado: SEGURAR CH+ nao faz a pilula aparecer (ilha_rect 0
//     durante o segurar), o progresso existe, e a Central nasce da barra;
//   - sem menu do topo e sem relogio: tudo nasce do centro do alto;
//   - "De onde os paineis nascem" = Do centro vence o topo; "Busca a partir
//     do menu do topo" desligado tira so a Busca da lupa;
//   - a Busca aberta da lupa guarda a origem e a peca cresce dela.
//   bash tests/ch_origem_shot.sh /tmp/nv-ch
#include "ajustes.h"
#include "catalogo.h"
#include "central.h"
#include "chorigem.h"
#include "dados.h"
#include "gfx.h"
#include "home.h"
#include "ilha.h"
#include "layout.h"
#include "menu.h"
#include "spotlight.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *janela;
static const char *dirDados;
static Uint32 relogio = 1000;
static int falhas, relogioLigado = 1;
static unsigned char *pix;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "ok" : "FALHA", m); if (!c) falhas++; }
static void gravar(const char *bmp) {
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (int y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  SDL_SaveBMP(s, bmp);
  SDL_FreeSurface(s);
  printf("captura: %s\n", bmp);
}
static void quadros(int n, const char *bmp) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, relogio);
    central_tecla_quadro(relogio, NULL);
    central_atualizar(1.0f / 60.0f, relogio);
    spot_atualizar(1.0f / 60.0f, relogio);
    txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    home_desenhar(relogio);
    menu_desenhar(relogio);
    ilha_relogio_visivel(relogioLigado);
    ilha_desenhar(relogio);
    if (spot_aberto() || spot_progresso() > 0.0f) spot_desenhar(relogio, 0);
    if (bmp && i == n - 1) gravar(bmp);
    SDL_GL_SwapWindow(janela);
  }
}
static void ajustes(const char *extra) {
  char cam[800];
  FILE *a;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w"); assert(a);
  fprintf(a, "idioma 0\nhomeLayoutLocal 1\nmodernSidebar 1\nselected_theme 9\n%s", extra);
  fclose(a);
  ajustes_dir(dirDados);
}
static void chTecla(Uint32 tipo) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = tipo; e.key.timestamp = relogio; e.key.keysym.sym = SDLK_PAGEUP;
  e.key.state = tipo == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
  central_tecla(&e, 1);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nv-ch/c";
  static CatItem itens[20];
  static CatFileira fils[2];
  char cam[800], bmp[900];
  GfxRect r, r2;
  float x, y, w, h, ix, iy, iw, ih;
  pix = malloc(1920 * 1080 * 4);
  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  janela = SDL_CreateWindow("Nuvio: origem CH", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!janela || !SDL_GL_CreateContext(janela)) return 1;
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  if (!gfx_iniciar() || !txt_iniciar("deploy/app", 1)) return 1;
  tex_iniciar(192);
  snprintf(cam, sizeof cam, "%s/cache", dirDados);
  tex_cache_dir(cam);
  gfx_icones_dir("deploy/app/art");
  ajustes("menuTopoLocal 1\nmenuTopoFixoLocal 1\nrelogioTelaLocal 1\n");
  if (!home_iniciar("deploy/app/art")) return 1;
  memset(itens, 0, sizeof itens); memset(fils, 0, sizeof fils);
  for (int i = 0; i < 12; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt91%05d", i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    snprintf(c->titulo, sizeof c->titulo, "Titulo %d", i);
    snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", 20 + i);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", 20 + i);
  }
  snprintf(fils[0].chave, sizeof fils[0].chave, "pop_movie");
  snprintf(fils[0].titulo, sizeof fils[0].titulo, "Popular - Filme");
  snprintf(fils[0].tipo, sizeof fils[0].tipo, "movie");
  fils[0].ini = 0; fils[0].n = 12;
  cat_definir_tudo(itens, 12, fils, 1);
  chorigem_teste_na_home(1);
  quadros(60, NULL);
  snprintf(bmp, sizeof bmp, "%s-0-home.bmp", saida); quadros(1, bmp);
  // Um toque curto antes: o CH+ so confia no "segurado" depois de ver um KEYUP
  // de verdade (chsegura.h); na TV o primeiro toque do dia faz isso.
  chTecla(SDL_KEYDOWN); quadros(4, NULL); chTecla(SDL_KEYUP); quadros(30, NULL);

  // ------------------------------------------- topo fixo + relogio na tela
  ok(chorigem(CHO_P_BUSCA, &r) == CHO_TOPO && r.w < 260.0f && r.y < 140.0f, "Busca: da lupa do menu do topo");
  ok(chorigem(CHO_P_SALVOS, &r2) == CHO_TOPO && r2.w > 300.0f && r2.w > r.w * 3.0f, "Salvos: da barra do topo (a trilha inteira)");
  ok(r.x >= r2.x - 1.0f && r.x + r.w <= r2.x + r2.w + 1.0f, "a lupa fica dentro da barra");
  ok(chorigem(CHO_P_CENTRAL, NULL) == CHO_RELOGIO, "Central: da pilula do relogio (ela mora na barra)");
  ok(ilha_rect(&ix, &iy, &iw, &ih), "com o relogio ligado a pilula esta na tela");
  // A Busca nasce da lupa: a origem guardada e a da lupa, na camada da busca.
  spot_abrir_de(0, r.x, r.y, r.w, r.h);
  ok(spot_teste_origem(&x, &y, &w, &h) && w < r.w && w > 10.0f, "Busca aberta guarda a lupa como origem (na escala dela)");
  quadros(6, NULL);
  snprintf(bmp, sizeof bmp, "%s-1-busca-da-lupa.bmp", saida); quadros(1, bmp);
  quadros(50, NULL);
  snprintf(bmp, sizeof bmp, "%s-2-busca-aberta.bmp", saida); quadros(1, bmp);
  spot_fechar(); quadros(40, NULL);

  // ------------------------------------------- relogio desligado
  relogioLigado = 0;
  ajustes("menuTopoLocal 1\nmenuTopoFixoLocal 1\nrelogioTelaLocal 0\n");
  quadros(60, NULL);
  ok(!ilha_rect(&ix, &iy, &iw, &ih), "sem relogio a pilula nao esta na tela");
  ok(chorigem(CHO_P_CENTRAL, &r) == CHO_TOPO && r.w > 300.0f, "Central sem relogio: da barra do topo");
  chTecla(SDL_KEYDOWN);
  quadros(26, NULL);   // ~420 ms: passou da zona morta, antes do limiar
  ok(central_segurar_progresso(relogio) > 0.05f, "segurando: ha progresso");
  ok(!ilha_rect(&ix, &iy, &iw, &ih), "segurando sem relogio: a pilula do relogio NAO aparece");
  snprintf(bmp, sizeof bmp, "%s-3-segurando-na-barra.bmp", saida); quadros(1, bmp);
  quadros(20, NULL);
  ok(central_aberta(), "segurado: a Central abriu");
  ok(central_teste_origem(&x, &y, &w, &h) && w > 300.0f && y < 120.0f, "a Central nasceu da barra");
  ok(ilha_corpo_teste_origem(), "o corpo da ilha cresce da barra, nao da pilula");
  snprintf(bmp, sizeof bmp, "%s-4-central-da-barra.bmp", saida); quadros(1, bmp);
  chTecla(SDL_KEYUP);
  quadros(40, NULL);
  snprintf(bmp, sizeof bmp, "%s-5-central-aberta.bmp", saida); quadros(1, bmp);
  central_fechar(); quadros(60, NULL);

  // ------------------------------------------- sem topo e sem relogio
  ajustes("menuTopoLocal 0\nrelogioTelaLocal 0\n");
  quadros(60, NULL);
  ok(chorigem(CHO_P_SALVOS, &r) == CHO_CENTRO && chorigem(CHO_P_BUSCA, NULL) == CHO_CENTRO &&
     chorigem(CHO_P_CENTRAL, NULL) == CHO_CENTRO, "sem topo e sem relogio: do centro");
  ok(r.y < 100.0f && r.x + r.w * 0.5f > 900.0f && r.x + r.w * 0.5f < 1020.0f, "o centro e o meio do alto");
  // soltar antes do limiar recolhe e nao abre
  chTecla(SDL_KEYDOWN);
  quadros(24, NULL);
  snprintf(bmp, sizeof bmp, "%s-6-segurando-no-centro.bmp", saida); quadros(1, bmp);
  ok(!ilha_rect(&ix, &iy, &iw, &ih), "segurando no centro: sem pilula");
  chTecla(SDL_KEYUP);
  quadros(30, NULL);
  ok(!central_aberta() && central_segurar_progresso(relogio) <= 0.0f, "soltou antes: recolhe e nao abre");

  // ------------------------------------------- os dois ajustes
  ajustes("menuTopoLocal 1\nmenuTopoFixoLocal 1\nrelogioTelaLocal 1\nchOrigemLocal 2\n");
  relogioLigado = 1;
  quadros(60, NULL);
  ok(chorigem(CHO_P_SALVOS, NULL) == CHO_CENTRO && chorigem(CHO_P_BUSCA, NULL) == CHO_CENTRO,
     "ajuste Do centro vence o topo");
  ajustes("menuTopoLocal 1\nmenuTopoFixoLocal 1\nrelogioTelaLocal 1\nchOrigemLocal 0\nbuscaDaBarraLocal 1\n");
  quadros(20, NULL);
  ok(chorigem(CHO_P_BUSCA, NULL) == CHO_RELOGIO && chorigem(CHO_P_SALVOS, NULL) == CHO_TOPO,
     "Busca sem a lupa: do relogio; os Salvos continuam no topo");
  printf("%s\n", falhas ? "ch_origem: FALHOU" : "ch_origem: ok");
  return falhas != 0;
}
