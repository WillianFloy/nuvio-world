#!/usr/bin/env bash
# B4 (2.0.46): um quadro longo nao e uma ausencia (cascata de entrada das fileiras). home.c de verdade, GLES2 por software;
# nao prova nada sobre a TV. Moderna (0) e Padrao (1).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cc $FLAGS "$D"/*.o tests/home_quadro_longo.c -o "$D/home_quadro_longo" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
RES=0
for L in 0 1; do
  W=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-quadro-longo.XXXXXX")
  OUT=$(NV_LAYOUT=$L SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$W" NUVIO_TESTE_DIR="$W" timeout 900 "$D/home_quadro_longo" 2>&1 | grep -aE "^(PASS|FAIL|controle|quadro longo|Assertion|home_quadro_longo)" || true)
  echo "== layout $L"; echo "$OUT"
  if echo "$OUT" | grep -q "FAIL\|FALHOU\|Assertion" || ! echo "$OUT" | grep -q "home_quadro_longo: ok"; then RES=1; fi
  rm -rf "$W"
done
[ "$RES" = 0 ] && echo "home_quadro_longo: ok (Moderna e Padrao)"
exit $RES
