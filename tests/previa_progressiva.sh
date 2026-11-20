#!/bin/bash
# Carregamento progressivo de posteres (tex_cache.c): previa a 128 px do proprio
# arquivo, promocao sem piscar, substituicao no mesmo item. Usa GL real.
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
out="${TMPDIR:-/tmp}/nuvio-previa-progressiva"
if [ "$(uname)" = Darwin ]; then
  cc "${sources[@]}" tests/previa_progressiva.c -Isrc -o "$out" -O1 -g -I/opt/homebrew/include \
    -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
    -Wno-deprecated-declarations -Wno-macro-redefined
else
  cc "${sources[@]}" tests/previa_progressiva.c -Isrc -o "$out" -O1 -g $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf) \
    -lz -lGL -lm -lpthread -ldl -Wno-deprecated-declarations
fi
"$out" | grep -E "^(PASS|FAIL|  )|previa_progressiva"
