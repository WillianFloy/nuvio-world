// FACETAS (menu de segurar OK, proposta B): o POSICIONAMENTO e as curvas, com o
// comportamento de verdade — nao "a funcao existe". Para dezenas de retangulos
// de item reais (cartaz, card deitado, Palco, colecao, canto da tela) e de 1 a 9
// acoes, confere que as fichas: ficam dentro da tela, nao cobrem o item (salvo o
// Palco, que as poe NA arte), nao se tocam, e que o rotulo aberto do arco
// vertical tambem cabe. Confere tambem que o relogio unico abre e fecha pelo
// mesmo caminho e que nada comeca ou termina fora de 0..1.
#include "facetas.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define W 1920.0f
#define H 1080.0f
static int falhas, total;
#define OK(c, ...) do { total++; if (!(c)) { falhas++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)
static GfxRect R(float x, float y, float w, float h) { GfxRect r = { x, y, w, h }; return r; }
static int cruza(GfxRect a, GfxRect b) { return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y); }
static GfxRect cresce(GfxRect r, float k) {
  return R(r.x - r.w * (k - 1) * 0.5f, r.y - r.h * (k - 1) * 0.5f, r.w * k, r.h * k);
}
static GfxRect ficha(const FacetaGeo *g, int i, const float *larg, int expandida) {
  float w = expandida ? larg[i] : FACETA_T;
  float x = g->x[i];
  if (expandida && g->modo == FACETA_V && g->lado < 0) x = g->x[i] + FACETA_T - w;
  return R(x, g->y[i], w, FACETA_T);
}
// Confere uma geometria inteira. `palco` relaxa so a regra "nao cobrir o item".
static void confere(const char *nome, GfxRect it, int n, int palco, float abaixo) {
  FacetaGeo g; float larg[FACETA_MAX]; int i, j;
  for (i = 0; i < FACETA_MAX; i++) larg[i] = FACETA_T + 14 + 330 + 26;
  faceta_geometria(it, n, larg, palco, abaixo, W, H, &g);
  OK(g.n == n, "%s n=%d: g.n=%d", nome, n, g.n);
  for (i = 0; i < n; i++) {
    GfxRect f = ficha(&g, i, larg, 0), fe = ficha(&g, i, larg, 1);
    OK(f.x >= FACETA_BORDA_X - 0.5f && f.x + f.w <= W - FACETA_BORDA_X + 0.5f &&
       f.y >= FACETA_BORDA_Y - 0.5f && f.y + f.h <= H - FACETA_BORDA_Y + 0.5f,
       "%s n=%d ficha %d fora da area segura (%.0f,%.0f)", nome, n, i, f.x, f.y);
    if (g.modo == FACETA_DOCK) {
      OK(f.x >= it.x && f.x + f.w <= it.x + it.w && f.y >= it.y && f.y + f.h <= it.y + it.h,
         "%s n=%d ficha %d do dock fora da arte do cartaz", nome, n, i);
    } else {
      OK(!cruza(f, it), "%s n=%d ficha %d cobre o item", nome, n, i);
      if (palco) OK(f.y >= it.y + it.h + abaixo - 0.5f || f.y + f.h <= it.y, "%s ficha %d no nome do Palco", nome, i);
    }
    if (g.modo == FACETA_V) {   // o rotulo aberto tambem cabe e nao cobre o item
      OK(fe.x >= FACETA_BORDA_X - 0.5f && fe.x + fe.w <= W - FACETA_BORDA_X + 0.5f, "%s n=%d rotulo %d fora da tela", nome, n, i);
      OK(!cruza(fe, it), "%s n=%d rotulo %d cobre o item", nome, n, i);
    }
    for (j = i + 1; j < n; j++) OK(!cruza(f, ficha(&g, j, larg, 0)), "%s n=%d fichas %d e %d se tocam", nome, n, i, j);
  }
}
int main(void) {
  struct { const char *nome; GfxRect r; int palco; float abaixo; } itens[] = {
    { "poster",   R(560, 318, 212, 322), 0, 0 }, { "poster-esq", R(96, 318, 212, 322), 0, 0 },
    { "poster-dir", R(1608, 318, 212, 322), 0, 0 }, { "borda-inf-dir", R(1608, 676, 212, 322), 0, 0 },
    { "continuar", R(540, 560, 419, 236), 0, 0 }, { "continuar-alto", R(540, 200, 419, 236), 0, 0 },
    { "colecao",  R(560, 520, 320, 180), 0, 0 }, { "palco", R(520, 300, 276, 419), 1, 70 },
    { "palco-dir", R(1100, 300, 276, 419), 1, 70 }, { "grande", R(300, 250, 568, 320), 0, 0 },
  };
  int a, n, k; FacetaGeo g;
  // 1) cada contexto, de 1 a 9 acoes, com o item JA crescido (o que o menu usa)
  for (a = 0; a < (int)(sizeof itens / sizeof itens[0]); a++)
    for (n = 1; n <= FACETA_MAX; n++)
      confere(itens[a].nome, cresce(itens[a].r, FACETA_ESCALA_CARTAZ), n, itens[a].palco, itens[a].abaixo);
  // 2) a FORMA escolhida segue o formato e o espaco (os casos das pranchas)
  { float larg[3] = { 440, 440, 440 };
    faceta_geometria(cresce(R(560, 318, 212, 322), 1.06f), 3, larg, 0, 0, W, H, &g);
    OK(g.modo == FACETA_V && g.lado == 1, "pôster no meio: arco vertical a direita (modo %d lado %d)", g.modo, g.lado);
    faceta_geometria(cresce(R(1608, 676, 212, 322), 1.06f), 3, larg, 0, 0, W, H, &g);
    OK(g.modo == FACETA_V && g.lado == -1, "pôster no canto inferior direito: arco vertical a ESQUERDA (modo %d lado %d)", g.modo, g.lado);
    faceta_geometria(cresce(R(540, 560, 419, 236), 1.06f), 4, larg, 0, 0, W, H, &g);
    OK(g.modo == FACETA_H && g.lado == 1, "card deitado de 4 acoes: arco horizontal abaixo (modo %d lado %d)", g.modo, g.lado);
    faceta_geometria(cresce(R(560, 520, 320, 180), 1.06f), 3, larg, 0, 0, W, H, &g);
    OK(g.modo == FACETA_H, "colecao: arco horizontal (modo %d)", g.modo);
    faceta_geometria(cresce(R(520, 300, 276, 419), 1.06f), 3, larg, 1, 70, W, H, &g);
    OK(g.modo == FACETA_DOCK, "Palco com 3 acoes: fichas dentro da base do cartaz (modo %d)", g.modo);
    faceta_geometria(cresce(R(520, 300, 276, 419), 1.06f), 5, larg, 1, 70, W, H, &g);
    OK(g.modo != FACETA_DOCK && g.y[0] >= 300 + 419 + 70, "Palco com 5 acoes nao cabe no dock: vai para baixo do nome do titulo (y=%.0f)", g.y[0]);
    faceta_geometria(cresce(R(540, 780, 419, 236), 1.06f), 3, larg, 0, 0, W, H, &g);
    OK(g.modo == FACETA_H && g.lado == -1, "card no pe da tela: o arco vai ACIMA (modo %d lado %d)", g.modo, g.lado);
  }
  // 3) varredura: o item em muitas posicoes e tamanhos, 1..9 acoes
  { float ws[] = { 212, 276, 419, 320 }, hs[] = { 322, 419, 236, 180 }; int iw, x, y;
    for (iw = 0; iw < 4; iw++)
      for (x = 80; x + ws[iw] <= W - 80; x += 160)
        for (y = 140; y + hs[iw] <= H - 60; y += 140)
          for (n = 1; n <= FACETA_MAX; n += 2) {
            char nome[64]; snprintf(nome, sizeof nome, "varre %.0fx%.0f@%d,%d", ws[iw], hs[iw], x, y);
            confere(nome, cresce(R((float)x, (float)y, ws[iw], hs[iw]), FACETA_ESCALA_CARTAZ), n, 0, 0);
          }
  }
  // 4) confirmacao: nunca cobre o item e sempre cabe na tela
  { float ws[] = { 212, 419, 320 }, hs[] = { 322, 236, 180 }; int iw, x, y, lado;
    for (iw = 0; iw < 3; iw++)
      for (x = 80; x + ws[iw] <= W - 80; x += 200)
        for (y = 140; y + hs[iw] <= H - 60; y += 160) {
          GfxRect it = cresce(R((float)x, (float)y, ws[iw], hs[iw]), 1.06f), c;
          c = faceta_confirmacao(it, 560, 300, 0, 0, W, H, &lado);
          OK(c.x >= 0 && c.x + c.w <= W && c.y >= 0 && c.y + c.h <= H, "confirmacao fora da tela @%d,%d", x, y);
          OK(!cruza(c, it), "confirmacao cobre o item @%d,%d (lado %d)", x, y, lado);
        }
    { GfxRect it = cresce(R(520, 300, 276, 419), 1.06f), c = faceta_confirmacao(it, 560, 150, 1, 70, W, H, &k);
      OK(c.y >= it.y + it.h + 70 && k == 0, "confirmacao do Palco fica sob o nome do titulo (y=%.0f lado=%d)", c.y, k);
      c = faceta_confirmacao(it, 560, 300, 1, 70, W, H, &k);
      OK(!cruza(c, it) && c.y >= 0 && c.y + c.h <= H && c.x + c.w <= W, "confirmacao alta do Palco: cabe e nao cobre o cartaz (lado %d)", k); }
  }
  // 5) curvas: o mesmo relogio abre e fecha; limites; ordem
  { float p, ant = 0, antV = 0, antC = 1; int i;
    OK(fabsf(faceta_p_cartaz(0) - 1.0f) < 1e-6f && fabsf(faceta_p_cartaz(1) - FACETA_ESCALA_CARTAZ) < 1e-6f, "cartaz 1 -> %.2f", FACETA_ESCALA_CARTAZ);
    OK(faceta_p_veu(0) == 0.0f && fabsf(faceta_p_veu(1) - FACETA_VEU) < 1e-6f, "veu 0 -> %.2f", FACETA_VEU);
    OK(FACETA_VEU <= 0.25f && FACETA_ESCALA_CARTAZ <= 1.08f, "sutileza: veu %.2f, escala %.2f", FACETA_VEU, FACETA_ESCALA_CARTAZ);
    for (p = 0; p <= 1.0001f; p += 0.01f) {
      float q = faceta_p_ficha(p, 2, 5), c = faceta_p_cartaz(p), v = faceta_p_veu(p);
      OK(q >= ant - 1e-6f && c >= antC - 1e-6f && v >= antV - 1e-6f, "curvas monotonicas em p=%.2f", p);
      ant = q; antV = v; antC = c;
      OK(faceta_p_rotulo(p) >= 0 && faceta_p_rotulo(p) <= 1 && faceta_p_orbita(p) >= 0 && faceta_p_orbita(p) <= 1, "0..1 em p=%.2f", p);
    }
    for (n = 1; n <= FACETA_MAX; n++)
      for (i = 0; i < n; i++) {
        OK(faceta_p_ficha(0, i, n) == 0.0f, "ficha %d/%d some em p=0", i, n);
        OK(fabsf(faceta_p_ficha(1, i, n) - 1.0f) < 1e-6f, "ficha %d/%d pousa em p=1 (%.4f)", i, n, faceta_p_ficha(1, i, n));
        if (i > 0) OK(faceta_p_ficha(0.4f, i, n) <= faceta_p_ficha(0.4f, i - 1, n) + 1e-6f, "cascata: ficha %d nao passa a %d", i, i - 1);
      }
    OK(faceta_p_rotulo(0.5f) == 0.0f, "o rotulo so abre depois que as fichas pousam");
  }
  // 6) orbita: tracejado cresce com o progresso; o dock nao tem
  { FacetaPonto pts[256]; float larg[3] = { 440, 440, 440 }; int a0, a1, a2;
    faceta_geometria(cresce(R(560, 318, 212, 322), 1.06f), 3, larg, 0, 0, W, H, &g);
    a0 = faceta_orbita(&g, 0.0f, pts, 256); a1 = faceta_orbita(&g, 0.5f, pts, 256); a2 = faceta_orbita(&g, 1.0f, pts, 256);
    OK(a0 == 0 && a1 > 0 && a2 > a1, "orbita: %d, %d, %d pontos (0, meio, completo)", a0, a1, a2);
    faceta_geometria(cresce(R(520, 300, 276, 419), 1.06f), 3, larg, 1, 70, W, H, &g);
    OK(faceta_orbita(&g, 1.0f, pts, 256) == 0, "dock nao tem orbita");
  }
  printf("%s: %d verificacoes, %d falhas\n", falhas ? "FAIL" : "PASS", total, falhas);
  return falhas ? 1 : 0;
}
