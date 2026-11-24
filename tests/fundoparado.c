// FUNDO CONGELADO SOB A BUSCA (fundoparado.h): decisao copiar / repintar / direto
// com relogio falso, sem GL. Cada caso e um defeito que a Busca tinha:
//   1. republicacao do catalogo DEPOIS das repinturas por tempo: a copia era
//      refeita no quadro em que a cascata de entrada comeca (fileiras em
//      opacidade 0) e nunca mais, porque o contador de repinturas nao recomecava;
//   2. arte que ainda nao chegou: a copia ficava em branco para sempre;
//   3. copia preta (TCL): nunca percebida, a home sumia ate fechar a Busca.
#include "fundoparado.h"
#include <stdio.h>

static int falhas;
#define OK(c, msg) do { int _c = (c); printf("%s: %s\n", _c ? "PASS" : "FAIL", msg); if (!_c) falhas++; } while (0)

static const Uint32 REFAZ[] = { 400, 1500, 4000 };
#define PASSO(f, parar, rev, t) fundoparado_passo(&(f), (parar), (rev), (t), REFAZ, 3)

int main(void) {
  FundoParado f = {0};
  Uint32 t = 1000;
  FpAcao a;

  // Entrada: so congela com o painel assentado; antes disso, direto.
  OK(PASSO(f, 0, 1, t) == FP_DIRETO, "painel ainda entrando: home desenhada direto");
  OK(PASSO(f, 1, 1, t) == FP_PINTAR, "painel assentado: pinta a copia");
  fundoparado_pintado(&f, 0, 0, 1, t);
  OK(PASSO(f, 1, 1, t + 16) == FP_COPIAR, "copia pronta: so copia");

  // Repinturas por tempo a 0,4 / 1,5 / 4 s da primeira pintura.
  OK(PASSO(f, 1, 1, t + 399) == FP_COPIAR, "399 ms: ainda copia");
  a = PASSO(f, 1, 1, t + 400);
  OK(a == FP_PINTAR, "400 ms: repinta");
  fundoparado_pintado(&f, 0, 0, 1, t + 400);
  OK(PASSO(f, 1, 1, t + 1499) == FP_COPIAR, "1499 ms: copia");
  OK(PASSO(f, 1, 1, t + 1500) == FP_PINTAR, "1500 ms: repinta");
  fundoparado_pintado(&f, 0, 0, 1, t + 1500);
  OK(PASSO(f, 1, 1, t + 4000) == FP_PINTAR, "4000 ms: repinta (a ultima por tempo)");
  fundoparado_pintado(&f, 0, 0, 1, t + 4000);
  OK(PASSO(f, 1, 1, t + 9000) == FP_COPIAR, "esgotadas as repinturas por tempo: so copia");

  // 1. O CATALOGO E REPUBLICADO 9 s depois (o sync mexe nele): repinta ja, e os
  //    tempos RECOMECAM (a cascata de entrada das fileiras dura ~1 s).
  Uint32 r0 = t + 9500;
  OK(PASSO(f, 1, 2, r0) == FP_PINTAR, "catalogo republicado: repinta no mesmo quadro");
  fundoparado_pintado(&f, 0, 0, 2, r0);
  OK(PASSO(f, 1, 2, r0 + 399) == FP_COPIAR, "republicacao: 399 ms depois ainda copia");
  OK(PASSO(f, 1, 2, r0 + 400) == FP_PINTAR,
     "republicacao: 400 ms depois REPINTA (a copia do quadro da cascata, vazia, nao fica)");
  fundoparado_pintado(&f, 0, 0, 2, r0 + 400);
  OK(PASSO(f, 1, 2, r0 + 1500) == FP_PINTAR, "republicacao: 1,5 s depois repinta de novo");
  fundoparado_pintado(&f, 0, 0, 2, r0 + 1500);
  OK(PASSO(f, 1, 2, r0 + 4000) == FP_PINTAR, "republicacao: 4 s depois repinta de novo");
  fundoparado_pintado(&f, 0, 0, 2, r0 + 4000);
  OK(PASSO(f, 1, 2, r0 + 8000) == FP_COPIAR, "e assenta");

  // Republicacoes em sequencia nao se acumulam: cada uma recomeca.
  OK(PASSO(f, 1, 3, r0 + 8100) == FP_PINTAR, "segunda republicacao: repinta");
  fundoparado_pintado(&f, 0, 0, 3, r0 + 8100);
  OK(PASSO(f, 1, 3, r0 + 8100 + 400) == FP_PINTAR, "segunda republicacao: tambem recomeca os tempos");
  fundoparado_pintado(&f, 0, 0, 3, r0 + 8500);

  // 2. ARTE FALTANDO: repinta a cada 150 ms ate chegar, e para.
  FundoParado g = {0};
  Uint32 u = 50000;
  OK(PASSO(g, 1, 1, u) == FP_PINTAR, "arte: primeira pintura");
  fundoparado_pintado(&g, 1, 0, 1, u);
  OK(PASSO(g, 1, 1, u + 149) == FP_COPIAR, "arte faltando: 149 ms ainda copia");
  OK(PASSO(g, 1, 1, u + 150) == FP_PINTAR, "arte faltando: 150 ms repinta para pegar a arte que chegou");
  fundoparado_pintado(&g, 0, 0, 1, u + 150);
  OK(PASSO(g, 1, 1, u + 160) == FP_COPIAR, "arte chegou: volta a copiar");
  // Arte que nunca chega: desiste (nao repinta para sempre).
  FundoParado h = {0};
  Uint32 v = 90000; int pinturas = 0;
  fundoparado_passo(&h, 1, 1, v, REFAZ, 0);
  fundoparado_pintado(&h, 1, 0, 1, v);
  for (int i = 0; i < 500; i++) {
    v += 151;
    if (fundoparado_passo(&h, 1, 1, v, REFAZ, 0) == FP_PINTAR) { pinturas++; fundoparado_pintado(&h, 1, 0, 1, v); }
  }
  OK(pinturas == FP_FALTA_MAX, "arte que nunca chega: desiste depois de FP_FALTA_MAX repinturas");

  // 3. COPIA PRETA: home direto ate a Busca fechar; ao reabrir, tenta de novo.
  FundoParado p = {0};
  Uint32 w = 70000;
  OK(PASSO(p, 1, 1, w) == FP_PINTAR, "preta: primeira pintura");
  fundoparado_pintado(&p, 0, 1, 1, w);
  OK(PASSO(p, 1, 1, w + 16) == FP_DIRETO, "copia preta: a home e desenhada direto");
  OK(PASSO(p, 1, 1, w + 10000) == FP_DIRETO, "copia preta: continua direto");
  OK(PASSO(p, 0, 1, w + 10100) == FP_DIRETO, "Busca fechando: direto");
  OK(PASSO(p, 1, 1, w + 20000) == FP_PINTAR, "Busca reaberta: tenta a copia de novo");

  // Fechar e reabrir zera tudo.
  fundoparado_pintado(&p, 0, 0, 1, w + 20000);
  OK(PASSO(p, 1, 1, w + 20016) == FP_COPIAR, "reaberta: copia pronta");
  PASSO(p, 0, 1, w + 21000);
  OK(PASSO(p, 1, 1, w + 21016) == FP_PINTAR, "fechou e reabriu: pinta uma copia nova");

  printf("RESULTADO: %d falhas\n", falhas);
  return falhas ? 1 : 0;
}
