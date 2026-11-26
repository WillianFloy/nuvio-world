// A HOME CONGELADA ATRAS DO MENU R8: o quadro inteiro do fundo (decidir,
// pintar a copia, conferir se saiu preta, copiar recuada e escurecida).
//
// Mora aqui, e nao em app.c, para o app e o harness do menu
// (tests/r8_menu_quadros_shot.c) passarem pelo MESMO codigo. Antes o harness
// tinha a propria copia desta logica e divergia do app: o "fundo velho" de
// 26/11 passou nos testes porque o harness zerava um estado que o app nao
// zerava. Quem chama so fornece o que esta atras do menu (`atras`).
#ifndef NV_R8FUNDO_H
#define NV_R8FUNDO_H

#include "fundoparado.h"
#include <SDL2/SDL_stdinc.h>
#include <stddef.h>

typedef struct {
  FundoParado fp;
  unsigned verAnt, catAnt, sesAnt;
} R8Fundo;

// Um quadro do fundo com o menu R8 na tela (ctx_r8_fundo devolveu 1).
// `parar`: a copia e possivel (FBO pronto, sem video no ar).
// `atras(ctx)`: desenha o que fica atras do menu, SEM o menu.
// `relogio`: SDL_GetTicks no app (o harness passa o relogio falso dele); lido
// nos mesmos pontos de antes (antes de decidir, depois de pintar, ao copiar).
// `rastro`: o texto do [qd-lento] (" fundo=P ...", " fundo=C", " fundo=D").
// Devolve 1 se desenhou a copia (o chamador desenha o menu por cima) e 0 se
// a Home deve ser desenhada viva pelo chamador (copia preta ou impossivel).
int r8fundo_quadro(R8Fundo *f, int parar, float esc, float ox, float oy, float veu,
                   unsigned versao, unsigned sessao, Uint32 (*relogio)(void),
                   void (*atras)(void *), void *ctx, char *rastro, size_t nRastro);

#endif
