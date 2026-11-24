#ifndef NV_SEGURAR_H
#define NV_SEGURAR_H
#include <SDL2/SDL.h>

// SEGURAR OK: a tela arma com o tempo do KEYDOWN e pergunta, a cada quadro,
// se ja passou NV_HOLD_MS sem KEYUP. Antes cada tela fazia a conta com o
// relogio da hora da pergunta (`SDL_GetTicks() - desde`), e um quadro que
// demorou (decodificacao no fio principal, subida de textura, .NET ocupado
// entregando a tecla no .tpk) fazia o limiar vencer ANTES de o KEYUP que ja
// estava na fila ser lido: um toque curto virava menu do cartaz (#387).
//
// Agora o segurar e medido no instante da ULTIMA LEITURA DE EVENTOS do laco
// (segurar_quadro, chamado por app_atualizar com o `agora` que main.c tira logo
// depois do SDL_PollEvent). Tudo o que chegou ate esse instante ja foi lido:
// se o dedo tivesse saido, o KEYUP ja teria desarmado. Quadros lentos seguidos
// nao impedem o segurar de verdade, so o atrasam ate a proxima leitura.
//
// `agora` e o relogio da tela (usado so quando ninguem chamou segurar_quadro,
// como nos testes que chamam a tela direto); `desde` o tempo do KEYDOWN
// (0 = nao armado). `tela` so serve ao log, que sai uma vez por pressao.
void segurar_quadro(Uint32 agora);

// A BARRINHA "SEGURE PARA OPCOES" SO EXISTE NO SEGURAR, NAO NO TOQUE (dono,
// 24/11: "quando so da um click ela aparece como se tivesse carregando, e bem
// rapido, mas nao deveria"). Um toque dura ate ~150 ms entre KEYDOWN e KEYUP;
// a barra so nasce depois disso. Detalhe e lista ja contavam esses 150 ms por
// conta propria; Home e Biblioteca desenhavam desde o primeiro quadro.
#define NV_HOLD_TOQUE_MS 150u
static inline int segurar_barra_visivel(Uint32 desdeKeydownMs) {
  return desdeKeydownMs >= NV_HOLD_TOQUE_MS;
}
int segurar_venceu(Uint32 agora, Uint32 desde, const char *tela);

#endif
