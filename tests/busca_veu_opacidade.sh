#!/usr/bin/env bash
# A arte do item focado (24/11): comportamento com o home.c de verdade (GLES2 por software).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/busca_veu_opacidade.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/busca_veu_opacidade.c -o "$D/busca_veu_opacidade" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 900 "$D/busca_veu_opacidade" 2>&1 | grep -E "^(PASS|FAIL|busca_veu_opacidade|\[veu\]|\[menu\])|Assertion"
