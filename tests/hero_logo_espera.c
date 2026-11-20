// LOGO DO DESTAQUE (World, 20/11), com o home.c de verdade (heroLogoEntra):
//   1. logo pronta na primeira vez que a cena a pede: entra inteira (1);
//   2. logo a caminho: 0 (espaco vazio) por quanto tempo for -- NUNCA o nome no
//      lugar por demora (o substituto de 150 ms da etapa 3 saiu);
//   3. logo que chega atrasada entra em ~250 ms, crescente, sem salto;
//   4. outro titulo comeca do zero.
#include "../src/home.c"
#include <assert.h>
#include <stdio.h>
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  OK(heroLogoEntra("https://x/pronta.png", 7) == 1.0f, "logo ja pronta: entra inteira, sem rampa");
  float a = 1.0f;
  for (int t = 0; t < 30; t++) { a = heroLogoEntra("https://x/lenta.png", 0); SDL_Delay(10); }
  OK(a == 0.0f, "logo a caminho ha 300 ms: area vazia (sem nome no lugar)");
  int q = 0, mono = 1; float ant = -1.0f, v = 0.0f;
  for (int t = 0; t < 60; t++) {
    v = heroLogoEntra("https://x/lenta.png", 7); q++;
    if (v < ant - 1e-6f) mono = 0;
    ant = v;
    if (v >= 1.0f) break;
    SDL_Delay(16);
  }
  printf("  entrada: %d quadros\n", q);
  OK(q >= 10 && q <= 24 && mono, "logo atrasada entra em ~250 ms, sempre crescente");
  OK(heroLogoEntra("https://x/lenta.png", 7) == 1.0f, "depois, inteira");
  OK(heroLogoEntra("https://x/outra.png", 0) == 0.0f, "titulo novo comeca do zero");
  // Guardas de codigo: o substituto textual saiu e a cena espera a logo.
  { FILE *f = fopen("src/home.c", "rb"); static char buf[1 << 23]; size_t n = f ? fread(buf, 1, sizeof buf - 1, f) : 0;
    if (f) fclose(f); buf[n] = 0;
    OK(!strstr(buf, "aNomeEspera") && !strstr(buf, "heroLogoCruza("), "nenhum nome por demora no desenho do destaque");
    OK(strstr(buf, "int home_vitrine_logo_pronta(const CatItem *ci)") != NULL, "Vitrine sabe se a logo esta resolvida"); }
  { FILE *f = fopen("src/vertudo.c", "rb"); static char buf[1 << 23]; size_t n = f ? fread(buf, 1, sizeof buf - 1, f) : 0;
    if (f) fclose(f); buf[n] = 0;
    OK(strstr(buf, "int logoOk = home_vitrine_logo_pronta(it) || parado >= 1200;") && strstr(buf, "home_vitrine_logo_prebuscar(vz)"),
       "troca de cena espera a logo e as logos vizinhas sao pedidas antes"); }
  if (falhas) return 1;
  puts("hero_logo_espera: ok");
  return 0;
}
