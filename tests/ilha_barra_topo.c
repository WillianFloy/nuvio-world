// World 23/11 (dono: "mostra a barra, mas sem bugs"): com a barra do menu no
// topo (layout Padrao + menuTopoLocal 1) o corpo da ilha (Central, Fontes) e o
// modal do cartao brotam da pilula mas ASSENTAM ABAIXO da barra; sem barra no
// topo ficam onde sempre ficaram. Teste de COMPORTAMENTO: mede o retangulo que
// a ilha realmente desenha (ilha_rect) depois da mola assentar.
#include "ajustes.h"
#include "ilha.h"
#include "layout.h"
#include "menu.h"
#include "text.h"
#include "dados.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "nvtmp.h"

static char dir[NVT_MAX], caminho[700];
static void gravar(const char *txt) {
  FILE *f = fopen(caminho, "w"); assert(f);
  fputs(txt, f); fclose(f);
  ajustes_dir(dir);
}
static void corpoNada(GfxRect r, float a, void *u) { (void)r; (void)a; (void)u; }
// Abre o corpo e deixa a mola assentar; devolve o y do retangulo desenhado.
static float assentaCorpo(float h) {
  float x, y, w, hh = 0;
  Uint32 t = 1000;
  for (int i = 0; i < 400; i++, t += 16) {
    ilha_relogio_visivel(1);
    ilha_posicionar(1);
    ilha_corpo(492.0f, h, corpoNada, NULL);
    ilha_desenhar(t);
  }
  assert(ilha_rect(&x, &y, &w, &hh));
  return y;
}
static void fechaCorpo(void) {
  Uint32 t = 100000;
  for (int i = 0; i < 400; i++, t += 16) { ilha_relogio_visivel(1); ilha_posicionar(1); ilha_desenhar(t); }
}

int main(void) {
  nvt_set(dir, sizeof dir, "/nuvio-ilha-barra-XXXXXX"); assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  assert(SDL_GL_CreateContext(SDL_CreateWindow("t", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN)));
  assert(txt_iniciar("deploy/app", 1));
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);

  // Sem barra no topo: o corpo nasce na linha da pilula, como sempre.
  gravar("homeLayoutLocal 1\nmodernSidebar 0\nmenuTopoLocal 0\n");
  assert(!ajustes_menu_topo() && menu_topo_reserva() == 0.0f);
  float yLivre = assentaCorpo(340.0f);
  assert(fabsf(yLivre - NV_ILHA_Y / gfx_escala_txt()) < 2.0f);
  fechaCorpo();

  // Barra no topo (Padrao = homeLayoutLocal 1): a reserva existe e o corpo assenta ABAIXO dela.
  gravar("homeLayoutLocal 1\nmodernSidebar 0\nmenuTopoLocal 1\n");
  assert(ajustes_menu_topo());
  float reserva = menu_topo_reserva() / gfx_escala_txt();
  assert(reserva > 80.0f && reserva < 220.0f);   // a barra tem ~100 px; nao e zero nem tela inteira
  float yBarra = assentaCorpo(340.0f);
  assert(yBarra >= reserva - 1.0f);
  assert(yBarra > yLivre + 40.0f);
  fechaCorpo();

  // Layout Moderno (rail lateral): sem a faixa do Padrao, nada muda de lugar.
  gravar("homeLayoutLocal 0\nmodernSidebar 1\nmenuTopoLocal 0\n");
  assert(menu_topo_reserva() == 0.0f);
  assert(fabsf(assentaCorpo(340.0f) - yLivre) < 2.0f);

  puts("PASS: corpo da ilha assenta abaixo da barra do topo (e so com ela)");
  return 0;
}
