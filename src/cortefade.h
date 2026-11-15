// Esvanecer da ultima linha de um texto cortado (sinopse da Vitrine). Pura, para
// o teste (tests/cortefade.c) e o desenho (text.c) usarem a mesma curva.
#ifndef NV_CORTEFADE_H
#define NV_CORTEFADE_H
#define CF_FAIXA_PX 3.0f   // altura de cada faixa recortada, em unidades de layout
// Opacidade (0..1) a `t` (0 = topo da linha, 1 = base): inteira no comeco, some
// na base (smoothstep entre 10% e 95% da altura, onde ficam os tracos dos glifos).
static inline float cortefade_alfa(float t) {
  float u = (t - 0.10f) / 0.85f;
  if (u <= 0.0f) return 1.0f;
  if (u >= 1.0f) return 0.0f;
  return 1.0f - u * u * (3.0f - 2.0f * u);
}
#endif
