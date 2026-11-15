// RITMO DA NAVEGACAO POR SETAS (app todo, menos o player).
//
// O controle de TV manda a seta segurada como uma sequencia de KEYDOWNs, cada um
// com repeat=0, a ~40-100 ms (ver app.c, "o firmware da TV manda a tecla
// segurada..."). Cada um andava um cartaz: o alvo da rolagem corria na frente
// da animacao, o foco pulava cartazes e a tela carregava arte de tudo o que
// passava. Com "Navegação geral rápida" DESLIGADA (o padrao) o ritmo e este:
//
//  - um toque anda UM item, na hora, na velocidade normal;
//  - segurando, o primeiro passo extra so vem depois de NAV_ESPERA_MS e os
//    seguintes a cada NAV_PASSO_MS, um item de cada vez (~1,5x a velocidade de
//    um toque). Eventos entre dois passos sao DESCARTADOS, nunca enfileirados:
//    soltar a tecla para a navegacao na hora, sem fila para consumir;
//  - mudar de seta, soltar a tecla ou uma pausa maior que NAV_PAUSA_MS comecam
//    um toque novo.
// Ligada, nada disto vale: o comportamento rapido de antes.
//
// Funcao pura (sem SDL, sem estado global): o teste usa um fluxo simulado.
#ifndef NUVIO_NAVRITMO_H
#define NUVIO_NAVRITMO_H

#define NAV_ESPERA_MS   350u   // segurando: antes do 1o passo extra
#define NAV_PASSO_MS    320u   // segurando: entre passos (~3 por segundo)
#define NAV_FOLGA_MS     30u   // tolera a repeticao do controle cair um pouco antes
#define NAV_PAUSA_MS    220u   // sem evento ha mais que isto: toque novo
#define NAV_TOQUE_MIN_MS 45u   // dois toques mais juntos que isto: o segundo e trepidacao

typedef struct {
  int tecla;                 // a seta em curso (0 = nenhuma)
  unsigned inicio, ultimo, entregue;
} NavRitmo;

// A tecla foi solta: o proximo KEYDOWN e um toque novo.
static inline void navritmo_soltou(NavRitmo *n, int tecla) {
  if (n->tecla == tecla) n->tecla = 0;
}
// KEYDOWN de seta: 1 = entrega a tela, 0 = descarta.
static inline int navritmo_baixou(NavRitmo *n, unsigned agora, int tecla) {
  int novo = n->tecla != tecla || agora - n->ultimo > NAV_PAUSA_MS;
  n->ultimo = agora;
  if (novo) {
    // Toque novo; so a trepidacao (dois eventos quase juntos) e barrada.
    if (n->tecla == 0 && n->entregue && agora - n->entregue < NAV_TOQUE_MIN_MS) return 0;
    n->tecla = tecla; n->inicio = agora; n->entregue = agora;
    return 1;
  }
  if (agora - n->inicio >= NAV_ESPERA_MS && agora - n->entregue + NAV_FOLGA_MS >= NAV_PASSO_MS) {
    n->entregue = agora;
    return 1;
  }
  return 0;
}

#endif
