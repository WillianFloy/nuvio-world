// Regras puras dos Ajustes World (src/ajwmodelo.h): grade de valores, segurar
// OK, rascunho das setas e rolagem. Comportamento, sem GL.
#include <stdio.h>
#include "ajwmodelo.h"
static int falhas;
#define CHECA(c, m) do { if (!(c)) { falhas++; printf("FALHOU: %s (linha %d)\n", m, __LINE__); } } while (0)
int main(void) {
  // Grade: colunas por quantidade.
  CHECA(ajw_grade_colunas(3) == 1 && ajw_grade_colunas(8) == 2 && ajw_grade_colunas(30) == 3, "colunas");
  // 8 valores em 2 colunas: Baixo pula fileira, laterais ficam na fileira, sem dar a volta.
  CHECA(ajw_grade_mover(0, 8, 2, 0, 1, NULL) == 2, "baixo pula uma fileira");
  CHECA(ajw_grade_mover(1, 8, 2, 1, 0, NULL) == 1, "direita na ultima coluna fica");
  CHECA(ajw_grade_mover(2, 8, 2, -1, 0, NULL) == 2, "esquerda na primeira coluna fica");
  CHECA(ajw_grade_mover(7, 8, 2, 0, 1, NULL) == 7, "baixo na ultima fileira fica");
  CHECA(ajw_grade_mover(0, 8, 2, 0, -1, NULL) == 0, "cima na primeira fileira fica");
  // 7 valores em 2 colunas: Baixo da coluna 2 na penultima fileira cai na ultima celula.
  CHECA(ajw_grade_mover(5, 7, 2, 0, 1, NULL) == 6, "baixo para a fileira incompleta");
  // Indisponiveis sao pulados.
  { unsigned char ok[6] = { 1, 0, 0, 1, 1, 1 };
    CHECA(ajw_grade_mover(0, 6, 1, 0, 1, ok) == 3, "pula indisponiveis para baixo");
    CHECA(ajw_grade_mover(3, 6, 1, 0, -1, ok) == 0, "pula indisponiveis para cima");
    unsigned char so[3] = { 1, 0, 0 };
    CHECA(ajw_grade_mover(0, 3, 1, 0, 1, so) == 0, "sem escolhivel adiante fica"); }
  // Segurar OK.
  { AjwSegurar s = { 0 };
    CHECA(ajw_ok_baixo(&s, 1000, 0) == 0, "sem padrao para voltar: age na hora");
    CHECA(ajw_ok_cima(&s, 1100) == 0, "e o soltar nao age de novo");
    CHECA(ajw_ok_baixo(&s, 2000, 1) == 1, "com padrao: arma");
    CHECA(ajw_ok_tique(&s, 2300) == 0, "antes do tempo nao restaura");
    CHECA(ajw_ok_cima(&s, 2300) == 1, "soltar cedo = OK curto");
    ajw_ok_baixo(&s, 3000, 1);
    CHECA(ajw_ok_tique(&s, 3000 + AJW_SEGURAR_MS) == 1, "segurado restaura");
    CHECA(ajw_ok_tique(&s, 4000) == 0, "restaura uma vez so");
    CHECA(ajw_ok_cima(&s, 4100) == 0, "o soltar depois de restaurar nao age");
    ajw_ok_baixo(&s, 5000, 1);
    CHECA(ajw_ok_cima(&s, 5000 + AJW_SEGURAR_MS + 50) == 2, "longo sem tique: restaura ao soltar"); }
  // Rascunho.
  { AjwRascunho r; ajw_rasc_limpar(&r);
    CHECA(ajw_rasc_passo(&r, 7, 1, 0, 2, 1, 100) == 1 && r.v == 2, "passo a partir do gravado");
    CHECA(ajw_rasc_passo(&r, 7, 1, 0, 2, 1, 150) == 0 && r.v == 2, "limita no fim sem dar a volta");
    CHECA(!ajw_rasc_vencido(&r, 150 + AJW_RASCUNHO_MS - 60), "nao grava antes da pausa");
    CHECA(ajw_rasc_vencido(&r, 100 + AJW_RASCUNHO_MS), "grava depois da pausa");
    CHECA(ajw_rasc_passo(&r, 7, 1, 0, 2, -1, 200) == 1 && r.v == 1 && r.ate == 200 + AJW_RASCUNHO_MS, "novo passo adia a gravacao");
    CHECA(ajw_rasc_passo(&r, 9, 5, 0, 10, 2, 300) == 1 && r.op == 9 && r.v == 7, "outra opcao parte do gravado dela"); }
  // Abas e rolagem: o ativo sempre a vista.
  CHECA(ajw_abas_desloc(800, 1000, 700, 790, 40, 0) == 0, "abas que cabem nao rolam");
  { float d = ajw_abas_desloc(2000, 1000, 1800, 1900, 40, 0);
    CHECA(1900 - d <= 1000 - 40 + 0.01f && d <= 1000, "aba do fim fica inteira");
    d = ajw_abas_desloc(2000, 1000, 10, 100, 40, d);
    CHECA(d == 0, "voltar ao comeco zera"); }
  { float r = ajw_rolagem(2000, 600, 900, 990, 30, 0);
    CHECA(990 - r <= 600 - 30 + 0.01f && 900 - r >= 30 - 0.01f, "item em foco inteiro na janela");
    CHECA(ajw_rolagem(2000, 600, 1950, 2000, 30, 0) == 1400, "nao rola alem do fim");
    CHECA(ajw_rolagem(400, 600, 300, 380, 30, 99) == 0, "lista curta nao rola"); }
  printf(falhas ? "FALHOU: %d\n" : "PASS: regras dos Ajustes World\n", falhas);
  return falhas ? 1 : 0;
}
