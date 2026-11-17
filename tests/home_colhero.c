// 2.0.43 (B): HERO DAS PASTAS das Collections da Home — fundo, logo e informacoes numa so transicao.
// home.c real, GLES2 por software (logica, NAO o desempenho da Mali-G31). Uso:
//   home_colhero [layout 0|1|2] [prefixo-de-capturas]
#include "../src/home.c"
#include "home_colfix.h"
static int descer_ate(int rc) { for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); } frames(120); return foco.fileira == rc; }
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0; const char *pre = argc > 2 ? argv[2] : NULL;
  int rc = colfix_iniciar(layout, 24, 1);
  ok(rc >= 0 && descer_ate(rc), "o foco chegou na fileira de Collections");
  if (rc < 0) { puts("home_colhero: FALHOU"); return 1; }
  if (layout == 2) {
    // DINAMICA: so o banner da pasta; crossfade em 350 ms, sem apagar-e-depois-acender.
    ok(dinColArte[0] && dinColAlfa >= 0.999f, "pasta em foco: banner assentado");
    char antes[512]; snprintf(antes, sizeof antes, "%s", dinColArte);
    key(SDLK_RIGHT);
    int cruzou = 0, semBuraco = 1, tFim = -1; float aAnt = 1.0f;
    for (int q = 0; q < 60; q++) {
      frames(1);
      if (dinColAnt[0] && dinColAlfa < 0.999f) cruzou = 1;
      // nunca o banner antigo sai antes de o novo cobrir: sem apagar a tela entre os dois.
      if (!dinColAnt[0] && dinColAlfa < 0.999f && strcmp(dinColArte, antes)) semBuraco = 0;
      if (tFim < 0 && dinColAlfa >= 0.999f && strcmp(dinColArte, antes)) tFim = (q + 1) * 16;
      (void)aAnt;
    }
    ok(cruzou, "Dinamica: troca de pasta e um crossfade (o antigo fica por baixo enquanto o novo acende)");
    ok(semBuraco, "Dinamica: nunca fica sem banner entre as duas pastas");
    ok(tFim > 0 && tFim <= 700, "Dinamica: a troca completa em ~350 ms depois de o banner estar pronto");
    ok(strcmp(dinColArte, antes) != 0, "Dinamica: o banner novo assentou");
    puts(falhas ? "home_colhero: FALHOU" : "home_colhero: ok"); tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0;
  }
  // ----- Moderna e Padrao -----
  // Aquece o cache (banner e logo de cada pasta decodificados) e volta ao inicio.
  for (int k = 0; k < 8; k++) { key(SDLK_RIGHT); frames(45); }
  for (int k = 0; k < 8; k++) { key(SDLK_LEFT); frames(45); }
  frames(60);
  ok(colCamN == 1 && colCam[0].t >= NV_HERO_ARTE_FADE_MS, "pasta em foco: uma camada assentada");
  int pasta0 = colCam[0].pasta;
  // 1. Troca normal (banner e logo prontos): uma transicao so, fundo + logo + texto coordenados.
  int r0 = txt_rasterizadas;
  key(SDLK_RIGHT);
  int prontoSemTroca = 0, nCamMax = colCamN, quadroSwap = -1, rasterTardio = 0, maxRaster = 0, camadaZeroPerdida = 0, fimQ = -1, ambosCheios = 0, noMeio = 0;
  for (int q = 0; q < 100; q++) {
    int a0 = txt_rasterizadas; frames(1); int d = txt_rasterizadas - a0;
    if (colCamN >= 2 && quadroSwap < 0) quadroSwap = q;
    if (colCamN == 1 && quadroSwap < 0) { const ColFolder *fd = col_folder(fileiras[foco.fileira].folders[foco.coluna]); if (fd && pastaPronta(fd) && q > 0) prontoSemTroca++; }
    if (colCamN > nCamMax) nCamMax = colCamN;
    if (d > maxRaster) maxRaster = d;
    if (quadroSwap >= 0 && q > quadroSwap + 3 && d > 0) rasterTardio++;
    if (colCamN >= 2) {
      noMeio++;
      if (colCam[0].t < NV_HERO_ARTE_FADE_MS) camadaZeroPerdida = 1;   // a de baixo e sempre a inteira: sem corte
      // o texto antigo e o novo nunca estao ambos acesos (>0.9) num mesmo elemento
      ColCam *ant = &colCam[0], *nov = &colCam[colCamN - 1];
      for (int k = 0; k < HT_N; k++) {
        float aa = ant->coberta ? ht_saida_alfa(&ant->snap, k, ant->tCob) : 1.0f, an = ht_entra_alfa(k, nov->t);
        if (aa > 0.9f && an > 0.9f) ambosCheios = 1;
      }
      if (pre && (colCam[colCamN-1].t > 60 && colCam[colCamN-1].t < 78)) shot(pre, "t060");
      if (pre && (colCam[colCamN-1].t > 150 && colCam[colCamN-1].t < 166)) shot(pre, "t150");
      if (pre && (colCam[colCamN-1].t > 250 && colCam[colCamN-1].t < 266)) shot(pre, "t250");
    }
    if (quadroSwap >= 0 && colCamN == 1 && fimQ < 0) fimQ = q;
  }
  ok(quadroSwap >= 0 && quadroSwap <= 4 && prontoSemTroca <= 1, "a troca comeca assim que banner e logo estao prontos (sem repouso extra)");
  ok(nCamMax == 2, "uma camada nova por troca (fundo + logo + texto juntos)");
  ok(fimQ >= 0 && (fimQ - quadroSwap) * 16 >= 330 && (fimQ - quadroSwap) * 16 <= 420, "a transicao dura ~350 ms");
  ok(!camadaZeroPerdida, "a camada de baixo fica inteira ate a de cima cobrir (sem corte)");
  ok(!ambosCheios, "texto/logo antigo e novo nunca estao os dois cheios no mesmo elemento");
  ok(rasterTardio == 0, "nenhuma linha de texto rasterizada depois dos primeiros quadros da troca");
  printf("[info] rasterizadas=%d maximo por quadro=%d camadas no ar=%d quadros\n", txt_rasterizadas - r0, maxRaster, noMeio);
  ok(colCam[0].pasta != pasta0, "a pasta nova assentou");
  // 2. Interrupcao: duas trocas seguidas empilham (3 camadas) e a de baixo continua inteira: nada pula.
  int camMax = 1, pop = 0;
  key(SDLK_RIGHT); frames(3); key(SDLK_RIGHT);
  for (int q = 0; q < 80; q++) { frames(1); if (colCamN > camMax) camMax = colCamN; if (colCamN >= 2 && colCam[0].t < NV_HERO_ARTE_FADE_MS) pop = 1; }
  ok(camMax >= 3, "trocas seguidas empilham as camadas");
  ok(!pop && colCamN == 1, "interrupcao: camada de baixo sempre inteira e tudo assenta numa so");
  // 3. Banner que FALHA de vez (pasta 5 sem arquivo): o anterior segue inteiro ate a falha ser conhecida e a troca
  //    NAO espera os 600 ms por uma URL morta (2.0.44). O limite de 600 ms continua para o que so demora (guarda no .sh).
  while (foco.coluna > 3) { key(SDLK_LEFT); frames(25); } frames(60);
  while (foco.coluna < 4) { key(SDLK_RIGHT); frames(25); } frames(60);
  int antes5 = colCam[colCamN - 1].pasta; Uint32 t0 = now;
  key(SDLK_RIGHT);   // coluna 5: banner inexistente
  int trocou = -1, manteve = 1;
  for (int q = 0; q < 80; q++) { frames(1); if (colCam[colCamN-1].pasta != antes5) { trocou = (int)(now - t0); break; } if (colCamN != 1 || colCam[0].pasta != antes5) manteve = 0; }
  ok(manteve, "enquanto o banner nao chega, o destaque anterior fica inteiro");
  ok(trocou >= 0 && trocou < (int)NV_HERO_ESPERA_MS - 100, "banner que falhou de vez: a troca nao espera os 600 ms por uma URL morta");
  frames(60);
  // 4. Efeitos minimos / animacoes reduzidas: troca instantanea (uma camada, sem fade).
  gfx_definir_efeitos_minimos(1);
  key(SDLK_RIGHT); int inst = 1; for (int q = 0; q < 30; q++) { frames(1); if (colCamN != 1) inst = 0; }
  ok(inst, "efeitos minimos: troca sem camadas (instantanea)");
  gfx_definir_efeitos_minimos(0);
  // 5. Sair da fileira zera a camada; voltar entra direto, sem animacao.
  key(SDLK_UP); frames(100);
  ok(colCamN == 0, "fora da fileira de pastas a camada zera");
  puts(falhas ? "home_colhero: FALHOU" : "home_colhero: ok");
  tex_encerrar(); SDL_Quit(); return falhas ? 1 : 0; }
