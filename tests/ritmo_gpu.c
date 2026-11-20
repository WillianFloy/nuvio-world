// FRAME PACING COM A GPU NO LIMITE (auditoria 20/11, P1).
//
// O que este teste reproduz: a Home da TCL a ~36 fps numa tela de 60 Hz. Com
// vsync, cada quadro ocupa 1 ou 2 varreduras (cadencia 1-2-2: 16,7 / 33,3 /
// 33,3 ms). Antes da correcao, o relato usava como prazo a EMA do swapint, que
// converge para o proprio periodo do quadro (~28 ms) e dizia: 0 fora do prazo,
// 0 varreduras puladas, histograma todo na coluna 1. O teste roda O MESMO
// CAMINHO de main.c -- swapint_marca + rq_quadro a cada swap, rq_relato com
// swapint_referencia(hz da tela) a cada 3 s, limite de jank e [qd-lento] por
// quadro -- alimentado so por carimbos de tempo, como a TV entrega.
//
// A EMA antiga dependia da HISTORIA: so passava a seguir o ritmo do app depois
// de 8 quadros lentos SEGUIDOS (SWAPINT_TROCA_N). Na TV isso acontece na
// montagem da Home (decode/upload de arte: dezenas de quadros de 40-60 ms); dai
// em diante qualquer quadro ate 1,5x a EMA a puxa para a media, e na cadencia
// 1-2-2 ela assenta em ~28 ms. O log da TCL da etapa 5 mostrava exatamente
// isso ([swap] periodo~25-28 ms na Home/Collections). O cenario reproduz a
// sequencia: arranque a 60, montagem da Home, navegacao a ~36 fps.
//
// Cada cenario tambem roda o CRITERIO ANTIGO (prazo = EMA) como controle, para
// provar que o cenario reproduz o problema da auditoria: se o controle nao
// esconder o atraso, o cenario nao serve de prova.
#include "../src/swapint.h"
#include "../src/ritmoquadro.h"
#include <stdio.h>

static int falhas;
#define OK(c, ...) do { if (c) { printf("PASS: "); printf(__VA_ARGS__); printf("\n"); } \
                         else { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); falhas++; } } while (0)

typedef struct {
  RqRelato novo, antigo;   // antigo = prazo pela EMA (o comportamento ate 20/11)
  int janks, janksAntigo, lentos, lentosAntigo, quadros;
} Saida;

// Gerador deterministico (LCG) para o ruido de +-jit ms no carimbo.
static unsigned semente = 12345u;
static double ruido(double jit) {
  semente = semente * 1103515245u + 12345u;
  return jit * (((double)((semente >> 8) & 0xffff) / 65535.0) * 2.0 - 1.0);
}

// `aquece`: quadros a cadencia cheia antes do cenario (telas leves do arranque),
// como na TV: o app abre a 60 e so a Home cheia cai. `varr`: varreduras por
// quadro, repetido ate `n` quadros. `hzInformado`: o que NUVIO_DISPLAY_HZ diz
// (0 = plataforma sem a informacao). `hzReal`: a varredura de verdade.
// `carga`: quadros de 50 ms seguidos da montagem da Home (decode/upload).
static Saida roda(double hzReal, double hzInformado, int aquece, int carga, const int *varr, int nv, int n, double jit) {
  static RitmoQuadro r, ra;
  SwapInt s = {0};
  Saida o;
  double p = 1000.0 / hzReal, t = 1000.0, ult = 0.0;
  int i;
  memset(&r, 0, sizeof r); memset(&ra, 0, sizeof ra); memset(&o, 0, sizeof o);
  for (i = 0; i < aquece; i++) { t += p; swapint_marca(&s, t); }
  for (i = 0; i < carga; i++) { t += 3 * p; swapint_marca(&s, t); }
  // Aquecimento sai do relato (como o relato de 3 s anterior, ja colhido).
  rq_quadro(&r, t, t - 5, 4, 1); rq_quadro(&ra, t, t - 5, 4, 1); ult = t;
  for (i = 0; i < n; i++) {
    double apres = t + varr[i % nv] * p, dt;
    t = apres;
    apres += ruido(jit);
    dt = apres - ult; ult = apres;
    // por quadro (main.c): jank e [qd-lento] contra a referencia
    {
      double ref = swapint_referencia(&s, hzInformado), refAnt = swapint_periodo(&s);
      double limAnt = refAnt * 2.0 < 33.0 ? 33.0 : refAnt * 2.0;
      o.janks += dt > swapint_limite_jank(ref);
      o.janksAntigo += dt > limAnt;
      o.lentos += rq_quadro_lento(dt, ref);
      o.lentosAntigo += dt > 2.5 * refAnt;
    }
    swapint_marca(&s, apres);
    rq_quadro(&r, apres, apres - 6, 5, 1);
    rq_quadro(&ra, apres, apres - 6, 5, 1);
  }
  o.quadros = n;
  o.novo = rq_relato(&r, swapint_referencia(&s, hzInformado));
  o.antigo = rq_relato(&ra, swapint_periodo(&s));
  return o;
}

static void mostra(const char *nome, const Saida *o) {
  char b[640];
  rq_formatar(&o->novo, 0, "teste", b, sizeof b);
  printf("  %s\n    novo:   %s\n", nome, b);
  rq_formatar(&o->antigo, 0, "ema", b, sizeof b);
  printf("    antigo: %s\n", b);
}

int main(void) {
  // 1. O caso da auditoria: 60 Hz, Home a ~36 fps (1-2-2).
  {
    const int c[3] = { 1, 2, 2 };
    Saida o = roda(60.0, 60.0, 600, 30, c, 3, 180, 0.0);   // 30 quadros de montagem; 180 quadros = 10 s de Home
    mostra("60 Hz / Home ~36 fps (1-2-2)", &o);
    OK(o.antigo.foraPrazo == 0 && o.antigo.pulados == 0 && o.antigo.hist[1] == 0,
       "controle: o criterio antigo (EMA) esconde o atraso -- fora=%d pulados=%d hist2=%d (reproduz a auditoria)",
       o.antigo.foraPrazo, o.antigo.pulados, o.antigo.hist[1]);
    OK(o.novo.fps > 35.5 && o.novo.fps < 36.5, "fps apresentado = quadros/tempo = %.2f (~36)", o.novo.fps);
    OK(fabs(o.novo.periodo - 16.667) < 0.01, "referencia = varredura de 60 Hz (%.3f ms), nao o ritmo do app", o.novo.periodo);
    OK(o.novo.foraPrazo == 120 && o.novo.pulados == 120,
       "2 de cada 3 quadros fora do prazo (%d/180) e 120 varreduras repetidas (%d)", o.novo.foraPrazo, o.novo.pulados);
    OK(o.novo.hist[0] == 60 && o.novo.hist[1] == 120 && o.novo.hist[2] == 0,
       "histograma 1/2 varreduras = %d/%d (a cadencia irregular aparece)", o.novo.hist[0], o.novo.hist[1]);
    OK(fabs(o.novo.ritmoMedio - 27.78) < 0.05, "ritmo medio continua no log: %.2f ms", o.novo.ritmoMedio);
    OK(o.lentos == 0 && o.lentosAntigo == 0, "[qd-lento] nao dispara por cadencia 2 varreduras (33 ms < 41,7)");
  }
  // 2. GPU presa em 2 varreduras (30 fps uniformes numa tela de 60 Hz).
  {
    const int c[1] = { 2 };
    Saida o = roda(60.0, 60.0, 600, 30, c, 1, 90, 0.0);
    mostra("60 Hz / GPU presa em 30 fps", &o);
    OK(o.antigo.foraPrazo == 0 && o.antigo.hist[0] == 90, "controle: o antigo chamava 30 fps de cadencia cheia (hist1=%d)", o.antigo.hist[0]);
    OK(o.novo.foraPrazo == 90 && o.novo.pulados == 90 && o.novo.hist[1] == 90,
       "novo: 100%% fora do prazo, 1 varredura repetida por quadro (fora=%d pulados=%d)", o.novo.foraPrazo, o.novo.pulados);
  }
  // 3. Tela que REALMENTE roda a 30 Hz (informada): 30 fps e pontual.
  {
    const int c[1] = { 1 };
    Saida o = roda(30.0, 30.0, 300, 30, c, 1, 90, 0.0);
    OK(o.novo.foraPrazo == 0 && o.novo.pulados == 0 && o.novo.hist[0] == 90 && o.janks == 0,
       "30 Hz informado a 30 fps: pontual, sem falso atraso");
  }
  // 4. Cadencia cheia em 50/60/90/100/120 Hz: zero atraso, inclusive com ruido de +-1,5 ms.
  {
    const double hz[5] = { 50, 60, 90, 100, 120 };
    const int c[1] = { 1 };
    int k;
    for (k = 0; k < 5; k++) {
      Saida o = roda(hz[k], hz[k], 300, 30, c, 1, 300, hz[k] >= 100 ? 1.0 : 1.5);
      OK(o.novo.foraPrazo == 0 && o.novo.pulados == 0 && o.lentos == 0 && fabs(o.novo.fps - hz[k]) < 0.5,
         "%.0f Hz cadencia cheia com ruido: fps=%.1f fora=%d", hz[k], o.novo.fps, o.novo.foraPrazo);
    }
  }
  // 5. Tranco real (1 em 10 quadros leva 4 varreduras): detectado nos dois.
  {
    const int c[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 };
    Saida o = roda(60.0, 60.0, 300, 30, c, 10, 300, 0.0);
    OK(o.novo.foraPrazo == 30 && o.novo.pulados == 90 && o.lentos == 30 && o.janks == 30,
       "tranco 1/10: fora=%d pulados=%d [qd-lento]=%d janks=%d", o.novo.foraPrazo, o.novo.pulados, o.lentos, o.janks);
  }
  // 6. Plataforma SEM a frequencia informada (so o Android informa): fica o
  //    criterio antigo -- de proposito, para nao regredir a TV a 30 Hz com o
  //    SDL dizendo 60 (swapint.h). Limite conhecido, registrado aqui.
  {
    const int c[3] = { 1, 2, 2 };
    Saida o = roda(60.0, 0.0, 600, 30, c, 3, 180, 0.0);
    OK(o.novo.foraPrazo == o.antigo.foraPrazo && fabs(o.novo.periodo - o.antigo.periodo) < 1e-9,
       "sem Hz informado: comportamento de antes preservado (ref=%.2f ms, limite conhecido)", o.novo.periodo);
  }
  // 7. 120 Hz com o app a 60 fps: uniforme (2 varreduras sempre), cadencia que casa.
  {
    const int c[1] = { 2 };
    Saida o = roda(120.0, 120.0, 600, 30, c, 1, 120, 0.0);
    OK(o.novo.hist[1] == 120 && o.novo.p50 == o.novo.p99 && rq_cadencia_uniforme(120, o.novo.fps) == 2,
       "120 Hz a 60 fps: tudo na coluna 2, p50=p99 (uniforme), cadencia 2");
  }
  // 8. Jank e o medidor de desempenho: 36 fps na Home deixam de dar "0 janks".
  {
    const int c[3] = { 1, 2, 2 };
    Saida o = roda(60.0, 60.0, 600, 30, c, 3, 180, 0.6);
    OK(o.janksAntigo == 0, "controle: limite antigo (2x EMA) -> %d janks", o.janksAntigo);
    OK(o.janks >= 40, "novo: quadros de 2 varreduras com ruido passam de 33,3 ms e contam como jank (%d/180)", o.janks);
  }
  // 9. Valores fora da faixa de Hz caem no piso (nunca divisao por lixo).
  {
    SwapInt s = {0};
    OK(fabs(swapint_referencia(&s, 0.0) - 16.7) < 0.01 && fabs(swapint_referencia(&s, 1000.0) - 16.7) < 0.01 &&
       fabs(swapint_referencia(&s, 59.94) - 1000.0 / 59.94) < 1e-9, "faixa 20..500 Hz; fora dela, piso/16,7");
  }
  if (falhas) { printf("ritmo_gpu: %d falha(s)\n", falhas); return 1; }
  puts("ritmo_gpu: ok");
  return 0;
}
