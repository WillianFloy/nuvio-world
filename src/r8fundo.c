// Ver r8fundo.h.
#include "r8fundo.h"
#include "catalogo.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

unsigned r8fundo_pinturas, r8fundo_pinturas_complemento;

int r8fundo_quadro(R8Fundo *f, int parar, float esc, float ox, float oy, float veu,
                   unsigned versao, unsigned sessao, Uint32 (*relogio)(void),
                   void (*atras)(void *), void *ctx, char *rastro, size_t nRastro) {
  unsigned rev = cat_revisao() + versao * 7919u;
  int assentada = home_assentada(), assentar = 0, pintou = 0;
  R8fAcao acao = r8fundo_decidir(f, parar, sessao, rev, assentada, tex_upl_total, relogio(), &assentar);
  if (assentar) home_assentar();
  if (acao == R8F_PINTAR) {
    unsigned faltas0 = tex_n_falta;
    int txt0 = txt_pendentes;
    int mx = 0, preta, fw = 0, fh = 0;
    Uint64 c0 = SDL_GetPerformanceCounter(), c1;
    double ms, lerMs;
    const char *motivo = f->motivo ? f->motivo : "?";
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
    printf("[ctx-r8] copia da Home pintada: motivo=%s%s %.1f ms (leitura %.1f ms) fbo=%dx%d\n",
           motivo, assentada ? "" : " (Home mexendo)", ms, lerMs, fw, fh);
    if (!strcmp(motivo, "arte") || !strcmp(motivo, "texto")) r8fundo_pinturas_complemento++;
    r8fundo_pintou(f, tex_n_falta != faltas0, txt_pendentes != txt0, preta, assentada, tex_upl_total);
    r8fundo_pinturas++;
    pintou = 1;
    if (preta) {
      printf("[ctx-r8] copia parada saiu PRETA (max=%d): desenhando a home direto\n", mx);
      fflush(stdout);
      acao = R8F_DIRETO;
    } else acao = R8F_COPIAR;
  }
  if (acao == R8F_COPIAR) {
    if (!pintou && rastro) snprintf(rastro, nRastro, " fundo=C");
    home_fileiras_vistas(relogio());
    gfx_sem_recorte();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_snap_desenhar_xf(esc, ox, oy, 0.0f, 0.0f, veu);
    return pintou ? R8F_Q_PINTOU : R8F_Q_COPIOU;
  }
  if (rastro) snprintf(rastro, nRastro, acao == R8F_ESPERA ? " fundo=E" : " fundo=D");
  return acao == R8F_ESPERA ? R8F_Q_ESPERA : R8F_Q_SEM;
}
