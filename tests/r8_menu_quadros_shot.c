#include "fileiras.h"
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
#include "r8fundo.h"
#include "app.h"
#include "corviva.h"
#include "ilha.h"
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
// O TAMANHO DA TELA: 1920 x 1080 (as pranchas) ou, com NV_720=1, o drawable
// real da TCL (1280 x 720), com a interface de 1920 escalada como no app.
static int janW = 1920, janH = 1080;
static const char *NOMES[] = { "Duna: Parte Dois", "Moana 2", "Severance", "Fallout", "Andor", "O Urso",
                               "Slow Horses", "Shōgun", "The Last of Us", "Ruptura", "Arcane", "Pinguim",
                               "Bebê Rena", "Fargo", "True Detective", "Silo", "Reacher", "Invencível",
                               "Gen V", "Blue Eye Samurai" };

static void gravarPng(const char *png) {
  unsigned char *pix = malloc((size_t)janW * janH * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, janW, janH, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < janH; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (size_t)(janH - 1 - y) * janW * 4, (size_t)janW * 4);
  IMG_SavePNG(s, png);
  SDL_FreeSurface(s);
  free(pix);
  printf("%s\n", png);
}
static int passoMs = 16;
// Diferenca media (0-255) entre a Home desenhada viva agora e a copia parada
// sem veu, com o cartao do menu por cima no tamanho de origem (a copia sai SEM
// ele: o menu o desenha vivo, crescendo; 27/11): a copia tem de ser a Home
// DESTA abertura.
static int indiceMenu = -1;
// Toda abertura do menu passa por aqui: a medida acima sabe qual cartao e o dele.
#define ABRIR(i) (indiceMenu = (i), ctx_abrir(indiceMenu))
static float difCopiaViva(void) {
  static unsigned char a[1920 * 1080 * 4], b[1920 * 1080 * 4];
  double soma = 0; long n = 0;
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  home_desenhar(relogio); menu_desenhar(relogio);
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, a);
  glClear(GL_COLOR_BUFFER_BIT);
  gfx_snap_desenhar_xf(1.0f, 960.0f, 540.0f, 0.0f, 0.0f, 0.0f);
  if (indiceMenu >= 0) home_cartao_foco_por_cima(indiceMenu);
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, b);
  for (long i = 0; i < (long)janW * janH * 4; i += 4 * 7) { for (int c = 0; c < 3; c++) soma += abs(a[i + c] - b[i + c]); n += 3; }
  // NV_DIFPNG=pasta: guarda a Home viva (v) e a copia (c) de cada medida, para
  // ver o que difere.
  if (getenv("NV_DIFPNG")) {
    static int nd; char cam[600]; int y;
    SDL_Surface *sv = SDL_CreateRGBSurfaceWithFormat(0, janW, janH, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Surface *sc = SDL_CreateRGBSurfaceWithFormat(0, janW, janH, 32, SDL_PIXELFORMAT_RGBA32);
    for (y = 0; y < janH; y++) {
      memcpy((char *)sv->pixels + y * sv->pitch, a + (size_t)(janH - 1 - y) * janW * 4, (size_t)janW * 4);
      memcpy((char *)sc->pixels + y * sc->pitch, b + (size_t)(janH - 1 - y) * janW * 4, (size_t)janW * 4);
    }
    snprintf(cam, sizeof cam, "%s/dif%02d-v.bmp", getenv("NV_DIFPNG"), nd); SDL_SaveBMP(sv, cam);
    snprintf(cam, sizeof cam, "%s/dif%02d-c.bmp", getenv("NV_DIFPNG"), nd); SDL_SaveBMP(sc, cam);
    nd++; SDL_FreeSurface(sv); SDL_FreeSurface(sc);
  }
  return (float)(soma / n);
}
// A COPIA SOZINHA (sem o cartao por cima) contra a Home viva, so dentro do
// retangulo `r` do cartaz: alta = o cartaz nao esta na copia (uma imagem so).
static float difCopiaNoCartaz(GfxRect r) {
  static unsigned char a[1920 * 1080 * 4], b[1920 * 1080 * 4];
  double soma = 0; long n = 0;
  int x0 = (int)r.x + 8, x1 = (int)(r.x + r.w) - 8, y0 = (int)r.y + 8, y1 = (int)(r.y + r.h) - 8;
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  home_desenhar(relogio); menu_desenhar(relogio);
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, a);
  glClear(GL_COLOR_BUFFER_BIT);
  gfx_snap_desenhar_xf(1.0f, 960.0f, 540.0f, 0.0f, 0.0f, 0.0f);
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, b);
  for (int y = y0; y < y1; y += 3) for (int x = x0; x < x1; x += 3) {
    long i = ((long)(janH - 1 - y) * janW + x) * 4;
    if (x < 0 || y < 0 || x >= janW || y >= janH) continue;
    for (int c = 0; c < 3; c++) soma += abs(a[i + c] - b[i + c]);
    n += 3;
  }
  return n ? (float)(soma / n) : 0.0f;
}
// A Home desenhada agora (sem atualizar), lida da tela.
static const unsigned char *homeLida(void) {
  static unsigned char a[1920 * 1080 * 4];
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  home_desenhar(relogio); menu_desenhar(relogio);
  glReadPixels(0, 0, janW, janH, GL_RGBA, GL_UNSIGNED_BYTE, a);
  return a;
}
static float luz(const unsigned char *a, int x, int y) {
  const unsigned char *q = a + ((long)(janH - 1 - y) * janW + x) * 4;
  return (q[0] + q[1] + q[2]) / 3.0f;
}
static Uint32 relogioFalso(void) { return relogio; }
static void atrasHome(void *ctx) { (void)ctx; home_desenhar(relogio); menu_desenhar(relogio); }
// O CUSTO DA PINTURA NA GPU (TCL, 26/11: 45-60 ms, que caem no quadro
// SEGUINTE ao da pintura). NV_GPU_PINTURA=ms soma esse tempo ao relogio do
// quadro depois de cada pintura, para provar que a gaveta comeca do zero no
// primeiro quadro visivel mesmo assim.
static int ultQ = -1;
static void quadro(const char *png) {
  relogio += passoMs;
  if (ultQ == R8F_Q_PINTOU && getenv("NV_GPU_PINTURA")) relogio += (Uint32)atoi(getenv("NV_GPU_PINTURA"));
  SDL_PumpEvents();
  tex_bombear(8);
  home_atualizar(0.016f, relogio);
  spainel_atualizar(0.016f, relogio);
  spot_atualizar(0.016f, relogio);
  ctx_atualizar(0.016f, relogio);
  // Como main.c, antes do clear: o tom da interface (Preto puro) e a cor viva
  // assentam aqui a cada quadro. Sem isto o NV_PRETO nao valia (26/11).
  corviva_quadro(0.016f, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
  txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  { float esc, ox, oy, veu; unsigned ver;
    static R8Fundo rf;
    // O MESMO QUADRO DO APP (r8fundo_quadro, chamado por ctxFundoParado em
    // app.c): decidir, pintar, conferir e copiar sao o codigo do app; so o
    // que fica atras do menu (`atrasHome`) e deste harness.
    int q = -1;
    if (ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver)) {
      q = r8fundo_quadro(&rf, 1, esc, ox, oy, veu, ver, ctx_r8_sessao(), relogioFalso, atrasHome, NULL, NULL, 0);
      ctx_r8_fundo_estado(q);
    }
    ultQ = q;
    if (q == R8F_Q_PINTOU || q == R8F_Q_COPIOU) {
      ctx_r8_fundo_pronto(1);
    } else {
      // Como em app.c: com o menu fechado ninguem mexe na copia (ela e por
      // abertura: fundoparado_sessao), e sem copia a Home e desenhada viva.
      ctx_r8_fundo_pronto(0);
      atrasHome(NULL);
    } }
  ctx_desenhar(relogio);
  // A ILHA DO RELOGIO por cima, como em app_desenhar (app_ilha_coberta e
  // app_ilha_opacidade sao a regra do app). NV_SEM_RELOGIO = relogio desligado.
  if (!getenv("NV_SEM_RELOGIO")) {
    float ga = gfx_opacidade_grupo;
    ilha_relogio_visivel(1); ilha_posicionar(1); ilha_coberta(app_ilha_coberta());
    gfx_opacidade_grupo = ga * app_ilha_opacidade();
    ilha_desenhar(relogio);
    gfx_opacidade_grupo = ga;
  }
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
  if (getenv("NV_720")) { janW = 1280; janH = 720; }
  janela = SDL_CreateWindow("Nuvio: r8 quadros", 0, 0, janW, janH, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!janela || !SDL_GL_CreateContext(janela)) return 1;
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, janW, janH);
  gfx_tamanho_alvo(janW, janH);
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
  // O Desdobrar saiu (27/11): quem o tinha gravado abre a Folha (migracao).
  if (getenv("NV_DESDOBRAR_GRAVADO")) fprintf(a, "menuSegurarLocal 1\n");
  if (getenv("NV_REDUZIDO")) fprintf(a, "animacoes 1\nperformanceAutoVersaoLocal 1\nvidroOpacVersaoLocal 1\n");
  fclose(a);
  ajustes_dir(dir);
  // Como a TCL do dono (log de 27/11: vidro=0): pelo mesmo caminho dos Ajustes.
  if (getenv("NV_SEM_VIDRO_TV")) ajustes_definir_vidro(0);
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
    // Logo LOCAL: sem ele o destaque pede o logo na rede (endereco invalido do
    // fixture), a arte "falta" para sempre e a copia parada se repinta a cada
    // 150 ms (fundoparado.h) — o que esconderia um fundo velho na reabertura.
    snprintf(c->logo, sizeof c->logo, "deploy/app/art/logo/%02d.png", i % 10);
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
  // NV_TIPO=FIL_TIPO_* (fileiras.h): o estilo da fileira dos cartazes, depois
  // de ela existir (16 = Poster grande, o da TCL do dono).
  if (getenv("NV_TIPO")) {
    if (!fil_definir_tipo("pop_movie", atoi(getenv("NV_TIPO")))) { printf("FALHOU: estilo NV_TIPO nao aplicado\n"); return 1; }
    for (k = 0; k < 90; k++) quadro(NULL);
  }
  // MENU: o foco desce para a primeira fileira e vai ao terceiro cartaz.
  if (!strcmp(peca, "MC") || !strcmp(peca, "COMP")) {
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
    if (getenv("NV_TECLAS")) fecha = 1e9f;   // com roteiro, so ele fecha (E)
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
    if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz\n"); return 1; }
    // SEGURAR OK, OPCAO A (dono, 27/11): o anel de foco vira o progresso, de
    // 150 ms (antes disso e o toque) a 700 ms (abre o menu). A Home marca o
    // inicio com SDL_GetTicks: o relogio do teste parte dele.
    if (!getenv("NV_REDUZIDO") && !getenv("NV_SEM_SEGURAR")) {
      static unsigned char base[1920 * 1080 * 4];
      int antes = 0, cedo = 0, n = 0, fora = 0, dentro = 0, abriu = -1, k;
      unsigned long uplBase = 0, uplMeio = 0;
      float pior = 0, meio = -1, claro = 0, escuro = 255, difCartaz = 0;
      GfxRect R = hi.rect;
      // AS ARTES CHEGAM EM FIOS DE TEMPO REAL (na CI, bem mais devagar): antes
      // de apertar, 400 ms reais sem textura nova (teto 15 s). Senao a arte do
      // cartaz pode chegar no meio da medida e mudar o miolo dele.
      { unsigned long u0 = tex_upl_total; Uint32 quieto = SDL_GetTicks(), ini = quieto;
        while (SDL_GetTicks() - quieto < 400u && SDL_GetTicks() - ini < 15000u) {
          quadro(NULL); SDL_Delay(8);
          if (tex_upl_total != u0) { u0 = tex_upl_total; quieto = SDL_GetTicks(); }
        }
        printf("segurar: artes assentadas em %u ms reais\n", SDL_GetTicks() - ini); }
      relogio = SDL_GetTicks();
      { SDL_Event ok; memset(&ok, 0, sizeof ok); ok.type = SDL_KEYDOWN; ok.key.keysym.sym = SDLK_RETURN; home_evento(&ok); }
      for (k = 1; k <= 50 && abriu < 0; k++) {
        float tms, p, esp;
        quadro(NULL);
        tms = 16.0f * (float)k;
        p = home_segurar_contorno();
        if (ctx_aberto()) { abriu = (int)tms; break; }
        // O cartaz ja afunda no KEYDOWN (pressdip.h, 91,8 %, de antes): a base
        // de comparacao e ele afundado, ainda sem contorno (112 ms).
        if (k == 7) { memcpy(base, (homeLida(), homeLida()), sizeof base); uplBase = tex_upl_total; }
        if (tms < 150.0f) { if (p >= 0.0f) cedo++; continue; }
        if (p < 0.0f) { antes++; continue; }
        esp = (tms - 150.0f) / 550.0f; if (esp > 1.0f) esp = 1.0f;
        if (fabsf(p - esp) > pior) pior = fabsf(p - esp);
        n++;
        // No meio do caminho: o topo a direita do centro ja foi percorrido
        // (branco); a esquerda, ainda nao (apagado). Dentro do cartaz, nada.
        if (meio < 0 && p >= 0.45f && p <= 0.6f) {
          // Duas vezes: um texto novo (a dica antiga) so rasteriza no quadro
          // seguinte (orcamento de texto por quadro).
          const unsigned char *a = (homeLida(), homeLida());
          uplMeio = tex_upl_total;
          HomeItem fh;
          if (home_item_focado(&fh)) R = fh.rect;
          int yA = (int)(R.y - 2.0f), xC = (int)(R.x + R.w * 0.5f);
          meio = p;
          claro = luz(a, xC + (int)(R.w * 0.25f), yA);
          // NV_SEG_PNG=arquivo.bmp: guarda este quadro (o contorno pela metade).
          if (getenv("NV_SEG_PNG")) { SDL_Surface *sv = SDL_CreateRGBSurfaceWithFormat(0, janW, janH, 32, SDL_PIXELFORMAT_RGBA32);
            for (int y = 0; y < janH; y++) memcpy((char *)sv->pixels + y * sv->pitch, a + (size_t)(janH - 1 - y) * janW * 4, (size_t)janW * 4);
            SDL_SaveBMP(sv, getenv("NV_SEG_PNG")); SDL_FreeSurface(sv);
            printf("  rect %.0f %.0f %.0f %.0f\n", R.x, R.y, R.w, R.h); }
          escuro = luz(a, xC - (int)(R.w * 0.25f), yA);
          { double s = 0; long m = 0;
            for (int y = (int)R.y + 6; y < (int)(R.y + R.h) - 6; y += 3)
              for (int x = (int)R.x + 6; x < (int)(R.x + R.w) - 6; x += 3) {
                const unsigned char *q1 = a + ((long)(janH - 1 - y) * janW + x) * 4, *q0 = base + ((long)(janH - 1 - y) * janW + x) * 4;
                for (int c = 0; c < 3; c++) s += abs(q1[c] - q0[c]);
                m += 3;
              }
            difCartaz = m ? (float)(s / m) : 0; }
        }
      }
      (void)fora; (void)dentro;
      OKC(!cedo, "segurar: nos primeiros 150 ms (o toque) nao aparece nada (%d quadros com contorno)", cedo);
      OKC(n >= 25 && !antes && pior < 0.04f, "segurar: o contorno acompanha o tempo de 150 a 700 ms (%d quadros, pior diferenca %.3f)", n, pior);
      if (ajustes_vidro()) printf("(segurar: conferencia de pixel do anel so sem vidro, como na TCL)\n");
      else OKC(meio > 0 && claro > 200.0f && escuro < 110.0f,
          "segurar: o traco branco anda no sentido horario a partir do meio do topo (topo direito %.0f, topo esquerdo %.0f, em %.0f %%)", claro, escuro, meio * 100.0f);
      OKC(meio > 0 && difCartaz < 0.02f, "segurar: nada e desenhado dentro do cartaz (diferenca %.2f; texturas chegadas entre os dois quadros: %lu)",
          difCartaz, uplMeio - uplBase);
      OKC(abriu >= 690 && abriu <= 720 && home_segurar_contorno() < 0.0f, "segurar: o menu abre em 700 ms (%d ms) e o contorno sai com ele", abriu);
      // Fecha e solta: o resto do comportamento parte da Home parada.
      { SDL_Event e2; memset(&e2, 0, sizeof e2); e2.type = SDL_KEYDOWN; e2.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e2);
        for (k = 0; k < 40; k++) quadro(NULL);
        e2.type = SDL_KEYUP; e2.key.keysym.sym = SDLK_RETURN; home_evento(&e2);
        for (k = 0; k < 20; k++) quadro(NULL); }
      if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz depois do segurar\n"); return 1; }
      // NV_SO_SEGURAR: so esta parte (a passada sem vidro, como a TCL).
      if (getenv("NV_SO_SEGURAR")) { printf("r8_menu_segurar: %d falhas\n", falhas); return falhas ? 1 : 0; }
    }
    // A PINTURA DA COPIA CUSTA NA GPU (TCL: 45-60 ms no quadro seguinte):
    // simulado aqui (NV_GPU_PINTURA), a gaveta tem de comecar do zero mesmo assim.
    setenv("NV_GPU_PINTURA", "60", 0);
    ctx_fileira("pop_movie", "Populares"); ctx_dispensar_retomar(0); ABRIR(hi.indice);
    { float x0 = -1, ant = 1e9f, piorVeu = 0, piorRel = 0, meio = -1, passoMax = 0, piorCresce = 0, piorEsc = 0;
      int volta = 0, folha = !getenv("NV_REDUZIDO"), cresceu = 0;
      for (i = 0; i < 40; i++) {
        quadro(NULL);
        if (ctx_r8_geo(&g) && g.superficieA > 0.0f) {
          float m = (1920.0f - g.superficie.x) / 640.0f, rel = app_ilha_opacidade();
          if (x0 < 0) x0 = g.superficie.x;
          if (g.superficie.x > ant + 0.01f) volta = 1;
          if (ant < 1e8f && ant - g.superficie.x > passoMax) passoMax = ant - g.superficie.x;
          ant = g.superficie.x;
          if (folha && ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver)) {
            if (fabsf(veu - 0.5f * m) > piorVeu) piorVeu = fabsf(veu - 0.5f * m);
            if (fabsf(rel - (1.0f - m)) > piorRel) piorRel = fabsf(rel - (1.0f - m));
            if (meio < 0 && m > 0.3f && m < 0.7f) meio = rel;
            // O CARTAZ CRESCE COM A GAVETA (proposta aprovada, 27/11): 1 + 0,04 m,
            // e a Home atras nao recua (escala 1).
            if (g.cartazEscala > 0.0f) { cresceu = 1; if (fabsf(g.cartazEscala - (1.0f + 0.04f * m)) > piorCresce) piorCresce = fabsf(g.cartazEscala - (1.0f + 0.04f * m)); }
            if (fabsf(esc - 1.0f) > piorEsc) piorEsc = fabsf(esc - 1.0f);
          } else if (!folha && g.cartazEscala > 0.0f && fabsf(g.cartazEscala - 1.0f) > piorCresce) piorCresce = fabsf(g.cartazEscala - 1.0f);
        } }
      if (folha) {
        OKC(x0 >= 1920.0f - 80.0f && !volta, "Folha: entra de fora da tela pela direita (1o quadro x %.0f, com 60 ms de GPU depois da pintura) e so anda para a esquerda", x0);
        // O passo maximo legitimo: a curva de entrada anda ate 4,7 px/ms, ~76 px
        // num quadro de 16 ms. Um quadro que pagou a pintura (76 ms) saltava
        // ~270 px: a gaveta "aparecia ja no meio" (TCL, 26/11).
        OKC(passoMax < 90.0f, "Folha: nenhum salto na entrada, mesmo com a pintura cara na GPU (maior passo %.0f px)", passoMax);
        OKC(piorVeu < 0.01f, "Folha: o escurecimento da Home sai do mesmo valor da gaveta (pior diferenca %.3f)", piorVeu);
        OKC(piorRel < 0.01f && meio > 0.25f && meio < 0.75f && app_ilha_opacidade() < 0.01f && app_ilha_coberta(),
            "Folha: o relogio some junto com a gaveta (no meio %.2f, aberta %.2f, pior %.3f)", meio, app_ilha_opacidade(), piorRel);
        OKC(cresceu && piorCresce < 0.002f, "Folha: o proprio cartaz cresce junto com a gaveta, ate 4 %% (pior diferenca %.4f)", piorCresce);
        OKC(piorEsc < 0.0001f, "Folha: a Home atras nao recua nem da zoom out (pior escala fora de 1: %.4f)", piorEsc);
      } else OKC(piorCresce < 0.0001f, "reduzidas: o cartaz nao cresce (pior %.4f)", piorCresce); }
    ctx_r8_geo(&g);
    OKC(ctx_aberto() && g.visivel, "abre a Folha%s", getenv("NV_DESDOBRAR_GRAVADO") ? " mesmo com o Desdobrar gravado nos dados antigos" : "");
    OKC(ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver) && esc == 1.0f && veu > 0.3f, "a Home atras congela, fica no lugar (%.3f) e escurece (%.2f)", esc, veu);
    // UMA IMAGEM SO DO CARTAZ (TCL, 27/11: um segundo poster aparecia por cima
    // do original): a copia parada nao tem o cartaz; so o vivo, por cima.
    if (ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver) && home_item_focado(&hi) && hi.indice == indiceMenu) {
      float dc = difCopiaNoCartaz(hi.rect);
      OKC(dc > 25.0f, "a copia da Home sai sem o cartaz do menu: ele so existe vivo, por cima (diferenca no cartaz %.1f)", dc);
    }
    OKC(fabsf(g.superficie.x + g.superficie.w - 1920.0f) < 0.5f && g.superficie.y == 0.0f && g.superficie.h == 1080.0f,
          "Folha: gaveta encostada na borda direita, da altura da tela (x %.1f w %.1f)", g.superficie.x, g.superficie.w);
    // Voltar fecha: a superficie vai a zero sem saltar e a Home volta viva.
    memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
    // A Folha (gaveta) sai acelerando para a direita: no fim do fechar (k^2,4
    // em 0,26 s) anda ate 94 px num quadro de 16 ms.
#define SALTO(g, ant) (fabsf(g.superficie.x - (ant)) > 100.0f)
#define VAL(g) (g.superficie.x)
    { int salto = 0; antA = VAL(g);
      for (i = 0; i < 30; i++) { quadro(NULL); if (ctx_r8_geo(&g)) { if (SALTO(g, antA)) { salto = 1; printf("  salto no quadro %d: %.1f -> %.1f (a %.2f)\n", i, antA, VAL(g), g.superficieA); } antA = VAL(g); } }
      OKC(!salto, "fechar nao salta"); }
    OKC(!ctx_aberto() && !ctx_r8_geo(&g) && !ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver),
        "fechado: nada do menu na tela e a Home volta a ser desenhada viva");
    OKC(app_ilha_opacidade() > 0.99f && !app_ilha_coberta(), "fechado: o relogio volta (%.2f)", app_ilha_opacidade());
    // A COPIA E DA ABERTURA NOVA (TCL, 26/11: o menu de "A Odisseia" abria
    // sobre a Home de "Homem de Ferro"). Fecha, anda o foco na Home (muda o
    // destaque e a fileira) e reabre: a copia tem de ser a Home de agora.
    // A medida vale logo no comeco da reabertura (no 2o quadro e no 40o), e a
    // 1a abertura fica 4,8 s: antes de 26/11 (noite) havia repinturas por
    // tempo que trocavam a copia velha sozinhas e escondiam o defeito no teste.
    { float d0, d1, d2;
      ABRIR(hi.indice);
      for (i = 0; i < 300; i++) quadro(NULL);
      d0 = difCopiaViva();
      e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      for (i = 0; i < 30; i++) quadro(NULL);
      memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RIGHT; home_evento(&e);
      e.type = SDL_KEYUP; home_evento(&e);
      for (i = 0; i < 60; i++) quadro(NULL);
      if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz depois da seta\n"); return 1; }
      ABRIR(hi.indice);
      for (i = 0; i < 2; i++) quadro(NULL);
      d1 = difCopiaViva();
      for (i = 0; i < 38; i++) quadro(NULL);
      d2 = difCopiaViva();
      OKC(d0 < 4.0f && d1 < 4.0f && d2 < 4.0f,
          "a copia parada e a Home desta abertura desde o 2o quadro (dif %.2f na 1a; %.2f e %.2f depois de mudar a Home)", d0, d1, d2);
      memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      for (i = 0; i < 30; i++) quadro(NULL); }
    // O MENU ABERTO NO MEIO DA TROCA DO DESTAQUE (TCL, 26/11: dois titulos
    // sobrepostos atras do menu de "A Odisseia"). Anda o foco (o destaque
    // troca depois de 220 ms de repouso e esvanece/desliza por ~0,5 s) e abre
    // no meio: a copia tem de ser a Home de um titulo so — igual a Home viva
    // assentada — logo no primeiro quadro da curva.
    { float d = 99; Uint32 ini;
      memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RIGHT; home_evento(&e);
      e.type = SDL_KEYUP; home_evento(&e);
      for (i = 0; i < 24; i++) quadro(NULL);   // ~380 ms: destaque no meio da troca
      if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz depois da seta\n"); return 1; }
      ABRIR(hi.indice);
      for (i = 0; i < 12 && !ctx_r8_inicio(&ini); i++) quadro(NULL);
      quadro(NULL);
      d = difCopiaViva();
      OKC(d < 4.0f, "aberto no meio da troca do destaque: a copia e a Home de um titulo so (dif %.2f)", d);
      memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      for (i = 0; i < 30; i++) quadro(NULL); }
    // ESTILOS AO VIVO (TCL, 26/11: 4 repinturas por seta, engasgos de 40-75 ms;
    // no Poster grande o cartaz do menu ficava gigante sobre a fileira antiga
    // e o foco da Home ia para outro titulo). Cada seta muda o estilo da
    // fileira por baixo: a copia tem de mostrar o estilo novo ja no 6o quadro
    // (~100 ms), o foco da Home nao sai do titulo do menu, ha UMA pintura por
    // estilo e nenhuma com o menu parado.
    if (!getenv("NV_REDUZIDO")) {
      int k, pin0, pinSetas, comp0, compSetas, foraTitulo = 0, flutua = 0, nSetas = 12; float pior = 0, dk;
      HomeItem f;
      memset(&e, 0, sizeof e);
      ABRIR(hi.indice);
      for (i = 0; i < 40; i++) quadro(NULL);
      for (k = 0; k < 2; k++) { e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_DOWN; ctx_evento(&e); for (i = 0; i < 10; i++) quadro(NULL); }
      e.key.keysym.sym = SDLK_RETURN; e.type = SDL_KEYUP; ctx_evento(&e); e.type = SDL_KEYDOWN; ctx_evento(&e);
      for (i = 0; i < 40; i++) quadro(NULL);
      pin0 = (int)r8fundo_pinturas; comp0 = (int)r8fundo_pinturas_complemento;
      for (k = 0; k < nSetas; k++) {
        e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_DOWN; ctx_evento(&e);
        for (i = 0; i < 6; i++) quadro(NULL);
        dk = difCopiaViva(); if (dk > pior) pior = dk;
        // Um estilo que nao mostra o titulo (Heroi: um cartao so) leva o foco
        // para outro cartao; nos outros, o foco fica no titulo do menu.
        if ((!home_item_focado(&f) || f.indice != hi.indice) && home_titulo_na_fileira(hi.indice)) foraTitulo++;
        // Titulo fora da fileira (Heroi): o menu nao desenha a arte guardada no
        // retangulo antigo (o cartaz flutuando sobre a fileira nova, TCL 26/11).
        { CtxR8Geo gk; if (ctx_r8_geo(&gk) && gk.cartazGuardado) flutua++; }
        printf("  seta %2d: dif %.2f foco %d (menu %d) rect %.0fx%.0f pinturas %d\n", k + 1, dk,
               home_item_focado(&f) ? f.indice : -1, hi.indice, f.rect.w, f.rect.h, (int)r8fundo_pinturas - pin0);
        for (i = 0; i < 20; i++) quadro(NULL);
      }
      compSetas = (int)r8fundo_pinturas_complemento - comp0;
      pinSetas = (int)r8fundo_pinturas - pin0 - compSetas;
      OKC(pior < 4.0f, "estilos ao vivo: a copia mostra o estilo novo ~100 ms depois de cada seta (pior dif %.2f em %d estilos)", pior, nSetas);
      OKC(!foraTitulo, "estilos ao vivo: o foco da Home fica no titulo do menu (%d vezes fora)", foraTitulo);
      OKC(!flutua, "estilos ao vivo: nenhum cartaz guardado desenhado por cima da fileira (%d vezes)", flutua);
      OKC(pinSetas >= 1 && pinSetas <= nSetas, "estilos ao vivo: no maximo uma pintura por estilo (%d pinturas em %d setas; mais %d de complemento: arte que chegou, texto que faltava)", pinSetas, nSetas, compSetas);
      pin0 = (int)r8fundo_pinturas;
      for (i = 0; i < 360; i++) quadro(NULL);   // 5,8 s parado
      OKC((int)r8fundo_pinturas == pin0, "menu parado 5,8 s: nenhuma repintura (%d)", (int)r8fundo_pinturas - pin0);
      e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      for (i = 0; i < 20; i++) quadro(NULL);
      e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
      for (i = 0; i < 40; i++) quadro(NULL);
      if (!home_item_focado(&hi)) { printf("FALHOU: sem cartaz depois dos estilos\n"); return 1; } }
    // Reabrir no meio do fechar continua do ponto em que estava.
    ABRIR(hi.indice);
    for (i = 0; i < 40; i++) quadro(NULL);
    e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e);
    for (i = 0; i < 3; i++) quadro(NULL);
    ctx_r8_geo(&g); antA = VAL(g);
    ABRIR(hi.indice);
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
      ABRIR(hi.indice);
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
    int falhas = 0, conf = 0, ev = 0;
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
        if (ev == 0) { ctx_fileira("pop_movie", "Populares"); ctx_dispensar_retomar(0); ABRIR(hi.indice); }
        else { e.key.keysym.sym = ev == 1 || ev == 2 ? SDLK_DOWN : ev == 3 ? SDLK_UP : SDLK_ESCAPE; ctx_evento(&e); }
        ctx_atualizar(0.0f, base + (Uint32)(EV[ev] * 1000.0 + 0.5));
        // A CURVA COMECA QUANDO O FUNDO ESTA PRONTO (ctxmenu_r8.inc, r8Espera):
        // os quadros da espera rodam aqui e o resto da prancha e conferido a
        // partir do quadro 0 real (a prancha comeca a curva no OK, 1,3 s). O
        // atraso tem teto: no maximo 6 quadros.
        if (ev == 0) {
          Uint32 ini = 0, ok0 = base + 1300u; int k;
          relogio = ok0; passoMs = 16;
          for (k = 0; k < 40 && !ctx_r8_inicio(&ini); k++) quadro(NULL);
          if (!ctx_r8_inicio(&ini)) { printf("FALHOU a curva de abertura nunca comecou\n"); falhas++; }
          else {
            printf("abertura: a curva comecou %u ms depois do OK (%d quadros de espera)\n", ini - ok0, k);
            if (ini - ok0 > 6u * 16u) { printf("FALHOU espera longa demais (%u ms)\n", ini - ok0); falhas++; }
            base += ini - ok0;
          }
        }
        ev++;
      }
      relogio -= 16; passoMs = 16;   // quadro() soma o passo: chega em `relogio` exato
      { Uint32 alvo = base + (Uint32)(t * 1000.0 + 0.5); relogio = alvo - (Uint32)passoMs; }
      quadro(NULL);
      if (!ctx_r8_fundo(&esc, &ox, &oy, &veu, &ver)) veu = 0;
      ctx_r8_geo(&g);
#define CONF(nome, app, pr, tol) do { conf++; if (fabsf((app) - (pr)) > (tol)) { if (falhas < 25) \
        printf("FALHOU t=%.3f %s: prancha %.3f app %.3f\n", t, nome, (double)(pr), (double)(app)); falhas++; } } while (0)
      if (g.visivel) {
        // A gaveta anda 640 px em 0,44 s, ate 4,7 px/ms no pico; o relogio do
        // app e em ms inteiros (ate 0,33 ms fora do instante da prancha).
        CONF("folha.x", g.superficie.x, v[0], 2.0f);
        CONF("folha.o", g.superficieA, v[1], 0.01f);
        if (v[3] > 0.0f || g.pilulaA > 0.0f) { CONF("pilula.y", g.pilula.y, v[2], 1.0f); CONF("pilula.o", g.pilulaA, v[3], 0.01f); }
        CONF("titulo.y", g.tituloY, v[4], 1.0f); CONF("titulo.o", g.tituloA, v[5], 0.01f);
      } else CONF("folha.o", 0.0f, v[1], 0.01f);
      CONF("veu", veu, v[6], 0.01f);
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
        else if (!strcmp(peca, "MC")) {
          HomeItem hi;
          const char *orig = getenv("NV_ORIGEM") ? getenv("NV_ORIGEM") : "poster";
          if (!strcmp(orig, "colecao")) ctx_abrir_fileira("pop_movie", "Populares");
          else if (home_item_focado(&hi)) {
            ctx_fileira(strcmp(orig, "hero") ? "pop_movie" : NULL, "Populares");
            ctx_dispensar_retomar(0); ABRIR(hi.indice);
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
      // ROTEIRO DE TECLAS (NV_TECLAS="2.0:D,2.1:O,..."): no tempo da prancha,
      // D/U/L/R = setas, O = OK, E = Voltar; vai ao menu se ele esta aberto,
      // senao a Home. Substitui o roteiro fixo abaixo.
      if (alvo && getenv("NV_TECLAS")) {
        static int nr = -1, ir; static float rt[64]; static char rk[64];
        if (nr < 0) {
          const char *q = getenv("NV_TECLAS"); nr = 0;
          while (*q && nr < 64) { float tt; char kk; int n = 0;
            if (sscanf(q, "%f:%c%n", &tt, &kk, &n) == 2) { rt[nr] = tt; rk[nr++] = kk; q += n; } else break;
            if (*q == ',') q++; }
        }
        while (ir < nr && t + 0.0005f >= rt[ir]) {
          SDL_Event e; SDL_Keycode k = rk[ir] == 'D' ? SDLK_DOWN : rk[ir] == 'U' ? SDLK_UP : rk[ir] == 'L' ? SDLK_LEFT
                                     : rk[ir] == 'R' ? SDLK_RIGHT : rk[ir] == 'E' ? SDLK_ESCAPE : SDLK_RETURN;
          memset(&e, 0, sizeof e); e.key.keysym.sym = k;
          if (ctx_aberto()) {
            if (k == SDLK_RETURN) { e.type = SDL_KEYUP; ctx_evento(&e); e.type = SDL_KEYDOWN; ctx_evento(&e); }
            else { e.type = SDL_KEYDOWN; ctx_evento(&e); }
          } else { e.type = SDL_KEYDOWN; home_evento(&e); e.type = SDL_KEYUP; home_evento(&e); }
          ir++;
          { HomeItem fh; int ok = home_item_focado(&fh);
            printf("[roteiro] t=%.2f tecla=%c pagina-ctx foco da Home: indice %d rect %.0f,%.0f %.0fx%.0f\n", t, rk[ir - 1],
                   ok ? fh.indice : -1, ok ? fh.rect.x : 0, ok ? fh.rect.y : 0, ok ? fh.rect.w : 0, ok ? fh.rect.h : 0); }
        }
      } else
      if (alvo && ctx_aberto() && !strcmp(peca, "MC")) {
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
