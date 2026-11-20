// RITMO DE QUADROS por MODO DE DESEMPENHO (World, 20/11). Ver ritmo.c.
#ifndef NV_RITMO_H
#define NV_RITMO_H
#include <SDL2/SDL.h>
// Chamar uma vez por quadro, ANTES do SDL_GL_SwapWindow: aplica o modo quando
// ele muda e, no Qualidade, marca o instante de apresentacao (Android).
void ritmo_antes_swap(SDL_Window *win, int playerUi, double periodoMs);
// Depois do swap: no Qualidade segura a CPU ate a vez do proximo quadro (o
// quadro seguinte comeca ~1 periodo antes da apresentacao dele). Devolve ms
// esperados (0 nos outros modos).
unsigned ritmo_depois_swap(int playerUi, double periodoMs);
// Para o log de 3 s e os testes: o modo aplicado e como o 30 e feito.
int ritmo_modo_aplicado(void);
// O player trocou o swap interval (main.c): reaplica o modo no proximo quadro.
void ritmo_invalidar(void);
const char *ritmo_metodo(void);
// Puro (testes): o proximo alvo de apresentacao, no grid de 2 periodos a
// partir do ultimo alvo, nunca no passado (+ meio periodo de folga).
double ritmo_proximo_alvo(double ultimoAlvo, double agora, double periodo);
#endif
