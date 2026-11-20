// MENU (World, 20/11), com o ajustes.c de verdade:
//   1. Posicao "Na lateral": so opcoes da barra lateral; "No topo": so as do topo.
//   2. Nomes seguem a posicao ("Explorar no Menu do Topo"; apresentacao e ordem
//      dizem "barra lateral" na lateral).
//   3. Barra moderna ligada: sem "Recolhida/Fixa" e sem apresentacao (a coluna World).
//   4. Ordem padrao nova (Busca, Inicio, Explorar, Biblioteca, Guia, Agenda, Perfil,
//      Ajustes); a ordem de fabrica antiga le como a nova.
//   5. Layout Dinamica: fora do seletor e migrado para Moderna.
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
  ajustes_dir(d);   // gravar() precisa da pasta
  memcpy(valor, valorPadrao, sizeof valor);
  montarTela();
  // 1/3. Lateral, barra moderna desligada (padrao).
  valor[AJ_MENU_TOPO] = 0; valor[AJ_RAIL_MODERNA] = 1;
  OK(visivelOp(AJ_RAIL_MODERNA) == 1 && visivelOp(AJ_RAIL) == 1 && visivelOp(AJ_MENU_TOPO_MODO) == 1,
     "lateral sem moderna: barra moderna, recolhida/fixa e apresentacao a vista");
  OK(visivelOp(AJ_MENU_TOPO_TAMANHO) == 0 && visivelOp(AJ_MENU_TOPO_FIXO) == 0 && visivelOp(AJ_MENU_TOPO_FOCO_EXPANDIDO) == 0,
     "lateral: tamanho, fixo e foco expandido do topo escondidos");
  OK(visivelOp(AJ_RAIL_BLUR) == 0, "desfoque da barra moderna (sem efeito) escondido");
  valor[AJ_RAIL_MODERNA] = 0;
  OK(visivelOp(AJ_RAIL) == 0 && visivelOp(AJ_MENU_TOPO_MODO) == 0,
     "lateral com moderna: sem recolhida/fixa e sem apresentacao (coluna World)");
  OK(!strcmp(rotuloOpcao(AJ_MENU_TOPO_MODO), "Apresentação da barra lateral") &&
     !strcmp(rotuloOpcao(AJ_MENU_TOPO_ORDEM), "Ordem da barra lateral"),
     "lateral: apresentacao e ordem com o nome da barra lateral");
  // 1/2. Topo.
  valor[AJ_MENU_TOPO] = 1;
  if (ajustes_menu_topo()) {
    OK(visivelOp(AJ_RAIL_MODERNA) == 0 && visivelOp(AJ_RAIL) == 0, "topo: opcoes da barra lateral escondidas");
    OK(visivelOp(AJ_MENU_TOPO_TAMANHO) == 1 && visivelOp(AJ_MENU_TOPO_ORDEM) == 1, "topo: opcoes do menu superior a vista");
    OK(!strcmp(rotuloOpcao(AJ_MENU_EXPLORAR), "Explorar no Menu do Topo") &&
       !strcmp(rotuloOpcao(AJ_MENU_PERFIL), "Perfil e Stats no Menu do Topo"),
       "topo: \"Explorar no Menu do Topo\", \"Perfil e Stats no Menu do Topo\"");
    OK(strstr(ajudaOpcaoBase(AJ_MENU_EXPLORAR), "menu superior") != NULL, "topo: a ajuda do item fala do menu superior");
  } else OK(0, "topo nao ativou (retrato/telefone no teste?)");
  valor[AJ_MENU_TOPO] = 0;
  OK(!strcmp(rotuloOpcao(AJ_MENU_EXPLORAR), OPCOES[AJ_MENU_EXPLORAR].rotulo), "lateral: nome original \"na barra lateral\"");
  // 4. Ordem.
  { int o[AJ_MENU_TOPO_ORDEM_N], esp[AJ_MENU_TOPO_ORDEM_N] = { 3, 0, 1, 4, 2, 5, 6, 7 }, igual = 1;
    valor[AJ_MENU_TOPO_ORDEM] = valorPadrao[AJ_MENU_TOPO_ORDEM];
    ajustes_menu_topo_ordem(o);
    for (int i = 0; i < AJ_MENU_TOPO_ORDEM_N; i++) igual &= o[i] == esp[i];
    OK(igual, "ordem padrao: Busca, Inicio, Explorar, Biblioteca, Guia TV, Agenda, Perfil, Ajustes");
    valor[AJ_MENU_TOPO_ORDEM] = 16434824;   // fabrica antiga
    ajustes_menu_topo_ordem(o); igual = 1;
    for (int i = 0; i < AJ_MENU_TOPO_ORDEM_N; i++) igual &= o[i] == esp[i];
    OK(igual, "a ordem de fabrica antiga le como a nova (outros perfis)");
    { int minha[AJ_MENU_TOPO_ORDEM_N] = { 7, 6, 5, 4, 3, 2, 1, 0 }, r[AJ_MENU_TOPO_ORDEM_N];
      OK(ajustes_menu_topo_ordem_definir(minha), "ordem da pessoa grava");
      ajustes_menu_topo_ordem(r); igual = 1;
      for (int i = 0; i < AJ_MENU_TOPO_ORDEM_N; i++) igual &= r[i] == minha[i];
      OK(igual, "ordem escolhida pela pessoa fica"); } }
  // 5. Dinamica.
  OK(OPCOES[AJ_HOME_LAYOUT].n == 2, "seletor de layout so com Moderna e Padrao");
  if (falhas) return 1;
  puts("ajustes_menu_posicao: ok");
  return 0;
}
