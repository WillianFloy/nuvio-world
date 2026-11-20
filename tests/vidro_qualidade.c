// "Qualidade do vidro": Alta / Desempenho / Automatica and the ladder rhythm (gfx side).
//   bash tests/vidro_qualidade.sh
#include "gfx.h"
#include "ajustes.h"
#include "dados.h"
#include "ilha.h"
#include "menu.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const int LW = 1920, LH = 1080;
static int falhas;

static void confere(int ok, const char *nome) {
  printf("%s: %s\n", ok ? "PASSA" : "FALHA", nome);
  falhas += !ok;
}

// Home frame: a page under a bar, the clock island and a glass panel, the
// backdrop colour changing every frame so frosted glass has to capture.
static float fundo = 0.05f;
static int sujar = 1;   // 0: the backdrop changes only through its hash (the app's normal scroll)
static void quadro(Uint32 agora) {
  int i;
  gfx_novo_quadro();
  glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
  if (sujar) gfx_vidro_sujar();   // changing art: forces the backdrop hash off last frame's
  gfx_cor((GfxRect){ 0, 0, 1920, 1080 }, 0, fundo, fundo, fundo * 1.2f, 1.0f);
  for (i = 0; i < 12; i++)
    gfx_cor((GfxRect){ 0, i * 90.0f - 80.0f, 1920, 90 }, 0, .1f + .06f * i, .2f, .5f - .03f * i, 1.0f);
  menu_atualizar(0.016f, agora);
  menu_desenhar(agora);
  ilha_desenhar(agora);
  gfx_vidro_ou_solido((GfxRect){ 1240, 300, 560, 600 }, 36.0f / 600.0f, 0.80f, 1.0f, .12f, .13f, .15f, .95f);
}

// Frames with a changing backdrop; returns the worst capture/pass counts and
// totals the frames' frosted / smoked glasses.
static int cap, pas, fonte, chapado;
static double copiatot;
static void roda(int n, Uint32 *t) {
  int k;
  cap = pas = fonte = chapado = 0; copiatot = 0.0;
  for (k = 0; k < n; k++) {
    fundo = 0.05f + 0.01f * ((k % 20) + 1);
    quadro((*t)++ * 16u);
    if (gfx_vidro_capturas > cap) cap = gfx_vidro_capturas;
    copiatot += gfx_vidro_copia;
    if (gfx_vidro_passadas > pas) pas = gfx_vidro_passadas;
    fonte += gfx_vidro_com_fonte; chapado += gfx_vidro_chapados;
  }
}

int main(void) {
  static char dir[] = "/tmp/nuvio-vidro-qual-XXXXXX";
  char cam[700];
  FILE *f;
  int i;
  Uint32 t = 0;
  GLuint fbo, tex;
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("vidro", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
  glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);

  assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dir);
  f = fopen(cam, "w"); assert(f);
  fputs("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\nidioma 0\n", f); fclose(f);
  ajustes_dir(dir);
  setenv("NUVIO_SHOT_VIDRO_FOSCO", "1", 1);

  menu_iniciar();
  menu_pilula_mostrar(1.0f);
  ilha_relogio_visivel(1);

  // Alta: the current frost, a capture per frame while the backdrop changes.
  setenv("NUVIO_SHOT_VIDRO_QUAL", "2", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro() && ajustes_vidro_fosco() && ajustes_vidro_qualidade() == 2);
  roda(150, &t); roda(20, &t);
  printf("Alta: cap<=%d pas<=%d fonte=%d chapado=%d\n", cap, pas, fonte, chapado);
  confere(cap == 1 && fonte > 0, "Alta: frost unchanged (one capture per frame)");

  // Desempenho: smoked glass everywhere, no capture and no blur pass, same
  // frames and same backdrop.
  setenv("NUVIO_SHOT_VIDRO_QUAL", "1", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro_qualidade() == 1);
  roda(150, &t); roda(20, &t);
  printf("Desempenho: cap<=%d pas<=%d fonte=%d chapado=%d\n", cap, pas, fonte, chapado);
  confere(cap == 0 && pas == 0 && fonte == 0 && chapado > 0, "Desempenho: Home draws no blur (smoked glass only)");

  // Automatica: frost. Nothing in gfx smokes it on its own any more (the glass
  // governor that used to, gfx_vidro_governar, was dead code in production and
  // was removed): the effects ladder decides (gpunivel.c, tests/gpun_vidro.sh).
  setenv("NUVIO_SHOT_VIDRO_QUAL", "0", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro_qualidade() == 0);
  roda(150, &t); roda(20, &t);
  confere(cap == 1 && fonte > 0, "Automatica: frost at first");
  roda(600, &t);
  confere(cap == 1 && fonte > 0 && chapado == 0, "Automatica: 600 frames later still frost (no hidden drop)");

  // THE LADDER'S GLASS RHYTHM, gfx side of the contract (gpun_quadro_inicio ->
  // gfx_vidro_nivel_definir). Same moving backdrop, 120 frames per rhythm:
  // rhythm 2 must capture less often AND blur with fewer passes, still frosted.
  {
    int capT[3] = {0}, pasT[3] = {0}, fonteT[3] = {0}, k, nv;
    sujar = 0;
    for (nv = 1; nv <= 2; nv++) {
      gfx_vidro_nivel_definir(nv);
      for (k = 0; k < 4; k++) quadro(t++ * 16u);   // parado: the next change is a cut
      for (k = 0; k < 120; k++) {
        fundo = 0.30f + 0.002f * (k + 1) + 0.3f * nv;
        quadro(t++ * 16u);
        capT[nv] += gfx_vidro_capturas; pasT[nv] += gfx_vidro_passadas; fonteT[nv] += gfx_vidro_com_fonte;
      }
    }
    gfx_vidro_nivel_definir(1); sujar = 1;
    printf("ritmo 1: capturas=%d passadas=%d vidros-foscos=%d | ritmo 2: capturas=%d passadas=%d vidros-foscos=%d\n",
           capT[1], pasT[1], fonteT[1], capT[2], pasT[2], fonteT[2]);
    confere(capT[1] >= 55 && capT[1] <= 65, "ritmo 1: backdrop captured every 2nd moving frame (~60/120)");
    // Not exactly 40: the menu pill and the clock island move between frames
    // here, and a glass region that leaves the captured area forces a fresh
    // capture (vidroFundo `fora`). Measured: 45/120 (~25 % fewer than ritmo 1).
    confere(capT[2] >= 38 && capT[2] * 4 <= capT[1] * 3 + 2, "ritmo 2: at most 3/4 of ritmo 1's captures, never below every 3rd frame");
    confere(pasT[2] * capT[1] < pasT[1] * capT[2], "ritmo 2: fewer blur passes per capture (1 Kawase instead of 2)");
    confere(fonteT[2] > 0 && fonteT[2] == fonteT[1], "ritmo 2: same frosted glasses drawn (not smoked)");
  }

  // Interface de vidro OFF with frost kept (Alta): no glass consumes a
  // backdrop, so the zero-region fallback must not blur the full screen on
  // every changed frame. Then back ON: frost resumes and captures again.
  setenv("NUVIO_SHOT_VIDRO_QUAL", "2", 1); ajustes_teste_vidro_env();
  ajustes_definir_vidro(0);
  assert(!ajustes_vidro() && ajustes_vidro_fosco() && ajustes_vidro_qualidade() == 2);
  roda(30, &t);
  printf("Vidro desligado: cap<=%d pas<=%d copia=%.2f fonte=%d chapado=%d\n", cap, pas, copiatot, fonte, chapado);
  // <= 1.0 screen: the toggle frame closes last frame's regions once.
  confere(copiatot <= 1.0 && fonte == 0, "glass off: no full-screen blur on a changing backdrop");
  ajustes_definir_vidro(1);
  roda(30, &t);
  confere(cap == 1 && fonte > 0, "glass back on: frost captures the backdrop again");

  unlink(cam);
  { char tt[700]; snprintf(tt, sizeof tt, "%s/ajustes.tmp", dir); unlink(tt); }
  printf("vidro_qualidade: %s\n", falhas ? "FALHA" : "ok");
  return falhas ? 1 : 0;
}
