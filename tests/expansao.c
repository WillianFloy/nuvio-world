// EXPANSAO DO CARTAZ: entrada e saida animadas (expansao.h), Home e Vitrine.
// O que o dono via: "e seco e da uma leve bugada". Comportamento cobrado, a 60
// quadros por segundo:
//   1. abre so depois do atraso parado, com partida MACIA (o primeiro quadro
//      anda pouco; a mola antiga andava 13% de uma vez) e chega sem passar do
//      ponto;
//   2. mover o foco FECHA ANIMADO o que estava aberto: o empurrao dos vizinhos
//      cai sem salto de um quadro para o outro (antes caia tudo num quadro);
//   3. voltar a um cartao que fecha continua de onde ele estava;
//   4. seta segurada por varios cartoes abertos nao gera salto;
//   5. menu do cartaz (pausado) segura; animacao reduzida e instantanea;
//   6. a arte deitada cruza sobre o cartaz entre 30% e 75%, sem degrau;
//   7. a arte deitada e pedida antes de abrir (metade do atraso, 200-600 ms).
#include "../src/expansao.h"
#include <stdio.h>
#include <math.h>
static int falhas;
#define OK(c, m) do { if (c) printf("ok    %s\n", m); else { printf("FALHA %s\n", m); falhas++; } } while (0)
#define DT (1.0f / 60.0f)
#define UN 569.0f   // o que um cartaz de 322x483 ganha aberto (home.c)

static unsigned agora = 1000;
static void quadro(ExpEstado *e, int r, int c, int pode, int pausado, unsigned atraso) {
  agora += 17;
  exp_passo(e, r, c, pode, pausado, agora, atraso, DT, 0);
}

int main(void) {
  ExpEstado e;
  int i;
  exp_zerar(&e);
  // 1. Abre depois do atraso, macio.
  for (i = 0; i < 29; i++) quadro(&e, 0, 2, 1, 0, 500);
  OK(exp_propria(&e, 0, 2) == 0.0f, "antes do atraso (500 ms) nada abre");
  { float p0 = 0.0f, p1, maxSalto = 0.0f, ant = 0.0f; int mono = 1, chegou = -1;
    while (exp_propria(&e, 0, 2) == 0.0f) quadro(&e, 0, 2, 1, 0, 500);
    p1 = exp_propria(&e, 0, 2);
    OK(p1 > p0 && p1 < 0.03f, "primeiro quadro da abertura anda menos de 3% (partida macia)");
    ant = p1;
    for (i = 0; i < 60; i++) {
      float p;
      quadro(&e, 0, 2, 1, 0, 500);
      p = exp_propria(&e, 0, 2);
      if (p < ant - 1e-6f) mono = 0;
      if (p - ant > maxSalto) maxSalto = p - ant;
      if (chegou < 0 && p >= 0.95f) chegou = (i + 2) * 17;
      ant = p;
    }
    OK(mono && ant <= 1.0f, "abertura monotona, sem passar de 1");
    OK(chegou > 0 && chegou <= 480, "abre (95%) em ate 480 ms");
    printf("      abertura: 95%% em %d ms, maior passo %.3f por quadro\n", chegou, maxSalto);
    OK(maxSalto < 0.09f, "nenhum quadro da abertura anda mais que 9%"); }

  // 2. Move o foco para a direita: o aberto fecha animado.
  { float antes = exp_empurra(&e, 0, 5, NULL, NULL, UN), ant = antes, maxQueda = 0.0f;
    int fechou = -1;
    quadro(&e, 0, 3, 1, 0, 500);
    OK(exp_propria(&e, 0, 2) > 0.9f, "no quadro da tecla o cartao anterior ainda esta aberto");
    for (i = 0; i < 40; i++) {
      float d = exp_empurra(&e, 0, 5, NULL, NULL, UN);
      if (ant - d > maxQueda) maxQueda = ant - d;
      if (fechou < 0 && exp_propria(&e, 0, 2) <= 0.05f) fechou = (i + 1) * 17;
      ant = d;
      quadro(&e, 0, 3, 1, 0, 500);
    }
    printf("      fechamento: 95%% em %d ms, maior queda do empurrao %.0f px por quadro (de %.0f)\n",
           fechou, maxQueda, antes);
    OK(fechou > 0 && fechou <= 360, "fecha (95%) em ate 360 ms");
    OK(maxQueda < UN * 0.10f, "o vizinho volta sem salto (< 10% da abertura por quadro; antes 100%)");
    OK(exp_propria(&e, 0, 2) == 0.0f, "o anterior termina fechado de todo");
    OK(exp_propria(&e, 0, 3) > 0.0f && exp_propria(&e, 0, 3) < 0.6f, "o novo so comecou depois do atraso (680 ms de foco, 500 de atraso)"); }

  // 3. Volta a um cartao que ainda fecha: continua de onde estava.
  { float meio;
    for (i = 0; i < 80; i++) quadro(&e, 0, 3, 1, 0, 500);
    quadro(&e, 0, 4, 1, 0, 500);
    for (i = 0; i < 5; i++) quadro(&e, 0, 4, 1, 0, 500);
    meio = exp_propria(&e, 0, 3);
    quadro(&e, 0, 3, 1, 0, 500);
    OK(meio > 0.1f && meio < 0.95f && fabsf(exp_propria(&e, 0, 3) - meio) < 0.08f,
       "voltar ao cartao que fecha continua de onde ele estava (sem pular a 0)"); }

  // 4. Seta segurada atravessando cartoes abertos: o empurrao nunca salta.
  { ExpEstado s; float ant, maxSalto = 0.0f; int c;
    exp_zerar(&s);
    for (c = 0; c < 6; c++) {
      int k;
      for (k = 0; k < 45; k++) quadro(&s, 1, c, 1, 0, 300);   // cada um chega a abrir
    }
    ant = exp_empurra(&s, 1, 12, NULL, NULL, UN);
    for (c = 6; c < 12; c++) {
      int k;
      for (k = 0; k < 4; k++) {                               // e agora a seta corre
        float d;
        quadro(&s, 1, c, 1, 0, 300);
        d = exp_empurra(&s, 1, 12, NULL, NULL, UN);
        if (fabsf(d - ant) > maxSalto) maxSalto = fabsf(d - ant);
        ant = d;
      }
    }
    printf("      seta segurada: maior variacao do empurrao %.0f px por quadro\n", maxSalto);
    OK(maxSalto < UN * 0.12f, "seta segurada por cartoes abertos: sem salto"); }

  // 5. Pausado (menu do cartaz) e animacao reduzida.
  { ExpEstado p; float x;
    exp_zerar(&p);
    for (i = 0; i < 40; i++) quadro(&p, 2, 1, 1, 0, 300);
    x = exp_propria(&p, 2, 1);
    for (i = 0; i < 30; i++) quadro(&p, 2, 1, 1, 1, 300);
    OK(x > 0.0f && x < 1.0f && exp_propria(&p, 2, 1) == x, "menu aberto: a abertura fica onde esta");
    exp_zerar(&p);
    for (i = 0; i < 30; i++) quadro(&p, 2, 1, 1, 1, 300);
    OK(exp_propria(&p, 2, 1) == 0.0f, "menu aberto: o que nao abriu espera");
    exp_zerar(&p);
    for (i = 0; i < 20; i++) { agora += 17; exp_passo(&p, 2, 1, 1, 0, agora, 300, DT, 1); }
    OK(exp_propria(&p, 2, 1) == 1.0f, "animacao reduzida: abre de uma vez");
    agora += 17; exp_passo(&p, 2, 2, 1, 0, agora, 300, DT, 1);
    OK(exp_propria(&p, 2, 1) == 0.0f, "animacao reduzida: fecha de uma vez");
    // Fileira que nao pode abrir (deitada): o aberto fecha animado.
    exp_zerar(&p);
    for (i = 0; i < 45; i++) quadro(&p, 3, 0, 1, 0, 300);
    quadro(&p, 3, 0, 0, 0, 300);
    OK(exp_propria(&p, 3, 0) > 0.8f, "foco que vai para fileira que nao abre: fecha animado, nao num quadro"); }

  // 6. Cruzamento da arte.
  { float ant = -1.0f, maxSalto = 0.0f; int mono = 1;
    OK(exp_mistura_arte(0.0f) == 0.0f && exp_mistura_arte(0.30f) == 0.0f, "arte deitada ausente ate 30%");
    OK(exp_mistura_arte(0.75f) == 1.0f && exp_mistura_arte(1.0f) == 1.0f, "arte deitada inteira a partir de 75%");
    for (i = 0; i <= 100; i++) {
      float m = exp_mistura_arte(i / 100.0f);
      if (ant >= 0.0f) { if (m < ant) mono = 0; if (m - ant > maxSalto) maxSalto = m - ant; }
      ant = m;
    }
    OK(mono && maxSalto < 0.04f, "cruzamento monotono e sem degrau (antes: troca num quadro em 50%)"); }

  // 7. Pre-pedido da arte deitada.
  { ExpEstado q; unsigned t0;
    exp_zerar(&q);
    quadro(&q, 0, 0, 1, 0, 1000); t0 = agora;
    OK(!exp_pre_pedir(&q, t0 + 400, 1000), "atraso 1 s: nao pede aos 400 ms (o foco ainda passa)");
    OK(exp_pre_pedir(&q, t0 + 500, 1000), "atraso 1 s: pede aos 500 ms, antes de abrir");
    OK(!exp_pre_pedir(&q, t0 + 150, 0), "atraso 0: espera pelo menos 200 ms");
    OK(exp_pre_pedir(&q, t0 + 600, 5000), "atraso longo: pede aos 600 ms"); }

  printf("%s\n", falhas ? "expansao: FALHOU" : "expansao: tudo ok");
  return falhas != 0;
}
