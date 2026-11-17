// 2.0.43 (C): ANIMACAO DE ENTRADA DAS COLLECTIONS (abrir uma pasta a partir da Home), home.c + vertudo.c reais.
// Uso: home_colentrada [valor-do-ajuste 0 ligado|1 desligado] [efeitos-minimos 0|1] [prefixo-de-capturas]
// GLES2 por software: logica, NAO o desempenho da Mali-G31.
#include "../src/home.c"
#include "home_colfix.h"
static void frames2(int n) { for (int i = 0; i < n; i++) { now += 16; SDL_PumpEvents(); tex_bombear(16); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  home_atualizar(1.f / 60, now); vertudo_atualizar(1.f / 60, now); home_desenhar(now); vertudo_desenhar(now); gfx_ambiente_descarregar(); } }
int main(int argc, char **argv) {
  int ajuste = argc > 1 ? atoi(argv[1]) : 0, minimos = argc > 2 ? atoi(argv[2]) : 0; const char *pre = argc > 3 ? argv[3] : NULL;
  int rc = colfix_iniciar(1, 12, 1);
  { char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "a"); assert(f);
    fprintf(f, "colEntradaLocal %d\n", ajuste); fclose(f); ajustes_dir(getenv("NUVIO_DADOS")); }
  ok(ajustes_col_entrada() == (ajuste == 0), "o ajuste 'Animacao de entrada das Collections' le o arquivo (ligado por padrao)");
  for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); } frames(90);
  gfx_definir_efeitos_minimos(minimos);
  ok(foco.fileira == rc, "o foco esta na fileira de Collections");
  key(SDLK_RETURN);
  ok(vertudo_aberta(), "OK abre a Collection");
  float t0 = vertudo_entrada_ms(); int animando = t0 < HT_ENT_TOTAL_MS;
  int esperado = (ajuste == 0 && !minimos);
  ok(animando == esperado, esperado ? "ligado: a entrada comeca do zero" : "desligado / efeitos minimos: abre direto (assentada)");
  float ant = -1; int mono = 1, fim = -1, q;
  for (q = 0; q < 80; q++) {
    frames2(1); float t = vertudo_entrada_ms();
    if (animando) {
      if (t < HT_ENT_TOTAL_MS && t < ant - 1e-3f) mono = 0; ant = t;
      if (pre && t > 100 && t < 118) shot(pre, "t100"); if (pre && t > 250 && t < 268) shot(pre, "t250"); if (pre && t > 400 && t < 418) shot(pre, "t400");
      if (t >= HT_ENT_TOTAL_MS && fim < 0) fim = q;
    }
  }
  if (animando) { ok(mono, "o relogio da entrada so avanca"); ok(fim >= 0 && fim * 16 >= 540 && fim * 16 <= 640, "a entrada dura ~570 ms"); }
  ok(vertudo_entrada_ms() >= HT_ENT_TOTAL_MS, "no fim nada fica animando");
  // Fechar e reabrir reinicia a entrada.
  vertudo_fechar_seco(); frames2(20);
  key(SDLK_RETURN); frames2(1);
  ok(!vertudo_aberta() || (vertudo_entrada_ms() < HT_ENT_TOTAL_MS) == esperado, "reabrir reinicia a entrada (quando ligada)");
  puts(falhas ? "home_colentrada: FALHOU" : "home_colentrada: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
