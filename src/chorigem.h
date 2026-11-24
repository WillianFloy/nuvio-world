// DE ONDE OS PAINEIS DO CH NASCEM (World, 24/11/2026, pedido do dono).
//
//   Menu do topo aparecendo  -> nasce dele (a Busca, da lupa; os outros, da barra);
//   senao, relogio aparecendo -> nasce da pilula do relogio;
//   senao                     -> nasce do centro do alto da tela.
//
// Vale para a Busca (CH-), os Salvos (CH+ toque) e a Central (CH+ segurado),
// com a barra lateral ou o menu do topo, fixo ou nao (o nao fixo so aparece no
// destaque). Ajustes > Atalhos CH+ e CH- > "De onde os painéis nascem" fixa o
// relogio ou o centro; "Busca a partir do menu do topo" desliga a lupa.
// A Central, com o topo e o relogio na tela, nasce da pilula (que mora na
// ponta da barra) e nao da barra inteira: ver chorigem().
#ifndef NV_CHORIGEM_H
#define NV_CHORIGEM_H
#include "gfx.h"

typedef enum { CHO_TOPO = 0, CHO_RELOGIO = 1, CHO_CENTRO = 2 } ChOrigemTipo;
typedef enum { CHO_P_BUSCA = 0, CHO_P_SALVOS, CHO_P_CENTRAL } ChPainel;

// A decisao, pura (tests/chorigem.c). `ajuste`: 0 Automatica, 1 Relogio, 2 Centro.
// `buscaDaBarra`: a Busca pode nascer da lupa (so vale para CHO_P_BUSCA).
ChOrigemTipo chorigem_decidir(ChPainel p, int ajuste, int buscaDaBarra, int topoVisivel, int relogioVisivel);
// O retangulo de origem neste quadro, em px REAIS (quem desenha numa camada
// ampliada divide pela escala dela), e o tipo.
ChOrigemTipo chorigem(ChPainel p, GfxRect *r);
// O retangulo do centro (px reais) para uma tela de largura w.
GfxRect chorigem_centro(float w);
// Testes: finge (1) ou nega (0) estar na Home; -1 volta ao app.
void chorigem_teste_na_home(int sim);
#endif
