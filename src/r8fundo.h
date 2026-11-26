// A HOME CONGELADA ATRAS DO MENU R8: o quadro inteiro do fundo (decidir,
// pintar a copia, conferir se saiu preta, copiar recuada e escurecida).
//
// Mora aqui, e nao em app.c, para o app e o harness do menu
// (tests/r8_menu_quadros_shot.c) passarem pelo MESMO codigo. Antes o harness
// tinha a propria copia desta logica e divergia do app: o "fundo velho" de
// 26/11 passou nos testes porque o harness zerava um estado que o app nao
// zerava. Quem chama so fornece o que esta atras do menu (`atras`).
//
// QUANDO A COPIA E REFEITA (26/11, noite; TCL: cada repintura custa 45-60 ms
// de GPU, que caem no quadro SEGUINTE, e a copia pegava o destaque no meio da
// troca). Antes: por relogio (0,4/1,5/4 s depois de cada abertura E de cada
// troca de estilo: 4 repinturas por seta) e por arte faltando (a cada 150 ms
// por ate 12 s), sem saber se algo tinha mudado nem se a Home tinha parado.
// Agora, so quando algo mudou de fato, e sempre do estado FINAL da Home:
//   - abertura, estilo da fileira ou catalogo mudou: pede home_assentar (o
//     passo seguinte da Home vai direto ao alvo) e pinta quando ela assentar;
//     na abertura a Home viva fica na tela enquanto isso (o menu ainda esta em
//     0), nas outras a copia anterior;
//   - a Home se mexeu depois da pintura (ex.: o destaque trocou quando a arte
//     chegou): de novo, quando ela assentar;
//   - faltava arte na copia: quando uma textura NOVA chegou e o esvanecimento
//     dela terminou (R8F_QUIETO_MS sem chegar outra), no maximo R8F_ARTE_MAX
//     vezes. Arte que nunca chega nao gera repintura nenhuma;
//   - faltava TEXTO na copia (txt_pendentes: o orcamento de rasterizacao do
//     quadro acabou; ex.: os numerais do Ranking numerado no estilo trocado
//     ao vivo): no quadro seguinte, no maximo R8F_TEXTO_MAX vezes. Com a Home
//     congelada ninguem mais a desenha, entao so a repintura rasteriza o resto.
// TETO: na abertura a pintura sai no maximo R8F_ESPERA_QD quadros depois do
// pedido (o menu nao espera um destaque que aguarda arte); nos outros casos, a
// Home que nao assenta em R8F_TETO_MS e pintada assim mesmo (motivo "teto").
#ifndef NV_R8FUNDO_H
#define NV_R8FUNDO_H

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_stdinc.h>
#include <stddef.h>

#define R8F_QUIETO_MS 270u   // a arte que chega entra em 220 ms (NV_REVELA_MS) + folga
#define R8F_TETO_MS   1500u
#define R8F_ARTE_MAX  6
#define R8F_ESPERA_QD 2      // abertura: pinta no maximo 2 quadros depois do pedido
#define R8F_TEXTO_MAX 4

typedef enum { R8F_DIRETO, R8F_ESPERA, R8F_COPIAR, R8F_PINTAR } R8fAcao;

typedef struct {
  int pronto;            // ha copia valida no FBO
  int copiaPreta;        // a copia saiu preta: Home direto ate a proxima abertura
  int sujo;              // algo mudou: refazer quando a Home assentar
  int pediu;             // home_assentar ja foi pedido para esta sujeira
  int quadrosEspera;     // quadros desde o pedido (teto da abertura)
  int pintadaMexendo;    // a Home se mexeu ao pintar ou depois: refazer quando assentar
  int faltou;            // a copia atual saiu com arte faltando
  int tentArte;          // repinturas por arte nesta abertura
  int faltouTexto;       // a copia atual saiu com linhas de texto sem rasterizar
  int tentTexto;         // repinturas por texto desde a ultima mudanca
  unsigned sessao;       // a abertura dona da copia (ctx_r8_sessao)
  unsigned rev;          // revisao do que esta na copia (catalogo + estilo)
  unsigned long upl;     // texturas chegadas quando a copia foi pintada
  unsigned long uplVisto;
  Uint32 uplMudouEm;     // quando a ultima textura chegou
  Uint32 sujoDesde;
  const char *motivo;    // por que a proxima pintura acontece (log)
} R8Fundo;

// A decisao, pura (tests/r8fundo.c prova com relogio falso). `*assentar` = 1:
// quem chama pede home_assentar() (o passo seguinte da Home vai ao alvo).
static inline R8fAcao r8fundo_decidir(R8Fundo *f, int parar, unsigned sessao, unsigned rev,
                                      int assentada, unsigned long upl, Uint32 agora, int *assentar) {
  *assentar = 0;
  if (!parar) { f->pronto = 0; return R8F_DIRETO; }
  if (sessao != f->sessao) {
    f->sessao = sessao; f->pronto = f->copiaPreta = f->pintadaMexendo = 0;
    f->faltou = f->tentArte = f->faltouTexto = f->tentTexto = 0; f->rev = rev;
    f->uplVisto = upl; f->uplMudouEm = agora;
    f->sujo = 1; f->pediu = 0; f->quadrosEspera = 0; f->sujoDesde = agora;
    f->motivo = "abertura";
  }
  if (f->copiaPreta) return R8F_DIRETO;
  if (!f->pronto && !f->sujo) {   // voltou de video no ar: sem copia
    f->sujo = 1; f->pediu = 0; f->quadrosEspera = 0; f->sujoDesde = agora; f->motivo = "retomada";
  }
  if (upl != f->uplVisto) { f->uplVisto = upl; f->uplMudouEm = agora; }
  if (f->pronto) {
    // A Home se mexeu depois da pintura (o destaque trocou ao chegar a arte):
    // a copia e refeita quando ela assentar.
    if (!assentada) f->pintadaMexendo = 1;
    if (rev != f->rev) {
      if (!f->sujo) { f->sujoDesde = agora; f->pediu = 0; f->quadrosEspera = 0; }
      f->sujo = 1; f->rev = rev; f->motivo = "estilo/catalogo"; f->tentTexto = 0;
    }
    if (!f->sujo && f->pintadaMexendo && assentada) {
      f->sujo = 1; f->pediu = 1; f->sujoDesde = agora; f->motivo = "assentou";
    }
    if (!f->sujo && f->faltouTexto && f->tentTexto < R8F_TEXTO_MAX) {
      f->sujo = 1; f->pediu = 1; f->sujoDesde = agora; f->motivo = "texto"; f->tentTexto++;
    }
    if (!f->sujo && f->faltou && f->tentArte < R8F_ARTE_MAX && upl != f->upl &&
        agora - f->uplMudouEm >= R8F_QUIETO_MS) {
      f->sujo = 1; f->pediu = 1; f->sujoDesde = agora; f->motivo = "arte"; f->tentArte++;
    }
  } else f->rev = rev;
  if (!f->sujo) return R8F_COPIAR;
  if (!f->pediu) {
    // Ainda nao pediu: a Home vai ao alvo no proximo passo; este quadro mostra
    // o que ja havia (a copia anterior, ou a Home viva antes da primeira).
    f->pediu = 1; f->quadrosEspera = 0; *assentar = 1;
    return f->pronto ? R8F_COPIAR : R8F_ESPERA;
  }
  f->quadrosEspera++;
  if (assentada) return R8F_PINTAR;
  if (!f->pronto && f->quadrosEspera >= R8F_ESPERA_QD) { f->motivo = "abertura sem assentar"; return R8F_PINTAR; }
  if (agora - f->sujoDesde >= R8F_TETO_MS) { f->motivo = "teto"; return R8F_PINTAR; }
  return f->pronto ? R8F_COPIAR : R8F_ESPERA;
}

// Depois de pintar: `faltou` = arte sem chegar; `faltouTexto` = linhas de
// texto sem rasterizar; `preta` = gfx_snap_vazio; `assentada` = a Home estava
// parada ao ser pintada.
static inline void r8fundo_pintou(R8Fundo *f, int faltou, int faltouTexto, int preta, int assentada, unsigned long upl) {
  f->pronto = !preta; f->copiaPreta = preta; f->sujo = 0; f->pediu = 0; f->quadrosEspera = 0;
  f->faltou = faltou; f->faltouTexto = faltouTexto; f->pintadaMexendo = !assentada; f->upl = upl;
}

// O QUE O QUADRO FEZ (o menu usa para so comecar a abrir depois que a pintura
// passou: o custo dela na GPU cai no quadro seguinte ao da pintura).
#define R8F_Q_SEM     0   // sem copia possivel: Home viva (sem FBO, video, copia preta)
#define R8F_Q_ESPERA  1   // esperando a Home assentar: Home viva
#define R8F_Q_PINTOU  2   // pintou a copia neste quadro (e a desenhou)
#define R8F_Q_COPIOU  3   // so copiou

// Um quadro do fundo com o menu R8 na tela (ctx_r8_fundo devolveu 1).
// `parar`: a copia e possivel (FBO pronto, sem video no ar).
// `atras(ctx)`: desenha o que fica atras do menu, SEM o menu.
// `relogio`: SDL_GetTicks no app (o harness passa o relogio falso dele).
// `rastro`: o texto do [qd-lento] (" fundo=P ...", " fundo=C", " fundo=D").
// Devolve R8F_Q_*: PINTOU/COPIOU = a copia esta na tela (o chamador desenha o
// menu por cima); SEM/ESPERA = a Home deve ser desenhada viva pelo chamador.
// Pinturas feitas desde o inicio (testes e log): todas, e so as de
// complemento (arte que chegou, texto que faltava) — o resto e mudanca real.
extern unsigned r8fundo_pinturas, r8fundo_pinturas_complemento;
int r8fundo_quadro(R8Fundo *f, int parar, float esc, float ox, float oy, float veu,
                   unsigned versao, unsigned sessao, Uint32 (*relogio)(void),
                   void (*atras)(void *), void *ctx, char *rastro, size_t nRastro);

#endif
