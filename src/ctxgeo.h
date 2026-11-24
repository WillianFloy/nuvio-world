// GEOMETRIA DO MENU DE SEGURAR OK (World, 24/11/2026): as duas formas
// aprovadas pelo dono depois que as Facetas foram reprovadas na TCL.
//
//   FOLHA (padrao): um painel de altura inteira no lado OPOSTO ao item, com a
//   arte do proprio titulo no topo (ela voa do cartaz ate la), as notas, a
//   sinopse e as acoes. Mesmo desenho para pôster, card largo, Palco, destaque
//   e Collection.
//
//   DESDOBRAR: o proprio cartaz se abre num painel ao lado dele, e o resto da
//   fileira daquele lado ABRE ESPACO (desliza a largura do painel) em vez de
//   ficar coberto. No Palco o painel ocupa o lugar da ficha (nada desliza).
//
// Tudo em px da tela virtual (1920 x 1080), puro e testado em tests/ctxgeo.c.
#ifndef NV_CTXGEO_H
#define NV_CTXGEO_H
#include "gfx.h"

#define CTXG_MARGEM      48.0f
#define CTXG_FOLHA_W    600.0f
#define CTXG_FOLHA_ARTE 338.0f   // 16:9 na largura da folha
#define CTXG_DESD_W     520.0f
#define CTXG_LINHA_H     58.0f   // linha de acao
#define CTXG_LINHA_PASSO 66.0f

typedef struct {
  GfxRect folha;   // o painel inteiro
  GfxRect arte;    // a faixa de arte no topo
  int lado;        // +1 folha a direita, -1 a esquerda
} CtxFolhaGeo;
// `item` pode ser NULL (sem item de origem: folha a direita).
void ctxgeo_folha(const GfxRect *item, float telaW, float telaH, CtxFolhaGeo *g);

typedef struct {
  GfxRect cont;     // o cartaz + o painel (a superficie que se desdobra), aberto
  GfxRect painel;   // so o painel (onde vao titulo e acoes)
  int lado;         // +1 abre para a direita, -1 para a esquerda
  float empurra;    // quanto a fileira desliza daquele lado (0 = nada desliza)
  GfxRect faixa;    // a faixa da fileira que desliza (x de onde comeca ate a borda)
} CtxDesdGeo;
// `alturaConteudo`: o que o painel precisa (titulo + acoes); `palco` 1 = o painel
// toma o lugar da ficha do Palco (largura `fichaW` a `fichaVao` do cartaz) e nada
// desliza.
void ctxgeo_desdobrar(GfxRect item, float alturaConteudo, int palco, float fichaVao, float fichaW,
                      float telaW, float telaH, CtxDesdGeo *g);

// O relogio do menu (0..1) nas fases de cada forma: curvas puras.
float ctxgeo_dec(float k);                    // desacelera (entrada)
float ctxgeo_fase(float t, float a, float b); // 0..1 entre a e b, desacelerando
#endif
