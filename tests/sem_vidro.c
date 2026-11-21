// SEM VIDRO NO WORLD (20/11/2026), com o ajustes.c de verdade compilado com
// NV_SEM_VIDRO (o que o Android liga sozinho):
//   1. vidro salvo LIGADO (ou automatico) nao liga o vidro: ajustes_vidro() = 0;
//   2. a opcao e as que dependem dela somem da tela e do "abrir ajuste";
//   3. o valor salvo fica intocado (o codigo do vidro so sai depois da TV);
//   4. a escada de efeitos nao ve mais os degraus do vidro como automaticos;
//   5. o resto (fosco automatico, animacoes) continua.
#include "../src/ajustes.c"
#include <assert.h>
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
static int visivelOp(int op) {
  for (int i = 0; i < AJ_N_TELA; i++) if (TELA[i].tipo == IT_OPC && TELA[i].op == op) return visivel(i);
  return -1;
}
int main(void) {
  const char *d = getenv("NUVIO_DADOS");
  assert(d && *d);
  dados_iniciar(d);
  ajustes_dir(d);
  memcpy(valor, valorPadrao, sizeof valor);
  montarTela();
  valor[AJ_VIDRO] = 0;   // V_LIGA: 0 = ligado
  OK(!ajustes_vidro(), "vidro salvo ligado: o app desenha sem vidro");
  valor[AJ_VIDRO] = 2;   // automatico
  OK(!ajustes_vidro(), "vidro automatico: tambem sem vidro");
  OK(valor[AJ_VIDRO] == 2, "o valor salvo nao e reescrito");
  { int ops[] = { AJ_VIDRO, AJ_VIDRO_CONTORNO, AJ_FOCO_ACENTO, AJ_VIDRO_OPAC, AJ_VIDRO_FOSCO, AJ_VIDRO_QUALIDADE };
    int k, todas = 1;
    for (k = 0; k < (int)(sizeof ops / sizeof *ops); k++) todas &= visivelOp(ops[k]) != 1 && !ajOpcaoDisponivel(ops[k]);
    OK(todas, "vidro, contorno, acento do foco, opacidade, fosco e qualidade do vidro fora da tela"); }
  uxAbrirOp = -1;
  ajustes_abrir_opcao(AJ_VIDRO);
  OK(uxAbrirOp != AJ_VIDRO, "abrir ajuste (guia/busca) nao abre a opcao escondida");
  valor[AJ_VIDRO_QUALIDADE] = 0; valor[AJ_VIDRO_FOSCO] = 2; valor[AJ_ANIM] = 2;
  { int e = ajustes_efeitos_automaticos();
    OK(!(e & 1) && !(e & 8), "escada: degraus do vidro nao contam como automaticos");
    OK((e & 2) && (e & 4), "escada: fosco e animacoes continuam automaticos"); }
  OK(visivelOp(AJ_TEMA) == 1, "o resto de Aparencia continua a vista (cor de destaque)");
  printf("%s\n", falhas ? "sem_vidro: FALHOU" : "sem_vidro: tudo ok");
  return falhas != 0;
}
