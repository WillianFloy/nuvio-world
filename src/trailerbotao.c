#include "trailerbotao.h"
#include "extras.h"
#include "trailerapple.h"
#include <string.h>

static int mesmaLingua(int i, const char *iso) {
  const char *l = extras_trailer_lingua(i);
  return iso[0] && l[0] && (l[0] | 32) == (iso[0] | 32) && (l[1] | 32) == (iso[1] | 32);
}

int trailerbotao_escolher(int k) {
  int i, n = extras_n_trailers();
  char iso[8] = "";
  if (k > 0 && k < n && extras_trailer_yt(k)[0]) return k;
  for (i = 0; i < n; i++) if (extras_trailer_yt(i)[0] && extras_trailer_idioma_preferido(i) > 0) return i;
  extras_trailer_audio_iso(iso);
  for (i = 0; i < n; i++) if (extras_trailer_yt(i)[0] && extras_trailer_legendado(i) && mesmaLingua(i, iso)) return i;
  return n > 0 && extras_trailer_yt(0)[0] ? 0 : -1;
}

int trailerbotao_prioridade(int kk, const char *imdb, const char *apple, int appleRespondeu) {
  char iso[8] = "";
  int p;
  if (kk < 0) return extras_trailer_idioma_preferido(-1) < 0 ? -1 : 0;
  p = extras_trailer_idioma_preferido(kk);
  if (p) return p;
  extras_trailer_audio_iso(iso);
  if (!iso[0] || !strncmp(iso, "en", 2)) return 0;
  if (extras_trailer_legendado(kk) && mesmaLingua(kk, iso)) return 1;
  if (apple && apple[0] && trailerapple_tem_legenda(imdb, iso)) return 0;
  if (!appleRespondeu) return -1;
  return 1;
}
