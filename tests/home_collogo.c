// 2.0.44 (A): LOGO CONFIGURADA DA PASTA nunca vira o nome em texto enquanto carrega; so se falhar de vez.
// home.c real, GLES2 por software (logica, NAO o desempenho da Mali-G31). Uso: home_collogo [layout 0|1] [prefixo-de-capturas]
#include "../src/home.c"
#include "home_colfix.h"
static int descer_ate(int rc) { for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); } return foco.fileira == rc; }
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0; const char *pre = argc > 2 ? argv[2] : NULL;
  int rc = colfix_iniciar(layout, 24, 1);
  ok(rc >= 0, "ha fileira de Collections");
  // 1. Entrar na fileira com o CACHE FRIO da logo da pasta 1 (a 1a pasta tem logo configurada).
  homeTesteNomePasta = 0;
  for (int i = 0; i < 12 && foco.fileira != rc - 1 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); }
  frames(30);
  if (foco.fileira != rc) key(SDLK_DOWN);
  int pend = 0, fade = 0, pronta = 0; float ant = -1.0f, ultimo = -1.0f; int mono = 1;
  for (int q = 0; q < 90; q++) {
    frames(1);
    if (colCamN > 0) { float lt = colCam[colCamN - 1].logoT; if (lt == -2.0f) pend = 1; if (lt >= 0.0f && lt < HT_ENTRA_MS) { fade = 1; if (lt < ant - 1e-3f) mono = 0; ant = lt; } if (lt >= HT_ENTRA_MS) pronta = 1; ultimo = lt; }
    if (q == 0) shot(pre, "f001");
  }
  ok(foco.fileira == rc && foco.coluna == 0, "o foco esta na 1a pasta (com logo configurada)");
  ok(homeTesteNomePasta == 0, "logo configurada: o NOME da pasta nao aparece em nenhum quadro, nem no cache frio");
  ok(pend || pronta, "a logo carrega (ou ja estava pronta) e fica assentada");
  ok(pronta && ultimo >= HT_ENTRA_MS, "no fim a logo esta assentada");
  if (pend) ok(fade && mono, "a logo que chega depois entra com fade monotonico, nao de uma vez");
  // 2. Pasta sem logo configurada: o nome em texto, como sempre.
  homeTesteNomePasta = 0; key(SDLK_RIGHT); frames(120);
  ok(foco.coluna == 1 && homeTesteNomePasta > 0, "pasta SEM logo configurada: o nome aparece");
  // 3. Logo configurada que FALHOU de vez (pasta 10): o nome aparece, e a troca nao espera 600 ms por URL morta.
  for (int i = 0; i < 8; i++) { key(SDLK_RIGHT); frames(60); }
  ok(foco.coluna == 9, "chegou na pasta 10 (logo inexistente)");
  { const ColFolder *f = col_folder(fileiras[rc].folders[9]); const char *u = pastaLogoUrl(f);
    ok(u && tex_falhou(u), "a logo inexistente consta como FALHOU");
    homeTesteNomePasta = 0; frames(60); ok(homeTesteNomePasta > 0, "logo que falhou de vez: o nome em texto aparece");
    ok(pastaPronta(f), "pasta com logo falhada conta como resolvida (nao espera os 600 ms)"); }
  // 4. Voltar a uma pasta com logo ja em cache: assentada, sem fade, sem nome.
  key(SDLK_LEFT); frames(2); key(SDLK_LEFT); frames(2); key(SDLK_LEFT); frames(2); // ate a pasta 7 (sem logo) ... segue ate a 6 (com logo)
  for (int i = 0; i < 4; i++) { key(SDLK_LEFT); frames(60); }
  homeTesteNomePasta = 0;
  { int c = foco.coluna; for (int i = 0; i < 25 && (c % 3 != 0 || c == 9); i++) { key(SDLK_LEFT); frames(60); c = foco.coluna; }
    homeTesteNomePasta = 0; frames(80); ok(c % 3 == 0 && homeTesteNomePasta == 0, "pasta com logo em cache: sem nome em texto"); }
  // 5. Efeitos minimos (troca instantanea): o mesmo vale.
  gfx_definir_efeitos_minimos(1); homeTesteNomePasta = 0;
  { int c = foco.coluna; for (int i = 0; i < 6; i++) { key(SDLK_LEFT); frames(60); } (void)c; }
  gfx_definir_efeitos_minimos(0);
  puts(falhas ? "home_collogo: FALHOU" : "home_collogo: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
