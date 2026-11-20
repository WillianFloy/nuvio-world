// NOME NA ESPERA DA LOGO (19/11), com o home.c de verdade (heroLogoCruza):
//   1. logo que chega dentro de 150 ms entra direto (o nome nao pisca);
//   2. logo atrasada: o nome aparece a partir de 150 ms, no mesmo lugar;
//   3. logo chegando: cruza em 250 ms (nome 1->0, logo 0->1, soma ~1), sem quadro vazio;
//   4. depois do cruzamento, so a logo.
#include "../src/home.c"
#include <assert.h>
#include <stdio.h>
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  float aT, aL;
  // 1. rapida
  aL = heroLogoCruza("https://x/logo-rapida.png", 0, &aT);
  SDL_Delay(60);
  aL = heroLogoCruza("https://x/logo-rapida.png", 0, &aT);
  OK(aT == 0.0f && aL == 0.0f, "logo pendente ha 60 ms: nada ainda (sem piscar o nome)");
  aL = heroLogoCruza("https://x/logo-rapida.png", 7, &aT);
  OK(aT == 0.0f && aL == 1.0f, "logo que chega dentro de 150 ms entra direto");
  // 2. atrasada
  aL = heroLogoCruza("https://x/logo-lenta.png", 0, &aT);
  SDL_Delay(170);
  aL = heroLogoCruza("https://x/logo-lenta.png", 0, &aT);
  OK(aT == 1.0f && aL == 0.0f, "logo atrasada (170 ms): o nome em texto ja esta na tela");
  // 3. cruzamento
  int vazio = 0, quadros = 0; float somaMin = 2.0f;
  for (int t = 0; t < 40; t++) {
    aL = heroLogoCruza("https://x/logo-lenta.png", 7, &aT);
    if (aT + aL < 0.98f) vazio++;
    if (aT + aL < somaMin) somaMin = aT + aL;
    quadros++;
    if (aT == 0.0f && aL == 1.0f) break;
    SDL_Delay(16);
  }
  printf("  cruzamento: %d quadros, soma minima %.3f\n", quadros, somaMin);
  OK(quadros >= 10 && quadros <= 24, "a troca nome -> logo dura ~250 ms (nao e corte seco)");
  OK(vazio == 0, "nenhum quadro do cruzamento fica sem titulo (nome + logo >= 98%)");
  aL = heroLogoCruza("https://x/logo-lenta.png", 7, &aT);
  OK(aT == 0.0f && aL == 1.0f, "depois do cruzamento, so a logo");
  // 4. outra URL nao herda estado
  aL = heroLogoCruza("https://x/outra.png", 0, &aT);
  OK(aT == 0.0f, "titulo novo comeca do zero (tolerancia de novo)");
  if (falhas) return 1;
  puts("hero_logo_espera: ok");
  return 0;
}
