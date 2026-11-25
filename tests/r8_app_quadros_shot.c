// O LADO "APP" DO COMPARADOR (tools/r8/comparar.py): abre a peca na Home real
// com arte de verdade e grava os quadros nos MESMOS instantes da prancha
// (tempo da prancha; a abertura comeca onde a prancha comeca a abrir).
//   bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4 3.1
// Pecas desta fase (a base de 23/11): BU = Busca (CH-), SV = Salvos (CH+).
// As pecas novas (MC, MA, S1, S2, C2, HO, HR) entram aqui a cada fase.
// NAO ENTRA NA SUITE (*_shot).
#include "addons.h"
#include "ajustes.h"
#include "catalogo.h"
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
static void quadro(const char *png) {
  relogio += 16;
  SDL_PumpEvents();
  tex_bombear(8);
  home_atualizar(0.016f, relogio);
  spainel_atualizar(0.016f, relogio);
  spot_atualizar(0.016f, relogio);
  txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  home_desenhar(relogio);
  menu_desenhar(relogio);
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
  tex_iniciar(192);
  snprintf(cam, sizeof cam, "%s/cache", dir);
  tex_cache_dir(cam);
  gfx_icones_dir("deploy/app/art");
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dir);
  if (!(a = fopen(cam, "w"))) return 1;
  // A TCL do dono: Home Moderna, menu do topo, relogio ligado, Montserrat.
  fprintf(a, "idioma 0\nhomeLayoutLocal 1\nmodernSidebar 1\nmenuTopoLocal %d\nselected_theme 9\n",
          getenv("NV_SEM_TOPO") ? 0 : 1);
  fclose(a);
  ajustes_dir(dir);
  if (!home_iniciar("deploy/app/art")) return 1;
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
      }
      if (alvo && !fechou && t + 0.0005f >= fecha) {
        fechou = 1;
        if (!strcmp(peca, "BU")) spot_fechar();
        else if (!strcmp(peca, "SV")) spainel_fechar();
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
