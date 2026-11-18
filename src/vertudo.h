// TELA "VER TUDO": um catalogo inteiro em grade, aberto pelo card no fim da
// fileira da home.
//
// Existe porque a fileira mostra 12 itens e o catalogo tem centenas — no web
// isso e o `catalogSeeAllScreen`, aberto pelo card `openCatalogSeeAll`. Sem ela
// o resto de cada catalogo era inalcancavel: nao havia NENHUM caminho no app
// para ver o item 13 de uma fileira.
//
// A grade e de 5 colunas em 1920, medida do web (.seeall-card 248px de largura,
// e o corpo reserva 368px do painel lateral). O painel de detalhe a direita que
// o web tem NAO entra nesta versao — ele repete o que a tela de titulo ja
// mostra, e a grade sozinha ja resolve o que faltava.
#ifndef NV_VERTUDO_H
#define NV_VERTUDO_H
#include <SDL2/SDL.h>
#include "colecoes.h"
#include "gfx.h"
// IDs persistidos por colecao e compartilhados pelo ajuste global. Acrescentar
// novas exibicoes somente no fim; zero e automatico no global e herda-o na pasta.
typedef enum {
  VT_LAY_AUTO = 0, VT_LAY_GRADE, VT_LAY_LINHA, VT_LAY_RANKING,
  VT_LAY_SAGA, VT_LAY_DECADAS, VT_LAY_MURAL, VT_LAY_VITRINE,
  VT_LAY_FLUXO,                   // no fim: o numero gravado das outras nao muda
  VT_LAY_N
} VtLayout;
void vertudo_colecao(const ColFolder *folder);
// 2.0.44: abre a Collection como VOO DO CARTAO: `origem` e o retangulo real do cartao da pasta na Home
// (home_ancora_foco) e `raioPx` o raio dos cantos dele. Com o ajuste desligado / animacoes reduzidas /
// efeitos minimos / telefone / Vitrine ja aberta abre como vertudo_colecao.
void vertudo_colecao_voo(const ColFolder *folder, GfxRect origem, float raioPx);
// 1 quando a Collection cobre a tela (a Home por baixo nao precisa ser desenhada). 0 durante o voo do cartao.
int  vertudo_cobre_tela(void);
// Do voo (testes): 1 enquanto o cartao esta em cena; p = progresso do crescimento 0..1; rect = onde ele esta.
int  vertudo_voo_ativo(float *p, GfxRect *rect);

// Abre com o catalogo de uma fileira da home.
void vertudo_abrir(const char *base, const char *tipo, const char *catId,
                   const char *titulo);
int  vertudo_aberta(void);
// 2.0.45: TRANSFORMACAO HOME -> COLLECTION (transicao.h). A Home pergunta quanto recuar (0..1) e quanto do
// texto do hero ainda aparece (1..0); 0 / 1 fora da transicao.
float vertudo_home_recuo(float *textoVisivel);
int   vertudo_home_oculta_origem(void);   // 2.0.45: o cartao de origem virou o cartao da transformacao (a Home nao o desenha)
// 2.0.45 (D2): pre-carga por permanencia: listas do colcache (sem rede) e cartazes visiveis com o cache ocioso.
void vertudo_prever(const ColFolder *folder);
int  vertudo_prever_pendentes(void);   // testes: cartazes ainda na fila da pre-carga
// 2.0.45 (testes): fase do relogio (transicao.h: TR_*), p, r e o ms em que a revelacao comecou (-1 = ainda nao).
int  vertudo_tr_estado(float *p, float *r, float *msRevela, float *ms);
float vertudo_entrada_ms(void);   // 2.0.43: ms desde a abertura da Collection (>= 570: assentado)
const char *vertudo_perf_tag(void);
// Perf-only navigation: first collection, then cycle without saving preferences.
void vertudo_perf_cycle(void);
// Some sem animar (o player saiu para a home: ilha_minimizar).
void vertudo_fechar_seco(void);
void vertudo_evento(const SDL_Event *e);
void vertudo_atualizar(float dt, Uint32 agora);
void vertudo_desenhar(Uint32 agora);
// Indice no catalogo global do titulo que o dono abriu, ou -1. Consumido uma
// vez: o roteador chama, abre o detalhe e a tela se fecha.
int  vertudo_pediu_abrir(void);
// 1 uma vez: ESQUERDA na coluna 0 pediu a barra lateral (abre por cima).
int  vertudo_pediu_menu(void);
int  vertudo_pediu_relogio(void);
// Collection aberta na TV: o relogio e o botao "Exibição" saem da tela e as
// opcoes vao para a primeira aba do painel do CH+ (salvospainel.c).
int  vertudo_em_colecao(void);
int  vertudo_opcoes_n(void);
const char *vertudo_opcao_rotulo(int i, int *marcada, const char **icone);
int  vertudo_opcao_ok(int i);   // 1 = fechar o painel
// Posicao do cartao em foco na grade (testes: o foco volta ao mesmo cartao
// depois do menu do cartaz).
int  vertudo_foco(void);
#endif
