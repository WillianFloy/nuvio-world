// Ver r8fundo.h. Movido de app.c (ctxFundoParado) sem mudar o comportamento.
#include "r8fundo.h"
#include "catalogo.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <stdio.h>

int r8fundo_quadro(R8Fundo *f, int parar, float esc, float ox, float oy, float veu,
                   unsigned versao, unsigned sessao, Uint32 (*relogio)(void),
                   void (*atras)(void *), void *ctx, char *rastro, size_t nRastro) {
  static const Uint32 REFAZ[] = { 400, 1500, 4000 };
  FpAcao acao;
  int pintou = 0;
  fundoparado_sessao(&f->fp, sessao);
  acao = fundoparado_passo(&f->fp, parar, cat_revisao() + versao * 7919u, relogio(), REFAZ, 3);
  if (acao == FP_PINTAR) {
    unsigned faltas0 = tex_n_falta;
    int mx = 0, preta, fw = 0, fh = 0;
    Uint64 c0 = SDL_GetPerformanceCounter(), c1;
    double ms, lerMs;
    const char *motivo = sessao != f->sesAnt ? "abertura" : versao != f->verAnt ? "estilo"
                       : cat_revisao() != f->catAnt ? "catalogo" : f->fp.faltou ? "arte" : "tempo";
    gfx_snap_comecar();
    gfx_sem_recorte();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    atras(ctx);
    gfx_sem_recorte();
    c1 = SDL_GetPerformanceCounter();
    preta = gfx_snap_vazio(&mx);
    gfx_snap_terminar();
    lerMs = (double)(SDL_GetPerformanceCounter() - c1) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    ms = (double)(SDL_GetPerformanceCounter() - c0) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    gfx_snap_tamanho(&fw, &fh);
    if (rastro) snprintf(rastro, nRastro, " fundo=P %.1fms(ler %.1f)", ms, lerMs);
    printf("[ctx-r8] copia da Home pintada: motivo=%s %.1f ms (leitura %.1f ms) fbo=%dx%d\n", motivo, ms, lerMs, fw, fh);
    f->sesAnt = sessao; f->verAnt = versao; f->catAnt = cat_revisao();
    fundoparado_pintado(&f->fp, tex_n_falta != faltas0, preta, cat_revisao() + versao * 7919u, relogio());
    pintou = 1;
    if (preta) {
      printf("[ctx-r8] copia parada saiu PRETA (max=%d): desenhando a home direto\n", mx);
      fflush(stdout);
      acao = FP_DIRETO;
    } else acao = FP_COPIAR;
  }
  if (acao == FP_COPIAR) {
    if (!pintou && rastro) snprintf(rastro, nRastro, " fundo=C");
    home_fileiras_vistas(relogio());
    gfx_sem_recorte();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_snap_desenhar_xf(esc, ox, oy, 0.0f, 0.0f, veu);
    return 1;
  }
  if (rastro) snprintf(rastro, nRastro, " fundo=D");
  return 0;
}
