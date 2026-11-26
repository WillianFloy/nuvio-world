// O LADO "APP" DO COMPARADOR (tools/r8/comparar.py): abre a peca na Home real
// com arte de verdade e grava os quadros nos MESMOS instantes da prancha
// (tempo da prancha; a abertura comeca onde a prancha comeca a abrir).
//   bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4 3.1
// Pecas: BU = Busca (CH-), SV = Salvos (CH+) — as da base de 23/11 — e, desde a
// Fase 3, MC = menu de segurar OK na Folha e MA = no Desdobrar (o terceiro
// cartaz da fileira, como na prancha; o foco desce, desce e sobe; fecha em 5,3).
// Com o menu R8, a Home de tras congela como no app (ctxFundoParado em app.c).
// NAO ENTRA NA SUITE (*_shot).
#include "addons.h"
#include "ajustes.h"
#include "catalogo.h"
#include "ctxmenu.h"
#include "fundoparado.h"
#include "dados.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "menu.h"
#include "salvos.h"
#include "salvospainel.h"
#include "spotlight.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *janela;
static Uint32 relogio = 1000;
static const char *NOMES[] = { "Duna: Parte Dois", "Moana 2", "Severance", "Fallout", "Andor", "O Urso",
                               "Slow Horses", "Shōgun", "The Last of Us", "Ruptura", "Arcane", "Pinguim",
                               "Bebê Rena", "Fargo", "True Detective", "Silo", "Reacher", "Invencível",
                               "Gen V", "Blue Eye Samurai" };

static void gravarPng(const char *png) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  IMG_SavePNG(s, png);
  SDL_FreeSurface(s);
  free(pix);
  printf("%s\n", png);
}
static int passoMs = 16;
static void quadro(const char *png) {
  relogio += passoMs;
  SDL_PumpEvents();
  tex_bombear(8);
  home_atualizar(0.016f, relogio);
  spainel_atualizar(0.016f, relogio);
  spot_atualizar(0.016f, relogio);
  ctx_atualizar(0.016f, relogio);
  txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  { float esc, ox, oy, veu; unsigned ver;
    static FundoParado fp;
    static const Uint32 REFAZ[] = { 400, 1500, 4000 };
    if (ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver)) {
      // o mesmo caminho de ctxFundoParado (app.c)
      if (fundoparado_passo(&fp, 1, cat_revisao() + ver * 7919u, relogio, REFAZ, 3) == FP_PINTAR) {
        gfx_snap_comecar();
        glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        home_desenhar(relogio);
        menu_desenhar(relogio);
        gfx_snap_terminar();
        glViewport(0, 0, 1920, 1080);
        fundoparado_pintado(&fp, 0, 0, cat_revisao() + ver * 7919u, relogio);
      }
      home_fileiras_vistas(relogio);   // como app.c: a copia nao recomeca a entrada das fileiras
      glClear(GL_COLOR_BUFFER_BIT);
      gfx_snap_desenhar_xf(esc, ox, oy, 0.0f, 0.0f, veu);
      ctx_r8_fundo_pronto(1);
    } else {
      fundoparado_passo(&fp, 0, 0, relogio, REFAZ, 3);
      ctx_r8_fundo_pronto(0);
      home_desenhar(relogio);
      menu_desenhar(relogio);
    } }
  ctx_desenhar(relogio);
  if (spainel_visivel()) spainel_desenhar(relogio);
  if (spot_progresso() > 0.0f) { spot_veu(); spot_desenhar(relogio, 1); }
  if (png) gravarPng(png);
  SDL_GL_SwapWindow(janela);
}

int main(int argc, char **argv) {
  static CatItem itens[40];
  static CatFileira fils[3];
  char cam[800], png[900];
  const char *peca, *saida, *dir;
  float abre, fecha;
  int nt, k, feito = 0;
  FILE *a;
  if (argc < 4) { fprintf(stderr, "uso: peca saida t...\n"); return 2; }
  peca = argv[1]; saida = argv[2]; nt = argc - 3;
  abre = 0.8f;    // as pranchas dos paineis abrem em 0,8 s...
  fecha = 5.4f;   // ...e comecam a fechar em 5,4 s
  dados_iniciar("deploy/app/art");
  dir = dados_dir();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#ifndef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
  janela = SDL_CreateWindow("Nuvio: r8 quadros", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!janela || !SDL_GL_CreateContext(janela)) return 1;
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  if (!gfx_iniciar() || !txt_iniciar("deploy/app", 1)) return 1;
  gfx_snap_iniciar(1920, 1080);
  tex_iniciar(192);
  snprintf(cam, sizeof cam, "%s/cache", dir);
  tex_cache_dir(cam);
  gfx_icones_dir("deploy/app/art");
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dir);
  if (!(a = fopen(cam, "w"))) return 1;
  // A TCL do dono: Home Moderna, menu do topo, relogio ligado, Montserrat.
  fprintf(a, "idioma 0\nhomeLayoutLocal 1\nmodernSidebar 1\nmenuTopoLocal %d\nselected_theme 9\n",
          getenv("NV_SEM_TOPO") ? 0 : 1);
  if (getenv("NV_POSTER")) fprintf(a, "modernLandscapePostersEnabled 1\n");
  // As pranchas sao desenhadas a 100 %: o comparador usa o mesmo tamanho.
  fprintf(a, "tamanhoUiLocal 0\n");
  if (getenv("NV_PRETO")) fprintf(a, "temaFundoLocal 1\n");
  if (getenv("NV_DESDOBRAR")) fprintf(a, "menuSegurarLocal 1\n");
  if (getenv("NV_REDUZIDO")) fprintf(a, "animacoes 1\nperformanceAutoVersaoLocal 1\nvidroOpacVersaoLocal 1\n");
  fclose(a);
  ajustes_dir(dir);
  if (!home_iniciar("deploy/app/art")) return 1;
  printf("escala: ui %.3f texto %.3f\n", gfx_escala_ui_base(), gfx_escala_txt());
  // A Home exige um addon ligado mesmo com o catalogo de fixture (carrossel_shot.c),
  // senao o cartao "Sua Home esta vazia" toma a tela.
  { AddonRemoto addon = { .nome = "Fixture", .url = "http://127.0.0.1:1/manifest.json", .ativo = 1 };
    if (!addons_definir_lista(&addon, 1)) return 1; }
  memset(itens, 0, sizeof itens); memset(fils, 0, sizeof fils);
  for (int i = 0; i < 20; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt90%05d", i);
    snprintf(c->tipo, sizeof c->tipo, i % 3 ? "movie" : "series");
    snprintf(c->titulo, sizeof c->titulo, "%s", NOMES[i]);
    snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", i);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", i);
    if (i < 6) { c->progresso = 30 + i * 8; c->restanteMin = 40; }
  }
  snprintf(fils[0].chave, sizeof fils[0].chave, "pop_movie");
  snprintf(fils[0].titulo, sizeof fils[0].titulo, "Populares");
  snprintf(fils[0].tipo, sizeof fils[0].tipo, "movie");
  fils[0].ini = 6; fils[0].n = 14;
  snprintf(fils[1].chave, sizeof fils[1].chave, "continue_watching");
  snprintf(fils[1].titulo, sizeof fils[1].titulo, "Continuar assistindo");
  snprintf(fils[1].tipo, sizeof fils[1].tipo, "movie");
  fils[1].ini = 0; fils[1].n = 6;
  cat_definir_tudo(itens, 20, fils, 2);
  if (!strcmp(peca, "SV")) for (int i = 0; i < 12; i++) salvos_definir(&itens[i], 1);
  // A Home assenta (artes carregadas) antes de abrir.
  for (k = 0; k < 120; k++) quadro(NULL);
  // MENU: o foco desce para a primeira fileira e vai ao terceiro cartaz.
  if (!strcmp(peca, "MC") || !strcmp(peca, "MA") || !strcmp(peca, "COMP")) {
    SDL_Event e;
    // NV_ORIGEM: poster (padrao), borda (o ultimo cartaz visivel), hero (o
    // destaque, sem descer), colecao (so a pagina de estilos da fileira).
    const char *orig = getenv("NV_ORIGEM") ? getenv("NV_ORIGEM") : "poster";
    SDL_Keycode teclas[12];
    int nt2 = 0;
    if (strcmp(orig, "hero")) teclas[nt2++] = SDLK_DOWN;
    if (!strcmp(orig, "borda")) for (int q = 0; q < 7; q++) teclas[nt2++] = SDLK_RIGHT;
    else if (strcmp(orig, "hero")) { teclas[nt2++] = SDLK_RIGHT; teclas[nt2++] = SDLK_RIGHT; }
    abre = 1.3f; fecha = 5.28f;   // a prancha: segura de 0,6 a 1,3; o conteudo sai em 5,28
    for (k = 0; k < nt2; k++) {
      memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = teclas[k];
      home_evento(&e);
      e.type = SDL_KEYUP; home_evento(&e);
      for (int q = 0; q < 30; q++) quadro(NULL);
    }
    for (k = 0; k < 60; k++) quadro(NULL);
  }
  // MODO COMPORTAMENTO (peca COMP): o que o menu R8 faz, nao como se desenha.
  if (!strcmp(peca, "COMP")) {
    int falhas = 0, i;
    HomeItem hi;
    CtxR8Geo g; float esc, ox, oy, veu, antA = 0; unsigned ver;
    SDL_Event e;
#define OKC(c, ...) do { if (!(c)) { printf("FALHOU: "); printf(__VA_ARGS__); printf("\n"); falhas++; } \
                         else { printf("ok  "); printf(__VA_ARGS__); printf("\n"); } } while (0)
    OKC(ajustes_menu_segurar() == (getenv("NV_DESDOBRAR") ? 1 : 0), "ajuste: %s (%d)",
        getenv("NV_DESDOBRAR") ? "Desdobrar gravado" : "Folha por padrao", ajustes_menu_segurar());
    if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz\n"); return 1; }
    ctx_fileira("pop_movie", "Populares"); ctx_dispensar_retomar(0); ctx_abrir(hi.indice);
    for (i = 0; i < 40; i++) quadro(NULL);
    ctx_r8_geo(&g);
    OKC(ctx_aberto() && g.visivel && g.forma == ajustes_menu_segurar(), "abre o menu das pranchas na forma do ajuste (forma %d)", g.forma);
    OKC(ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver) && esc < 1.0f && veu > 0.3f, "a Home atras congela, recua (%.3f) e escurece (%.2f)", esc, veu);
    // Voltar fecha: a superficie vai a zero sem saltar e a Home volta viva.
    memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
    // A Folha apaga (alfa); o Desdobrar recolhe (largura, ate 3,8 px/ms no pico).
#define SALTO(g, ant) (g.forma ? fabsf(g.superficie.w - (ant)) > 80.0f : fabsf(g.superficieA - (ant)) > 0.35f)
#define VAL(g) (g.forma ? g.superficie.w : g.superficieA)
    { int salto = 0; antA = VAL(g);
      for (i = 0; i < 30; i++) { quadro(NULL); if (ctx_r8_geo(&g)) { if (SALTO(g, antA)) salto = 1; antA = VAL(g); } }
      OKC(!salto, "fechar nao salta"); }
    OKC(!ctx_aberto() && !ctx_r8_geo(&g) && !ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver),
        "fechado: nada do menu na tela e a Home volta a ser desenhada viva");
    // Reabrir no meio do fechar continua do ponto em que estava.
    ctx_abrir(hi.indice);
    for (i = 0; i < 40; i++) quadro(NULL);
    e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
    for (i = 0; i < 3; i++) quadro(NULL);
    ctx_r8_geo(&g); antA = VAL(g);
    ctx_abrir(hi.indice);
    { int salto = 0;
      for (i = 0; i < 40; i++) { quadro(NULL); ctx_r8_geo(&g); if (SALTO(g, antA)) salto = 1; antA = VAL(g); }
      OKC(!salto && g.visivel && ctx_aberto(), "reabrir durante o fechar nao pisca"); }
    // Setas: a pilula desliza para a linha de baixo (mola) e para nela.
    { float y0, y1; ctx_r8_geo(&g); y0 = g.pilula.y;
      e.key.keysym.sym = SDLK_DOWN; ctx_evento(&e);
      for (i = 0; i < 30; i++) quadro(NULL);
      ctx_r8_geo(&g); y1 = g.pilula.y;
      OKC(y1 - y0 > 50.0f && y1 - y0 < 70.0f, "seta para baixo leva o foco a proxima linha (%.1f px)", y1 - y0); }
    e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
    for (i = 0; i < 30; i++) quadro(NULL);
    // O painel de Salvos continua com o menu de antes (Fase 4).
    { const CatItem *ci = cat_item(hi.indice);
      if (ci) { ctx_abrir_salvo(ci); for (i = 0; i < 20; i++) quadro(NULL);
        OKC(ctx_aberto() && !ctx_r8_geo(&g) && !ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver),
            "o menu do painel de Salvos nao e o das pranchas (fica para a Fase 4)");
        e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e); for (i = 0; i < 30; i++) quadro(NULL); } }
    // Reduzir animacoes: nada desliza nem voa; tudo aparece junto em 0,12 s.
    if (getenv("NV_REDUZIDO")) {
      float x0 = -1, ty0 = -1; int anda = 0, n = 0;
      ctx_abrir(hi.indice);
      for (i = 0; i < 12; i++) {
        quadro(NULL);
        if (!ctx_r8_geo(&g) || g.superficieA < 0.01f) continue;
        if (x0 < 0) { x0 = g.superficie.x; ty0 = g.tituloY; }
        if (fabsf(g.superficie.x - x0) > 0.5f || fabsf(g.tituloY - ty0) > 0.5f) anda = 1;
        n++;
      }
      OKC(n > 0 && !anda, "reduzidas: superficie e titulo parados desde o 1o quadro visivel (%d quadros)", n);
      OKC(g.superficieA > 0.99f && g.tituloA > 0.99f, "reduzidas: em 0,2 s tudo aceso (sup %.2f tit %.2f)",
          g.superficieA, g.tituloA);
      e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      // 20 ms do conteudo + 120 ms da opacidade, contados do quadro que viu o Voltar.
      for (i = 0; i < 11; i++) quadro(NULL);
      OKC(!ctx_r8_geo(&g), "reduzidas: fecha em ate 0,2 s");
    }
    printf("r8_menu_comportamento: %d falhas\n", falhas);
    return falhas ? 1 : 0;
  }
  // MODO ESPECIFICACAO (tests/r8_menu_espec.sh): os quadros nos MESMOS
  // instantes da prancha (60/s), os eventos nos instantes dela, e o que o menu
  // desenhou (ctx_r8_geo) contra as trilhas exportadas.
  if (getenv("NV_ESPEC")) {
    FILE *ef = fopen(getenv("NV_ESPEC"), "r");
    char lin[400];
    int falhas = 0, conf = 0, ev = 0, ma = !strcmp(peca, "MA");
    Uint32 base = relogio;
    HomeItem hi;
    const double EV[] = { 1.3, 2.45, 3.1, 3.7, 5.28 };
    if (!ef) { printf("sem especificacao\n"); return 2; }
    if (!home_item_focado(&hi)) { printf("sem cartaz focado\n"); return 2; }
    while (fgets(lin, sizeof lin, ef)) {
      double t; float v[8] = { 0 };
      CtxR8Geo g; float esc, ox, oy, veu = 0; unsigned ver;
      if (sscanf(lin, "%lf %f %f %f %f %f %f %f %f", &t, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]) < 8) continue;
      relogio = base + (Uint32)(t * 1000.0 + 0.5);
      while (ev < 5 && t + 1e-6 >= EV[ev]) {
        SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN;
        ctx_atualizar(0.0f, base + (Uint32)(EV[ev] * 1000.0 + 0.5));
        if (ev == 0) { ctx_fileira("pop_movie", "Populares"); ctx_dispensar_retomar(0); ctx_abrir(hi.indice); }
        else { e.key.keysym.sym = ev == 1 || ev == 2 ? SDLK_DOWN : ev == 3 ? SDLK_UP : SDLK_ESCAPE; ctx_evento(&e); }
        ctx_atualizar(0.0f, base + (Uint32)(EV[ev] * 1000.0 + 0.5));
        ev++;
      }
      relogio -= 16; passoMs = 16;   // quadro() soma o passo: chega em `relogio` exato
      { Uint32 alvo = base + (Uint32)(t * 1000.0 + 0.5); relogio = alvo - (Uint32)passoMs; }
      quadro(NULL);
      if (!ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver)) veu = 0;
      ctx_r8_geo(&g);
#define CONF(nome, app, pr, tol) do { conf++; if (fabsf((app) - (pr)) > (tol)) { if (falhas < 25) \
        printf("FALHOU t=%.3f %s: prancha %.3f app %.3f\n", t, nome, (double)(pr), (double)(app)); falhas++; } } while (0)
      if (!ma) {
        if (g.visivel) {
          CONF("folha.x", g.superficie.x, v[0], 1.0f);
          CONF("folha.o", g.superficieA, v[1], 0.01f);
          if (v[3] > 0.0f || g.pilulaA > 0.0f) { CONF("pilula.y", g.pilula.y, v[2], 1.0f); CONF("pilula.o", g.pilulaA, v[3], 0.01f); }
          CONF("titulo.y", g.tituloY, v[4], 1.0f); CONF("titulo.o", g.tituloA, v[5], 0.01f);
        } else CONF("folha.o", 0.0f, v[1], 0.01f);
        CONF("veu", veu, v[6], 0.01f);
      } else {
        if (g.visivel) {
          // O relogio do app e em ms inteiros: o quadro de 1/60 s cai ate 0,33 ms
          // fora do instante da prancha, e no pico o Desdobrar anda 3,8 px/ms.
          CONF("superficie.w-cartaz", g.superficie.w - g.cartaz.w, v[0], 1.5f);
          if (v[2] > 0.0f || g.pilulaA > 0.0f) { CONF("pilula.y-cartaz", g.pilula.y - g.cartaz.y, v[1], 1.0f); CONF("pilula.o", g.pilulaA, v[2], 0.01f); }
          CONF("titulo.y-cartaz", g.tituloY - g.cartaz.y, v[3], 1.0f); CONF("titulo.o", g.tituloA, v[4], 0.01f);
          CONF("vizinhas.dx", g.deslVizinhas, v[6], 1.5f);
        } else CONF("superficie.w-cartaz", 0.0f, v[0], 1.0f);
        CONF("veu", veu, v[5], 0.01f);
      }
    }
    fclose(ef);
    printf("r8_menu_espec %s: %d valores conferidos, %d falhas\n", peca, conf, falhas);
    return falhas ? 1 : 0;
  }
  // Antes da abertura: os instantes da prancha anteriores a `abre`.
  {
    float t = 0.0f;
    int alvo = 0, fechou = 0;
    while (feito < nt) {
      float quer = (float)atof(argv[3 + feito]);
      if (!alvo && t + 0.0005f >= abre) {
        alvo = 1;
        if (!strcmp(peca, "BU")) spot_abrir(0);
        else if (!strcmp(peca, "SV")) spainel_abrir();
        else if (!strcmp(peca, "MC") || !strcmp(peca, "MA")) {
          HomeItem hi;
          const char *orig = getenv("NV_ORIGEM") ? getenv("NV_ORIGEM") : "poster";
          if (!strcmp(orig, "colecao")) ctx_abrir_fileira("pop_movie", "Populares");
          else if (home_item_focado(&hi)) {
            ctx_fileira(strcmp(orig, "hero") ? "pop_movie" : NULL, "Populares");
            ctx_dispensar_retomar(0); ctx_abrir(hi.indice);
          }
          printf("menu aberto: %d (indice %d)\n", ctx_aberto(), hi.indice);
        }
      }
      if (alvo && !fechou && t + 0.0005f >= fecha) {
        fechou = 1;
        if (!strcmp(peca, "BU")) spot_fechar();
        else if (!strcmp(peca, "SV")) spainel_fechar();
        else if (ctx_aberto()) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e); }
      }
      if (alvo && ctx_aberto() && !strcmp(peca, "MC") + !strcmp(peca, "MA")) {
        static int mov;
        const float quando[] = { 2.45f, 3.1f, 3.7f };
        const SDL_Keycode tec[] = { SDLK_DOWN, SDLK_DOWN, SDLK_UP };
        if (mov < 3 && t + 0.0005f >= quando[mov]) {
          SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = tec[mov];
          // NV_OK: no terceiro passo, OK na linha em foco (estilo, confirmacao).
          if (mov == 2 && getenv("NV_OK")) {
            e.type = SDL_KEYUP; e.key.keysym.sym = SDLK_RETURN; ctx_evento(&e);
            e.type = SDL_KEYDOWN; ctx_evento(&e);
          } else ctx_evento(&e);
          mov++;
        }
      }
      if (t + 0.0005f >= quer) {
        snprintf(png, sizeof png, "%s/app-%s-%.3f.png", saida, peca, quer);
        quadro(png);
        feito++;
      } else quadro(NULL);
      t += 0.016f;
    }
  }
  return 0;
}
