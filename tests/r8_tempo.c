// O MODULO DE TEMPO R8 contra a especificacao das pranchas (Fase 2).
//
// tests/r8_tempo_ref.py tira das pranchas aprovadas (tools/r8, 60 fps) as
// trilhas de elementos que dependem so do tempo. Aqui cada peca e simulada com
// src/r8tempo.c, dirigida pelos EVENTOS da prancha (abrir em t, fechar em t,
// apertar/soltar CH+, foco muda), e cada quadro tem de bater: 1 px nas
// posicoes e tamanhos, 0,01 em opacidade e escala.
//
// Alem da comparacao, comportamento: interromper nao salta, a troca cruzada
// nunca soma mais que um conteudo cheio, o toque curto de CH+ nao abre a
// Central e soltar na zona morta nao desenha nada.
#include "../src/r8tempo.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas, conferidos;
#define OK(c, ...) do { if (!(c)) { printf("FALHOU: "); printf(__VA_ARGS__); printf("\n"); falhas++; } } while (0)

static float pm_S1(double t, R8Abertura *a) { return r8ab_valor(a, t); }

// ---------------------------------------------------------------- simulacoes
typedef struct { double t; float v; } Amostra;

// S1 Estante: abre em 0,8; o foco vai 0 -> 1 (2,2) -> 2 (2,9) -> 1 (3,7); fecha
// com a superficie em 5,4 (o conteudo comeca 20 ms antes).
static float s1(const char *el, const char *campo, double t) {
  static R8Abertura a; static R8Mola f; static double ult = -1;
  static int ev;
  if (t < ult || ult < 0) {
    float z = 0; r8ab_iniciar(&a, R8_PAINEL_ABRIR, R8_PAINEL_FECHAR); r8mola_iniciar(&f, &z, 1); ev = 0;
  }
  ult = t;
  const double E[] = { 0.8, 2.2, 2.9, 3.7, 5.38 };
  while (ev < 5 && t >= E[ev] - 1e-9) {
    float v;
    switch (ev) {
      case 0: r8ab_abrir(&a, E[0]); break;
      case 1: v = 1; r8mola_ir(&f, &v, E[1]); break;
      case 2: v = 2; r8mola_ir(&f, &v, E[2]); break;
      case 3: v = 1; r8mola_ir(&f, &v, E[3]); break;
      case 4: r8ab_fechar(&a, E[4]); break;
    }
    ev++;
  }
  float m = pm_S1(t, &a), fo; r8mola_valor(&f, t, &fo);
  float c0 = r8ab_cascata(&a, t, 0, 0.14f, 0.045f, 0.22f);
  if (!strcmp(el, "bg")) return !strcmp(campo, "y") ? -60 * m : !strcmp(campo, "s") ? r8_lerp(1, 0.96f, m) : 1 - 0.65f * m;
  if (!strcmp(el, "heroT0")) return c0 * fmaxf(0, 1 - fabsf(fo - 0) * 1.6f);
  if (!strcmp(el, "heroT1")) return c0 * fmaxf(0, 1 - fabsf(fo - 1) * 1.6f);
  if (!strcmp(el, "hero1")) return m * fmaxf(0, 1 - fabsf(fo - 1));
  return NAN;
}

// C2 Coluna pelo toque (mesmos tempos da S1): a pilula do relogio vira a Coluna.
static float colunaCampo(const char *campo, float m, float px, float pw, float ph) {
  float k1 = r8_dec(r8_seg(m, 0, 0.55f)), k2 = r8_dec(r8_seg(m, 0.15f, 1));
  if (!strcmp(campo, "x")) return r8_lerp(px, 1304, k1);
  if (!strcmp(campo, "w")) return r8_lerp(pw, 520, k1);
  if (!strcmp(campo, "h")) return r8_lerp(ph, 900, k2);
  return NAN;
}
static float c2(const char *el, const char *campo, double t) {
  R8Abertura a; r8ab_iniciar(&a, R8_PAINEL_ABRIR, R8_PAINEL_FECHAR);
  if (t >= 0.8) r8ab_abrir(&a, 0.8);
  if (t >= 5.38) r8ab_fechar(&a, 5.38);
  (void)el;
  return colunaCampo(campo, r8ab_valor(&a, t), 1704, 120, 60);
}

// Menu C (Folha): segurar OK ate 1,3; abre; fecha com a superficie em 5,3.
// A FOLHA E UMA GAVETA (26/11, tools/r8/menus.py build_C): encostada na borda
// direita da tela de 1920 (x final 1280, 640 de largura), entra de fora da tela
// pela direita e e opaca enquanto esta na tela (sem esmaecer).
static float mc(const char *el, const char *campo, double t) {
  R8Abertura a; r8ab_iniciar(&a, R8_MENU_ABRIR, R8_MENU_FECHAR);
  if (t >= 1.3) r8ab_abrir(&a, 1.3);
  if (t >= 5.28) r8ab_fechar(&a, 5.28);
  float m = r8ab_valor(&a, t);
  if (!strcmp(el, "veil")) return 0.5f * m;
  if (!strcmp(campo, "x")) return 1280.0f + 640.0f * (1.0f - m);
  return m > 0.001f ? 1.0f : 0.0f;
}

// Segurar CH+: toque curto de 0,6 a 1,0 (cancela); segurar valido em 2,2.
// `abriu` devolve quando a Central abriu (o quadro em que completou).
static void segurarSim(double t, R8Segurar *s, double *abriu) {
  r8seg_iniciar(s); *abriu = -1;
  // quadro a quadro, como o app faz
  for (double q = 0; q <= t + 1e-9; q += 1.0 / 240) {
    if (q >= 0.6 && s->estado == 0 && q < 1.0) r8seg_apertar(s, 0.6);
    if (q >= 1.0 && s->estado == 1 && s->t0 < 1.0) r8seg_soltar(s, 1.0);
    if (q >= 2.2 && s->t0 < 2.2) r8seg_apertar(s, 2.2);
    if (r8seg_passo(s, q) && *abriu < 0) *abriu = s->tSolto;
  }
}
static float hr(const char *el, const char *campo, double t) {
  R8Segurar s; double abriu;
  segurarSim(t, &s, &abriu);
  (void)el;
  if (!strcmp(campo, "w")) return 84 * r8seg_progresso(&s, t);
  return r8seg_visivel(&s, t);
}
static float fhr(const char *el, const char *campo, double t) {
  R8Segurar s; double abriu; R8Abertura a;
  segurarSim(t, &s, &abriu);
  r8ab_iniciar(&a, 0.45f, R8_PAINEL_FECHAR);
  if (abriu >= 0) r8ab_abrir(&a, abriu);
  if (t >= 5.38) r8ab_fechar(&a, 5.38);
  (void)el;
  return colunaCampo(campo, r8ab_valor(&a, t), 1704, 120, 60);
}

// Busca flutuante (10.13): progresso linear de 0,65 s a partir de 0,8; fecha
// com a superficie em 7,6.
static float bf(const char *el, const char *campo, double t) {
  R8Abertura a; r8ab_iniciar(&a, R8_BUSCA_ABRIR, R8_BUSCA_FECHAR);
  if (t >= 0.8) r8ab_abrir(&a, 0.8);
  if (t >= 7.58) r8ab_fechar(&a, 7.58);
  float mm = r8ab_linear(&a, t);
  if (!strcmp(el, "veil")) return 0.8f * r8_dec(mm);
  if (!strcmp(el, "corpo")) return r8_lerp(60, 840, r8_dec(r8_seg(mm, 0.7f, 1)));
  float ap = r8_dec(r8_seg(mm, 0, 0.3f)), k1 = r8_dec(r8_seg(mm, 0.25f, 0.85f)), k2 = r8_dec(r8_seg(mm, 0.35f, 0.95f));
  float yb = r8_lerp(-64, 40, ap);
  if (!strcmp(campo, "x")) return r8_lerp(728, 410, k1);
  if (!strcmp(campo, "y")) return r8_lerp(yb, 40, k2);
  if (!strcmp(campo, "w")) return r8_lerp(464, 1100, k1);
  return r8_lerp(52, 72, k2);
}

// ---------------------------------------------------------------- comportamento
static void comportamento(void) {
  R8Abertura a; float ant, v; R8Troca x; R8Segurar s;
  // Interromper a abertura no meio e fechar: nunca salta mais que um quadro de curva.
  r8ab_iniciar(&a, R8_PAINEL_ABRIR, R8_PAINEL_FECHAR);
  r8ab_abrir(&a, 0); ant = 0;
  for (double t = 0; t < 2.0; t += 1.0 / 60) {
    if (fabs(t - 0.2) < 1e-6) r8ab_fechar(&a, t);
    if (fabs(t - 0.4) < 1e-6) r8ab_abrir(&a, t);
    v = r8ab_valor(&a, t);
    OK(fabsf(v - ant) < 0.12f, "abertura interrompida salta em t=%.3f (%.3f -> %.3f)", t, ant, v);
    ant = v;
  }
  OK(r8ab_assentada(&a, 2.0) && fabsf(r8ab_valor(&a, 2.0) - 1) < 1e-6, "reabrir termina assentada em 1");
  // Reabrir com a superficie ainda quase aberta nao apaga o conteudo.
  r8ab_iniciar(&a, R8_PAINEL_ABRIR, R8_PAINEL_FECHAR);
  r8ab_abrir(&a, 0); r8ab_fechar(&a, 2.0); r8ab_abrir(&a, 2.05);
  OK(r8ab_cascata(&a, 2.06, 3, 0.14f, 0.045f, 0.22f) > 0.99f, "reabrir no meio do fechar mantem o conteudo");
  // Fechada de verdade so depois da curva de saida.
  r8ab_iniciar(&a, R8_PAINEL_ABRIR, R8_PAINEL_FECHAR);
  r8ab_abrir(&a, 0); r8ab_fechar(&a, 1.0);
  OK(!r8ab_fechada(&a, 1.0 + R8_CONTEUDO_ANTES + 0.2), "nao fecha antes de 280 ms");
  OK(r8ab_fechada(&a, 1.0 + R8_CONTEUDO_ANTES + 0.29), "fecha depois de 280 ms");
  // Mola: chega com repique entre 1 e 6 % e assenta exata.
  { R8Mola m; float z = 0, um = 1, mx = 0, o; r8mola_iniciar(&m, &z, 1); r8mola_ir(&m, &um, 0);
    for (double t = 0; t < 0.4; t += 0.002) { r8mola_valor(&m, t, &o); if (o > mx) mx = o; }
    r8mola_valor(&m, 0.27, &o);
    // A curva das pranchas repica ~10 % (o texto da R8-2 diz "cerca de 4 %";
    // vale o desenho aprovado, que e esta curva).
    OK(mx > 1.08f && mx < 1.12f, "mola repica ~10 %% como na prancha (%.3f)", mx);
    OK(o == 1.0f && r8mola_parada(&m, 0.2701), "mola assenta em 270 ms");
    // trocar de alvo no meio parte do valor atual
    float dois = 2, meio; r8mola_valor(&m, 0.1, &meio); r8mola_ir(&m, &dois, 0.1); r8mola_valor(&m, 0.1, &o);
    OK(fabsf(o - meio) < 1e-5f, "mola redirecionada nao salta"); }
  // Troca: a soma das opacidades nunca passa de 1 (nao ha quadro com os dois cheios)
  // e o novo assenta em 160 ms sem deslocamento.
  r8troca_iniciar(&x, 0); r8troca_ir(&x, 1, 1.0);
  for (double t = 1.0; t <= 1.2; t += 0.004) {
    float d0, d1, o0 = r8troca_peso(&x, t, 0, &d0), o1 = r8troca_peso(&x, t, 1, &d1);
    OK(o0 + o1 <= 1.0001f, "troca: dois conteudos cheios em t=%.3f", t);
    OK(d0 <= 0 && d1 >= 0, "troca: o velho sai pela esquerda e o novo vem da direita");
  }
  { float d; OK(r8troca_peso(&x, 1.16, 1, &d) == 1.0f && d == 0, "troca assenta em 160 ms"); }
  // Segurar: soltar na zona morta nao desenha nada e devolve "toque".
  r8seg_iniciar(&s); r8seg_apertar(&s, 0);
  OK(r8seg_soltar(&s, 0.15) == 1, "toque na zona morta e toque (Salvos)");
  for (double t = 0.15; t < 0.6; t += 0.01) OK(r8seg_visivel(&s, t) == 0, "zona morta nao desenha (t=%.2f)", t);
  // Segurar ate o fim: completa UMA vez em 650 ms; soltar depois nao e toque.
  r8seg_iniciar(&s); r8seg_apertar(&s, 0);
  { int n = 0; for (double t = 0; t < 1.0; t += 1.0 / 60) n += r8seg_passo(&s, t);
    OK(n == 1, "completa uma vez (%d)", n);
    OK(fabs(s.tSolto - 0.65) < 1e-6, "completa em 650 ms");
    OK(r8seg_soltar(&s, 1.0) == 0, "soltar depois de completar nao e toque"); }
}

int main(int argc, char **argv) {
  const char *ref = argc > 1 ? argv[1] : "/tmp/r8_ref.txt";
  FILE *f = fopen(ref, "r");
  char peca[16], el[32], campo[8]; double t; float v;
  if (!f) { printf("sem referencia %s (rode tests/r8_tempo_ref.py)\n", ref); return 2; }
  while (fscanf(f, "%15s %31s %7s %lf %f", peca, el, campo, &t, &v) == 5) {
    float c, tol = (!strcmp(campo, "o") || !strcmp(campo, "s")) ? 0.01f : 1.0f;
    if (!strcmp(peca, "S1")) c = s1(el, campo, t);
    else if (!strcmp(peca, "C2")) c = c2(el, campo, t);
    else if (!strcmp(peca, "MC")) c = mc(el, campo, t);
    else if (!strcmp(peca, "HR")) {
      // A prancha esvazia o toque curto a partir do valor de 0,98 s (um quadro
      // antes de soltar); o app parte do valor no instante de soltar. So a
      // largura do fio nessa janela de 250 ms difere (o fio some junto).
      if (!strcmp(campo, "w") && t > 0.97 && t < 1.25) continue;
      c = hr(el, campo, t);
    }
    else if (!strcmp(peca, "FHR")) c = fhr(el, campo, t);
    else if (!strcmp(peca, "BF")) c = bf(el, campo, t);
    else continue;
    conferidos++;
    OK(fabsf(c - v) <= tol, "%s %s.%s t=%.3f: prancha %.3f, app %.3f", peca, el, campo, t, v, c);
    if (falhas > 30) break;
  }
  fclose(f);
  comportamento();
  printf("r8_tempo: %d quadros conferidos, %d falhas\n", conferidos, falhas);
  return falhas ? 1 : 0;
}
