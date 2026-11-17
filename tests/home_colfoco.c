// 2.0.43 (A): FOCO FIXO nas Collections da Home (grupos do perfil, FILEIRA_CATALOGOS), no home.c real.
// Uso: home_colfoco [layout 0|1|2]. GLES2 por software: logica, NAO o desempenho da Mali-G31.
#include "../src/home.c"
#include "home_colfix.h"
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0;
  int rc = colfix_iniciar(layout, 24, 1);
  ok(rc >= 0, "o grupo de Collections do perfil aparece na Home como fileira de pastas");
  if (rc < 0) { puts("home_colfoco: FALHOU"); return 1; }
  int rn = achar("fixo_0_movie");
  // Desce ate a fileira de Collections.
  for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); } frames(90);
  ok(foco.fileira == rc && !focoHero, "o foco chegou na fileira de Collections");
  ok(homeFocoFixo(rc), "Collections da Home tem foco fixo");
  ok(rn >= 0 && homeFocoFixo(rn), "fileira de cartazes continua com foco fixo");
  ok(home_foco_fixo_ativo(), "home_foco_fixo_ativo vale nas Collections da Home (ritmo de segurar)");
  TXT_INI(); float x0 = homeConteudoX() + bordaX(rc); int N = homeFixoN(rc), total = homeFixoTotal(rc); float passo = passoFil(rc); TXT_FIM();
  ok(N >= 2 && total > N + 3, "a fileira de teste e maior que a tela");
  int fixoOk = 1, tempoOk = 1;
  for (int k = 1; k <= total - N; k++) {
    float s0 = scrollX[rc]; key(SDLK_RIGHT); float a = alvo(rc, foco.coluna); int t90 = -1;
    for (int i = 1; i <= 90; i++) { frames(1); float p = (scrollX[rc] - s0) / (a - s0); if (t90 < 0 && p >= 0.9f) t90 = i * 16; }
    if (fabsf(fx(rc, foco.coluna) - x0) > 0.75f) fixoOk = 0;
    if (t90 < 320 || t90 > 380) tempoOk = 0;
  }
  ok(fixoOk, "o foco fica no mesmo x (o do 1o cartao) ate o fim da janela");
  ok(tempoOk, "a fileira chega a 90% em ~350 ms (320..380 ms)");
  ok(foco.coluna == total - N && fabsf(scrollX[rc] - (float)(total - N) * passo) < 0.75f, "rolagem = coluna x passo");
  for (int k = 0; k < total; k++) { key(SDLK_RIGHT); frames(3); } frames(60);
  ok(foco.coluna == total - 1, "foco chegou ao ultimo cartao");
  ok(fabsf(scrollX[rc] - (float)(total - N) * passo) < 0.75f, "no fim a fileira para no maximo");
  { TXT_INI(); ok(homeFileiraX(rc, total - 1) + larguraFil(rc) <= homeConteudoDireita() + 1.0f, "o ultimo cartao nao passa da margem direita"); TXT_FIM(); }
  for (int k = 0; k < total + 2; k++) { key(SDLK_LEFT); frames(3); } frames(80);
  ok(foco.coluna == 0 && fabsf(scrollX[rc]) < 0.75f && fabsf(fx(rc, 0) - x0) < 0.75f, "no inicio: coluna 0, rolagem 0, 1o cartao alinhado ao titulo");
  // O esvanecer das pontas liga e desliga em par (nao vaza para as fileiras seguintes).
  frames(2);
  // Abrir a Collection ainda funciona (OK abre a pasta).
  key(SDLK_RIGHT); frames(60);
  puts(falhas ? "home_colfoco: FALHOU" : "home_colfoco: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
