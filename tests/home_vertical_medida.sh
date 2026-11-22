#!/usr/bin/env bash
# World 22/11: o botao "Ver titulo" do destaque (Padrao) so existe com o foco NO destaque. Teste de comportamento com o
# home.c de verdade (GLES2 por software): estado + pixels. Nao prova nada sobre a TV.
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
cc $FLAGS "$D"/*.o tests/home_vertical_medida.c -o "$D/home_vertical_medida" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
OUT=$(SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 900 "$D/home_vertical_medida" 2>&1 | grep -vE "^\[(tex|hero|gfx|diag)" || true)
echo "$OUT"
! echo "$OUT" | grep -q "FAIL\|FALHOU\|Assertion" && echo "$OUT" | grep -q "home_vertical_medida: ok"
