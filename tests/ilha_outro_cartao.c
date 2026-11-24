// World 23/11 (dono: "e o 'rodando a cada 6 s'? isso tem que sair"): a pilula do
// relogio NAO alterna mais entre a sessao interrompida e a estreia. O primeiro
// cartao fica; o outro mora numa linha selecionavel no modal (seta para baixo,
// OK troca). Teste de COMPORTAMENTO: abre o modal em instantes diferentes e
// confere QUAL cartao ele pegou, navega ate a linha e confere o pedido.
#include "ajustes.h"
#include "ilha.h"
#include "layout.h"
#include "text.h"
#include "dados.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nvtmp.h"

static char dir[NVT_MAX];
static Uint32 t = 1000;
static void quadros(int n) {
  for (int i = 0; i < n; i++, t += 16) { ilha_relogio_visivel(1); ilha_posicionar(1); ilha_desenhar(t); }
}
static void passa(Uint32 ms) { quadros((int)(ms / 16u)); }
static void tecla(SDL_Keycode k) {
  SDL_Event e; memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  ilha_evento(&e);
  e.type = SDL_KEYUP; ilha_evento(&e);
  quadros(6);
}
static void cartao(IlhaCartao *c, const char *titulo, int t_, int e_, float prog) {
  memset(c, 0, sizeof *c);
  snprintf(c->chave, sizeof c->chave, "k:%s", titulo);
  snprintf(c->imdb, sizeof c->imdb, "tt%d", (int)strlen(titulo));
  snprintf(c->titulo, sizeof c->titulo, "%s", titulo);
  c->serie = 1; c->t = t_; c->e = e_; c->progresso = prog; c->restanteMin = 30;
  snprintf(c->avisoId, sizeof c->avisoId, "a:%s", titulo);
  snprintf(c->quando, sizeof c->quando, "hoje");
}
static int altura(void) { float x, y, w, h = 0; assert(ilha_rect(&x, &y, &w, &h)); return (int)h; }
// Abre o modal e devolve o cartao em que o OK no 1o botao pede "tocar".
static int abreEPede(void) {
  IlhaCartao c; int q = -1;
  assert(ilha_modal_abrir());
  passa(900);
  tecla(SDLK_RETURN);
  assert(ilha_pediu(&c, &q) == ILHA_PEDIU_TOCAR);
  passa(900);
  return q;
}

int main(void) {
  IlhaCartao vivo, est;
  nvt_set(dir, sizeof dir, "/nuvio-ilha-outro-XXXXXX"); assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  assert(SDL_GL_CreateContext(SDL_CreateWindow("t", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN)));
  assert(txt_iniciar("deploy/app", 1));
  cartao(&vivo, "Fallout", 1, 3, 0.42f);
  cartao(&est, "Widow's Bay", 2, 5, -1.0f);

  // 1) SEM ROTACAO: em 0 s, 7 s (a vez da estreia no relogio antigo), 13 s e
  //    20 s o modal sempre pega a SESSAO (o primeiro cartao).
  ilha_cartao(ILHA_VIVO, &vivo);
  ilha_cartao(ILHA_ESTREIA, &est);
  passa(800);
  assert(abreEPede() == ILHA_VIVO);
  passa(7000);
  assert(abreEPede() == ILHA_VIVO);
  passa(6000);
  assert(abreEPede() == ILHA_VIVO);
  passa(7000);
  assert(abreEPede() == ILHA_VIVO);

  // 2) A LINHA DO OUTRO CARTAO: abre, seta para baixo e OK trocam para a
  //    estreia; o modal fica mais alto que o de um cartao so.
  assert(ilha_modal_abrir()); passa(900);
  int alto2 = altura();
  tecla(SDLK_DOWN);
  tecla(SDLK_RETURN);          // troca o cartao (nao e o "Assistir")
  assert(ilha_modal_aberto());
  assert(ilha_pediu(NULL, NULL) == 0);
  passa(300);
  tecla(SDLK_RETURN);          // agora e o "Assistir" da estreia
  { IlhaCartao c; int q = -1;
    assert(ilha_pediu(&c, &q) == ILHA_PEDIU_TOCAR);
    assert(q == ILHA_ESTREIA && strcmp(c.titulo, "Widow's Bay") == 0); }
  passa(900);

  // 3) Para cima volta aos botoes: DOWN, UP, OK pede a sessao.
  assert(ilha_modal_abrir()); passa(900);
  tecla(SDLK_DOWN); tecla(SDLK_UP); tecla(SDLK_RETURN);
  { IlhaCartao c; int q = -1;
    assert(ilha_pediu(&c, &q) == ILHA_PEDIU_TOCAR && q == ILHA_VIVO); }
  passa(900);

  // 4) De lado nao ha o que fazer na linha: DOWN, RIGHT nao abre Salvos nem sai.
  assert(ilha_modal_abrir()); passa(900);
  tecla(SDLK_DOWN); tecla(SDLK_RIGHT); tecla(SDLK_LEFT);
  assert(ilha_modal_aberto() && ilha_pediu(NULL, NULL) == 0);
  ilha_modal_fechar(1); passa(900);

  // 5) Um cartao so: sem linha, seta para baixo nao muda nada e o modal e mais baixo.
  ilha_cartao(ILHA_ESTREIA, NULL);
  passa(800);
  assert(ilha_modal_abrir()); passa(900);
  int alto1 = altura();
  assert(alto2 > alto1 + 60);                  // a linha existe com dois e some com um
  tecla(SDLK_DOWN); tecla(SDLK_RETURN);
  { IlhaCartao c; int q = -1;
    assert(ilha_pediu(&c, &q) == ILHA_PEDIU_TOCAR && q == ILHA_VIVO); }   // OK foi no "Retomar"

  puts("PASS: relogio sem rotacao; o outro cartao e uma linha selecionavel no modal");
  return 0;
}
