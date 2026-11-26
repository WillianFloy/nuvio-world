// A COPIA PARADA DO MENU DE SEGURAR OK (r8fundo.h): QUANDO pintar, com relogio
// falso, sem GL. Cada caso e um defeito visto na TCL em 26/11 ou um teto que
// impede o defeito oposto (repintar sem fim):
//   1. repinturas por relogio (0,4/1,5/4 s), 4 por seta na lista de estilos,
//      45-60 ms de GPU cada: agora nenhuma sem mudanca;
//   2. a copia tirada com a Home no meio de uma troca: so com ela assentada;
//   3. arte que nunca chega repintava a cada 150 ms por 12 s: agora so quando
//      uma textura nova chega, no maximo R8F_ARTE_MAX vezes;
//   4. texto sem rasterizar (numerais do Ranking): repinta, com teto.
#include "r8fundo.h"
#include <stdio.h>
#include <string.h>

static int falhas;
#define OK(c, msg) do { int _c = (c); printf("%s: %s\n", _c ? "PASS" : "FAIL", msg); if (!_c) falhas++; } while (0)

static int pediu;
#define D(f, parar, ses, rev, ass, upl, t) r8fundo_decidir(&(f), (parar), (ses), (rev), (ass), (upl), (t), &pediu)

int main(void) {
  R8Fundo f = {0};
  Uint32 t = 1000;
  int k, n;

  // ABERTURA: pede a Home no alvo e mostra a Home viva enquanto isso; pinta
  // quando ela assentar.
  OK(D(f, 1, 1, 7, 0, 0, t) == R8F_ESPERA && pediu, "abertura: pede home_assentar e espera (Home viva)");
  OK(D(f, 1, 1, 7, 1, 0, t + 16) == R8F_PINTAR && !pediu, "abertura: Home assentada, pinta");
  r8fundo_pintou(&f, 0, 0, 0, 1, 0);
  OK(D(f, 1, 1, 7, 1, 0, t + 32) == R8F_COPIAR, "copia pronta: so copia");

  // 1. SEM MUDANCA, NENHUMA REPINTURA, por mais tempo que passe.
  n = 0;
  for (k = 0; k < 600; k++) if (D(f, 1, 1, 7, 1, 0, t + 48 + (Uint32)k * 16) == R8F_PINTAR) n++;
  OK(n == 0, "menu parado ~10 s: nenhuma repintura por relogio");

  // ESTILO TROCADO: pede o alvo, mostra a copia anterior e pinta UMA vez.
  t += 10000;
  OK(D(f, 1, 1, 8, 0, 0, t) == R8F_COPIAR && pediu, "estilo trocado: pede home_assentar, mostra a copia anterior");
  OK(D(f, 1, 1, 8, 1, 0, t + 16) == R8F_PINTAR, "estilo trocado: Home assentada, pinta");
  r8fundo_pintou(&f, 0, 0, 0, 1, 0);
  n = 0;
  for (k = 0; k < 200; k++) if (D(f, 1, 1, 8, 1, 0, t + 32 + (Uint32)k * 16) == R8F_PINTAR) n++;
  OK(n == 0, "estilo trocado: uma pintura so");

  // 2. A HOME QUE NAO ASSENTA nao e pintada no meio, ate o teto.
  t += 5000;
  D(f, 1, 1, 9, 0, 0, t);
  n = 0;
  for (k = 1; k < 120; k++) if (D(f, 1, 1, 9, 0, 0, t + (Uint32)k * 16) == R8F_PINTAR) { n = k; break; }
  OK(n * 16 >= (int)R8F_TETO_MS - 16 && n * 16 <= (int)R8F_TETO_MS + 16, "Home mexendo: pinta so no teto (1,5 s), nao no meio");
  r8fundo_pintou(&f, 0, 0, 0, 0, 0);   // pintada mexendo
  OK(D(f, 1, 1, 9, 0, 0, t + 1600) == R8F_COPIAR, "pintada mexendo: espera ela assentar");
  OK(D(f, 1, 1, 9, 1, 0, t + 1616) == R8F_PINTAR && !strcmp(f.motivo, "assentou"), "assentou: refaz uma vez");
  r8fundo_pintou(&f, 0, 0, 0, 1, 0);
  OK(D(f, 1, 1, 9, 1, 0, t + 1632) == R8F_COPIAR, "e para");

  // A HOME VOLTA A SE MEXER DEPOIS de uma copia assentada (o destaque troca
  // quando a arte dele chega): refaz uma vez, quando ela parar de novo.
  t += 5000;
  OK(D(f, 1, 1, 9, 0, 0, t) == R8F_COPIAR, "Home mexeu depois da copia: mostra a copia");
  OK(D(f, 1, 1, 9, 0, 0, t + 16) == R8F_COPIAR, "ainda mexendo: nao pinta no meio");
  OK(D(f, 1, 1, 9, 1, 0, t + 32) == R8F_PINTAR && !strcmp(f.motivo, "assentou"), "parou: refaz no mesmo quadro (ja assentada)");
  r8fundo_pintou(&f, 0, 0, 0, 1, 0);

  // 3. ARTE: so quando uma textura NOVA chega e assenta; nunca sem chegar.
  t += 5000;
  { R8Fundo a = {0}; unsigned long upl = 100;
    D(a, 1, 2, 1, 1, upl, t); D(a, 1, 2, 1, 1, upl, t);
    r8fundo_pintou(&a, 1, 0, 0, 1, upl);   // faltou arte
    n = 0;
    for (k = 0; k < 900; k++) if (D(a, 1, 2, 1, 1, upl, t + (Uint32)k * 16) == R8F_PINTAR) n++;
    OK(n == 0, "arte que nunca chega: nenhuma repintura em 14 s");
    upl++;   // chegou uma textura
    OK(D(a, 1, 2, 1, 1, upl, t + 20000) == R8F_COPIAR, "textura chegou: espera o esvanecer dela");
    OK(D(a, 1, 2, 1, 1, upl, t + 20000 + R8F_QUIETO_MS) == R8F_PINTAR && !strcmp(a.motivo, "arte"), "textura assentou: repinta (arte)");
    r8fundo_pintou(&a, 1, 0, 0, 1, upl);
    n = 1;
    for (k = 0; k < 40; k++) {
      Uint32 tk = t + 30000 + (Uint32)k * 1000;
      upl++;
      D(a, 1, 2, 1, 1, upl, tk);
      if (D(a, 1, 2, 1, 1, upl, tk + R8F_QUIETO_MS) == R8F_PINTAR) { n++; r8fundo_pintou(&a, 1, 0, 0, 1, upl); }
    }
    OK(n == R8F_ARTE_MAX, "arte faltando sempre: no maximo R8F_ARTE_MAX repinturas por abertura"); }

  // 4. TEXTO sem rasterizar: repinta no quadro seguinte, com teto.
  { R8Fundo x = {0};
    D(x, 1, 3, 1, 1, 0, t); D(x, 1, 3, 1, 1, 0, t);
    r8fundo_pintou(&x, 0, 1, 0, 1, 0);   // faltou texto
    OK(D(x, 1, 3, 1, 1, 0, t + 16) == R8F_PINTAR && !strcmp(x.motivo, "texto"), "texto faltando: repinta no quadro seguinte");
    r8fundo_pintou(&x, 0, 0, 0, 1, 0);   // completou
    OK(D(x, 1, 3, 1, 1, 0, t + 32) == R8F_COPIAR, "texto completo: para");
    n = 0;
    for (k = 0; k < 20; k++) {
      R8fAcao q = D(x, 1, 3, 2, 1, 0, t + 100 + (Uint32)k * 16);
      if (q == R8F_PINTAR) { n++; r8fundo_pintou(&x, 0, 1, 0, 1, 0); }   // sempre faltando
    }
    OK(n == 1 + R8F_TEXTO_MAX, "texto que nunca completa: no maximo R8F_TEXTO_MAX repinturas depois da mudanca"); }

  // ABERTURA COM A HOME SEM ASSENTAR (destaque esperando arte): o menu nao
  // fica esperando o teto de 1,5 s; pinta em R8F_ESPERA_QD quadros.
  { R8Fundo b = {0};
    OK(D(b, 1, 4, 1, 0, 0, t) == R8F_ESPERA, "abertura, Home sem assentar: espera");
    OK(D(b, 1, 4, 1, 0, 0, t + 16) == R8F_ESPERA, "1 quadro");
    OK(D(b, 1, 4, 1, 0, 0, t + 32) == R8F_PINTAR, "no 2o quadro pinta assim mesmo");
    r8fundo_pintou(&b, 0, 0, 0, 0, 0);
    OK(D(b, 1, 4, 1, 1, 0, t + 400) == R8F_PINTAR && !strcmp(b.motivo, "assentou"), "e refaz quando a Home assentar"); }

  // COPIA PRETA (TCL): Home viva ate a proxima abertura.
  { R8Fundo p = {0};
    D(p, 1, 5, 1, 1, 0, t); D(p, 1, 5, 1, 1, 0, t);
    r8fundo_pintou(&p, 0, 0, 1, 1, 0);
    OK(D(p, 1, 5, 1, 1, 0, t + 16) == R8F_DIRETO, "copia preta: Home viva");
    OK(D(p, 1, 5, 2, 1, 0, t + 32) == R8F_DIRETO, "copia preta: continua viva mesmo com mudanca");
    OK(D(p, 1, 6, 2, 1, 0, t + 48) != R8F_DIRETO, "nova abertura: tenta de novo"); }

  // VIDEO NO AR (parar 0) e de volta: refaz a copia.
  { R8Fundo v = {0};
    D(v, 1, 7, 1, 1, 0, t); D(v, 1, 7, 1, 1, 0, t);
    r8fundo_pintou(&v, 0, 0, 0, 1, 0);
    OK(D(v, 0, 7, 1, 1, 0, t + 16) == R8F_DIRETO, "video no ar: Home viva");
    OK(D(v, 1, 7, 1, 1, 0, t + 32) == R8F_ESPERA && pediu, "voltou: pede o alvo (Home viva enquanto isso)");
    OK(D(v, 1, 7, 1, 1, 0, t + 48) == R8F_PINTAR && !strcmp(v.motivo, "retomada"), "e refaz (retomada)"); }

  printf("r8fundo: %d falhas\n", falhas);
  return falhas ? 1 : 0;
}
