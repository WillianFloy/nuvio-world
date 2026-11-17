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
  unsigned alvo;             // navritmo_baixou_vitrine: quando vence o proximo passo
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

// SEGURAR NA VITRINE DAS COLLECTIONS (2.0.40). O movimento passou a levar ~350 ms
// (NV_MOTION_MS); segurando, um passo a cada NAV_PASSO_VITRINE_MS = 350 / 1,4:
// ~1,4x a velocidade de toques (faixa aprovada: 1,25x a 1,5x). Ainda um item de
// cada vez e sem fila. Diferente de navritmo_baixou, a cadencia segue uma AGENDA
// (cada passo vence PASSO depois do ANTERIOR VENCIMENTO, nao da entrega): com o
// controle repetindo a 33, 50 ou 100 ms o intervalo medio e o mesmo, em vez de
// arredondar para cima ao multiplo da repeticao (de 250 a 300 ms antes).
#define NAV_PASSO_VITRINE_MS 250u
static inline int navritmo_baixou_vitrine(NavRitmo *n, unsigned agora, int tecla) {
  unsigned espaco = agora - n->ultimo;   // entre este evento e o anterior
  int novo = n->tecla != tecla || espaco > NAV_PAUSA_MS;
  n->ultimo = agora;
  if (novo) {
    if (n->tecla == 0 && n->entregue && agora - n->entregue < NAV_TOQUE_MIN_MS) return 0;
    n->tecla = tecla; n->inicio = agora; n->entregue = agora;
    n->alvo = agora + NAV_ESPERA_MS;
    return 1;
  }
  // Vence quando faltar menos que meia repeticao: o evento mais proximo da agenda.
  if ((int)(agora + espaco / 2 - n->alvo) < 0) return 0;
  n->entregue = agora;
  n->alvo += NAV_PASSO_VITRINE_MS;
  // Atrasou demais (quadro longo, TV lenta): recomeca a agenda daqui, sem rajada.
  if ((int)(n->alvo - agora) < (int)(NAV_PASSO_VITRINE_MS / 2)) n->alvo = agora + NAV_PASSO_VITRINE_MS;
  return 1;
}

#endif
