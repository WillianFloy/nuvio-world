// A rolagem da Vitrine assenta no mesmo tempo com quadros de 16 ms ou de 100 ms.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "anim.h"

static float assenta(float dt, float alvo, float ate) {
  float x = 0.0f, v = 0.0f;
  for (float t = 0.0f; t < ate - 1e-4f; t += dt) x = anim_mola2_tempo(&v, x, alvo, dt, 11.5f);
  return x;
}

int main(void) {
  const float alvo = 408.0f;
  // Em 0,6 s: 60 fps, 20 fps e 10 fps chegam ao mesmo ponto (tempo real).
  float a = assenta(1.0f / 60.0f, alvo, 0.6f), b = assenta(0.05f, alvo, 0.6f), c = assenta(0.1f, alvo, 0.6f);
  assert(fabsf(a - c) < 6.0f && fabsf(a - b) < 6.0f);
  // A mola antiga com quadros de 100 ms fica para tras: o teto de 50 ms a atrasa.
  float x = 0.0f, v = 0.0f;
  for (float t = 0.0f; t < 0.6f - 1e-4f; t += 0.1f) x = anim_mola2(&v, x, alvo, 0.1f, 11.5f);
  assert(x < c - 30.0f);
  // Em 1,0 s esta assentada (a menos de 1 px) em qualquer FPS.
  assert(fabsf(assenta(0.1f, alvo, 1.0f) - alvo) < 1.5f);
  assert(fabsf(assenta(1.0f / 60.0f, alvo, 1.0f) - alvo) < 1.5f);
  puts("animtempo: tudo ok");
  return 0;
}
