// FACETAS: a geometria e as curvas do menu de segurar OK (proposta B, aprovada
// pelo dono em 24/11/2026, prancha "B · Facetas").
//
// As fichas (uma por acao, de canto cortado) ficam numa ORBITA ao redor do
// cartaz focado. Aqui mora SO a conta — nenhum GL, nenhuma textura — para o
// posicionamento poder ser testado com o comportamento de verdade: o menu nao
// usa coordenada de exemplo, usa o retangulo REAL do item, o espaco livre, os
// limites da tela, a quantidade de acoes e o tipo de fileira.
//
// Tres formas, escolhidas pelo formato do item e pelo espaco:
//   FACETA_V    arco VERTICAL ao lado do item (cartaz mais alto que largo, com
//               espaco no lado). O rotulo do foco se abre PARA FORA da ficha.
//   FACETA_H    arco HORIZONTAL sob (ou sobre) o item (card deitado, colecao,
//               ou quando nenhum lado tem espaco). A legenda fica no eixo.
//   FACETA_DOCK as fichas DENTRO da base do cartaz, onde so ha arte (o Palco,
//               que tem a ficha de informacoes de um lado e vizinhos do outro).
#ifndef NV_FACETAS_H
#define NV_FACETAS_H
#include "gfx.h"

#define FACETA_MAX    9
#define FACETA_T     76.0f   // lado da ficha em repouso
#define FACETA_PASSO 104.0f  // centro a centro, no eixo da orbita
#define FACETA_CORTE 16.0f   // o canto cortado (superior esquerdo e inferior direito)
#define FACETA_AO_LADO 36.0f // do item ate a ficha
#define FACETA_BORDA_X 40.0f // margem horizontal minima (a Home usa 80; a ficha pode ir ate a metade)
#define FACETA_BORDA_Y 60.0f // margem vertical minima
#define FACETA_ESCALA_CARTAZ 1.06f // o cartaz cresce so um pouco ao abrir (pedido do dono: sutil)
#define FACETA_VEU     0.22f // escurecimento leve da Home atras do menu

typedef enum { FACETA_V = 0, FACETA_H = 1, FACETA_DOCK = 2 } FacetaModo;

typedef struct {
  FacetaModo modo;
  int lado;       // V: +1 direita, -1 esquerda. H: +1 abaixo do item, -1 acima. DOCK: +1.
  int n;
  float x[FACETA_MAX], y[FACETA_MAX];   // canto superior esquerdo de cada ficha, em repouso
  float ancX, ancY;                     // de onde as fichas nascem: o ponto da borda do item
  float legX, legY;                     // H/DOCK: centro x e topo da legenda
  int   legAcima;                       // a legenda fica ACIMA das fichas
} FacetaGeo;

// `item` e o retangulo FINAL do cartaz (ja crescido por FACETA_ESCALA_CARTAZ).
// `larg[i]` e a largura da ficha i com o rotulo aberto (>= FACETA_T), so para o
// arco vertical medir o espaco que o rotulo pede. `palco` = o item e o foco do
// Palco (reserva `abaixo` px sob o cartaz para o nome do titulo).
void faceta_geometria(GfxRect item, int n, const float *larg, int palco, float abaixo,
                      float telaW, float telaH, FacetaGeo *g);

// O retangulo (w x h) do painel de CONFIRMACAO, ao lado do item (ou sob/sobre
// ele quando nao ha lado livre). `lado` devolve +1 direita, -1 esquerda, 0 sob,
// 2 sobre. Mesma regra de espaco do arco.
GfxRect faceta_confirmacao(GfxRect item, float w, float h, int palco, float abaixo,
                           float telaW, float telaH, int *lado);

// ---- Curvas (todas puras; `p` e o relogio unico do menu, 0 = fechado, 1 = aberto)
// O MESMO relogio abre e fecha: fechar e andar para tras no mesmo caminho,
// entao nao ha um "desfazer" separado e nada aparece ou some de repente.
float faceta_p_cartaz(float p);     // 1 .. FACETA_ESCALA_CARTAZ (fator de escala do cartaz)
float faceta_p_veu(float p);        // 0 .. FACETA_VEU (alfa do escurecimento)
float faceta_p_ficha(float p, int i, int n);   // 0..1 da ficha i: sai da borda do item e se encaixa
float faceta_p_rotulo(float p);     // 0..1: o rotulo/legenda so abre depois que as fichas pousam
float faceta_p_orbita(float p);     // 0..1: quanto do tracejado da orbita ja foi traçado

// Os pontos do tracejado da orbita (anchor -> fichas), um a cada ~9 px, ate a
// fracao `prog` do caminho. Devolve quantos escreveu em out (ate `max`).
typedef struct { float x, y; } FacetaPonto;
int faceta_orbita(const FacetaGeo *g, float prog, FacetaPonto *out, int max);

#endif
