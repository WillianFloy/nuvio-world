// FUNDO CONGELADO SOB UM PAINEL (Busca, e a mesma conta do painel de Salvos).
//
// Com um painel assentado por cima, a tela de tras e pintada UMA vez num FBO e
// os quadros seguintes so copiam. Esta decisao (copiar, repintar ou desenhar a
// home direto) e pura: nao toca em GL, para ser provada com um relogio falso
// (tests/fundoparado.c).
//
// A Busca nao tinha nada disto, so tres repinturas por tempo (0,4 / 1,5 / 4 s):
//  - o catalogo republicado DEPOIS delas (o sync mexe nele logo ao abrir) pintava
//    a copia no quadro em que a cascata de entrada das fileiras comeca, com as
//    fileiras em opacidade 0, e a copia ficava assim ate fechar a Busca;
//  - arte que ainda nao tinha chegado ficava em branco para sempre;
//  - copia que sai preta na TCL (gfx_snap_vazio) nunca era percebida.
// Os tres casos que o Salvos ja resolvia (salvospainel.c, spainel_fundo).
#ifndef NV_FUNDOPARADO_H
#define NV_FUNDOPARADO_H

#include <SDL2/SDL_stdinc.h>

#define FP_FALTA_MS   150   /* abaixo dos 200 ms que fazem um pedido de arte caducar */
#define FP_FALTA_MAX  80    /* ~12 s de tentativas por invalidacao */

typedef struct {
  int pronto;        // ha uma copia valida no FBO
  int refeitas;      // repinturas por tempo ja feitas desde a ultima republicacao
  int faltou;        // a ultima pintura saiu com arte faltando
  int tentativas;    // repinturas por arte faltando desde a ultima republicacao
  int copiaPreta;    // a copia saiu preta: a home e desenhada direto ate fechar
  unsigned rev;      // revisao do catalogo da copia pronta
  Uint32 desde;      // quando a serie de repinturas por tempo comecou
  Uint32 pintadoEm;  // quando a ultima pintura aconteceu
} FundoParado;

typedef enum { FP_DIRETO, FP_COPIAR, FP_PINTAR } FpAcao;

// `parar`: o painel esta assentado e nada atrapalha o congelamento.
// `refaz`: as esperas (ms, desde a primeira pintura) das repinturas por tempo.
static inline FpAcao fundoparado_passo(FundoParado *f, int parar, unsigned rev, Uint32 agora,
                                       const Uint32 *refaz, int nRefaz) {
  if (!parar) {
    f->pronto = f->refeitas = f->faltou = f->tentativas = f->copiaPreta = 0;
    return FP_DIRETO;
  }
  if (f->copiaPreta) return FP_DIRETO;
  if (f->pronto) {
    if (rev != f->rev) {
      // REPUBLICACAO: os tempos recomecam (a cascata de entrada das fileiras
      // novas leva ate ~1 s e as repinturas de antes ja se esgotaram).
      f->pronto = 0; f->refeitas = 0; f->tentativas = 0;
    } else if (f->faltou && f->tentativas < FP_FALTA_MAX && agora - f->pintadoEm >= FP_FALTA_MS) {
      f->pronto = 0; f->tentativas++;
    } else if (f->refeitas < nRefaz && agora - f->desde >= refaz[f->refeitas]) {
      f->pronto = 0; f->refeitas++;
    }
  }
  return f->pronto ? FP_COPIAR : FP_PINTAR;
}

// Depois de pintar a copia: `faltou` (arte sem chegar), `preta` (gfx_snap_vazio).
static inline void fundoparado_pintado(FundoParado *f, int faltou, int preta, unsigned rev, Uint32 agora) {
  f->faltou = faltou;
  f->rev = rev;
  f->pintadoEm = agora;
  if (!f->refeitas) f->desde = agora;
  if (preta) { f->copiaPreta = 1; f->pronto = 0; }
  else f->pronto = 1;
}

#endif
