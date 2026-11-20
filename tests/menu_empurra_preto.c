// ITENS A E B DA AUDITORIA DE 20/11, POR COMPORTAMENTO (pixels e geometria).
//
// A: o menu lateral aberto EMPURRA o conteudo so no layout Padrao. Na Moderna
//    (coluna World com a "Barra lateral Moderna" ligada, ou a ilha com ela
//    desligada) o conteudo fica onde esta. Ate 20/11 empurrava em todos.
// B: no tom Preto puro, a ilha da Moderna com a "Barra lateral Moderna"
//    DESLIGADA e #000000 absoluto (era vidro escuro, RGB 7-15). A coluna World
//    (Moderna ligada) fica como esta, por pedido.
//
// Uma configuracao por execucao (tests/menu_empurra_preto.sh roda a matriz):
//   NUVIO_SHOT_LAYOUT 0 Moderna / 1 Padrao, NUVIO_SHOT_MODERNA 0/1,
//   NUVIO_SHOT_RAIL 0 recolhida / 1 fixa, NUVIO_TEMA_PRETO 0/1,
//   NUVIO_SHOT_MENU_MODO 1 icones e textos / 0 so icones (a barra nao abre),
//   ESPERA_EMPURRA 0/1, ESPERA_PRETO 0 (nao e preto absoluto) / 1 (#000).
//
// O que se mede, com o menu de verdade (menu.c) aberto pela tecla e a mola
// assentada, sobre uma arte clara:
//   - menu_empurra(), o valor que app.c aplica a tela de tras;
//   - um marcador de conteudo desenhado como app.c desenha a tela de tras
//     (gfx_deslocar_base(floorf(empurra)) e o desenho), lido de volta da tela:
//     a coluna onde ele aparece tem de andar so na Padrao;
//   - o interior do menu aberto e da rail fechada, pixel a pixel (fora da
//     pilula de foco e dos rotulos).
#include "menu.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "ajustes.h"
#include "layout.h"
#include "shot_arte.h"
#include "dados.h"
#include "perfis.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas;
#define OK(c, ...) do { if (c) { printf("PASS: "); printf(__VA_ARGS__); printf("\n"); } \
                         else { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); falhas++; } } while (0)

static unsigned char *pix;
static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  menu_evento(&e);
}
// Um quadro como app.c: arte de fundo, a tela de tras empurrada (o marcador de
// conteudo: uma faixa branca de 6 px na borda do conteudo), o menu por cima.
static float empurraUsado;
static void quadro(int n) {
  while (n-- > 0) {
    float empurra;
    SDL_Delay(16);   // molas e ilha andam pelo relogio de parede, como na TV
    gfx_novo_quadro(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(6);
    menu_atualizar(1.0f / 60.0f, SDL_GetTicks());
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    gfx_tema_preto = ajustes_tema_preto();
    shot_arte_desenhar(.30f);
    empurra = menu_empurra();
    empurraUsado = empurra;
    if (empurra > 0.5f) gfx_deslocar_base(floorf(empurra));
    gfx_cor((GfxRect){ ajustes_conteudo_x() + 400.0f, 1000.0f, 6.0f, 40.0f }, 0, 0, 1, 0, 1);   // verde puro
    gfx_deslocar_base(0.0f);
    menu_desenhar(SDL_GetTicks());
  }
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
}
static const unsigned char *px(int x, int y) { return pix + ((1079 - y) * 1920 + x) * 4; }
// Coluna (em px reais) do marcador verde na linha y=1020 (o veu do menu
// aberto escurece a cena, entao o criterio e "verde dominante", nao o valor).
static int marcadorX(void) {
  int x;
  for (x = 0; x < 1920; x++) { const unsigned char *p = px(x, 1020); if (p[1] > 40 && p[1] > p[0] + 30 && p[1] > p[2] + 30) return x; }
  return -1;
}
static void salvar(const char *suf) {
  const char *d = getenv("SALVAR");
  char nome[700];
  SDL_Surface *s;
  int y;
  if (!d) return;
  snprintf(nome, sizeof nome, "%s/%s-%s.bmp", d, getenv("CONFIG"), suf);
  s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  for (y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  SDL_SaveBMP(s, nome); SDL_FreeSurface(s);
}
// Interior do menu numa faixa vertical `x` (6 px dentro da borda direita do
// painel: entre a pilula de foco, que para 12 px antes, e o fio da borda), de
// y0 a y1: quantos pixels contam e o maior/menor canal entre eles.
static void interior(int x, int y0, int y1, int *n, int *maior, int *menor) {
  int y;
  *n = 0; *maior = 0; *menor = 255;
  for (y = y0; y <= y1; y++) {
    const unsigned char *p = px(x, y);
    int m = p[0] > p[1] ? p[0] : p[1];
    if (p[2] > m) m = p[2];
    (*n)++;
    if (m > *maior) *maior = m;
    if (m < *menor) *menor = m;
  }
}

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  int esperaEmpurra = atoi(getenv("ESPERA_EMPURRA") ? getenv("ESPERA_EMPURRA") : "0");
  const char *ep = getenv("ESPERA_PRETO");
  const char *nome = getenv("CONFIG") ? getenv("CONFIG") : "?";
  char caminho[700];
  FILE *f;
  SDL_Window *w;
  int xFechado, xAberto, n, maior, menor;
  float borda;
  assert(dir && *dir);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  f = fopen(caminho, "w"); assert(f);
  fprintf(f, "idioma 0\nselected_theme 2\nhomeLayoutLocal %s\n", getenv("NUVIO_SHOT_LAYOUT"));
  shot_arte_material(f);
  fclose(f);
  snprintf(caminho, sizeof caminho, "%s/perfis.txt", dir);
  f = fopen(caminho, "w"); assert(f); fputs("1\t0\t1\t0\t#7c5cff\tHenrique\t\t\t\n", f); fclose(f);
  snprintf(caminho, sizeof caminho, "%s/perfil.txt", dir);
  f = fopen(caminho, "w"); assert(f); fputs("1\n", f); fclose(f);
  dados_iniciar("deploy/app/art");
  perfis_carregar_ativo();
  ajustes_dir(dir);
  ajustes_teste_vidro_env();   // NUVIO_SHOT_MODERNA / _RAIL

  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("menu", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar()); assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  pix = malloc(1920 * 1080 * 4); assert(pix);
  menu_iniciar();
  printf("[%s] padrao=%d moderna=%d rail-fixa=%d preto=%d\n", nome, ajustes_home_layout() == HOME_LAYOUT_PADRAO,
         ajustes_rail_moderna(), !ajustes_rail_recolhida(), ajustes_tema_preto());

  // Fechado: nenhum empurrao.
  quadro(40);
  salvar("fechado");
  xFechado = marcadorX();
  OK(menu_empurra() == 0.0f && xFechado >= 0, "[%s] fechado: empurra 0, marcador em x=%d", nome, xFechado);
  // Rail fechada (so com rail fixa): interior.
  if (!ajustes_rail_recolhida() && ep) {
    borda = menu_barra_borda();
    interior((int)borda - 6, 300, 700, &n, &maior, &menor);
    if (atoi(ep)) OK(n > 100 && maior == 0, "[%s] rail fechada no Preto: #000 (%d px, maior canal %d)", nome, n, maior);
    else OK(n > 100 && maior > 2, "[%s] rail fechada: nao e preto absoluto (maior canal %d), visual preservado", nome, maior);
  }

  // Aberto pela tecla, mola assentada.
  menu_abrir();
  tecla(SDLK_DOWN); tecla(SDLK_DOWN);
  quadro(90);
  salvar("aberto");
  xAberto = marcadorX();
  borda = menu_barra_borda();
  printf("[%s] aberto: borda=%.1f empurra=%.1f marcador %d -> %d\n", nome, borda, empurraUsado, xFechado, xAberto);
  if (esperaEmpurra) {
    OK(empurraUsado > 1.0f && xAberto - xFechado == (int)floorf(empurraUsado),
       "[%s] Padrao: o conteudo anda %.0f px com o menu aberto (marcador %d -> %d)", nome, empurraUsado, xFechado, xAberto);
    OK(xAberto > (int)borda, "[%s] Padrao: o conteudo fica a direita da barra (%d > %.0f)", nome, xAberto, borda);
  } else {
    OK(empurraUsado == 0.0f && xAberto == xFechado,
       "[%s] sem empurrao: o conteudo NAO anda (empurra=%.1f, marcador %d -> %d)", nome, empurraUsado, xFechado, xAberto);
  }
  if (ep) {
    // Faixa vertical a 24 px da borda direita do painel aberto (rotulos ficam a esquerda).
    interior((int)borda - 6, 330, 760, &n, &maior, &menor);
    if (atoi(ep)) OK(n > 150 && maior == 0, "[%s] menu aberto no Preto: interior #000000 (%d px, maior canal %d)", nome, n, maior);
    else OK(n > 150 && maior > 2, "[%s] menu aberto: nao e preto absoluto (canais %d..%d), visual preservado", nome, menor, maior);
  }
  free(pix);
  return falhas ? 1 : 0;
}
