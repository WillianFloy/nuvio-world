#!/bin/bash
# Regras da fila do tex_cache para a navegacao rapida (sem GL).
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-fila-recencia"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
if [ "$(uname)" = Darwin ]; then
  cc "${sources[@]}" tests/fila_recencia.c -Isrc -o "$out" -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations -w
else
  cc "${sources[@]}" tests/fila_recencia.c -Isrc -o "$out" -O1 $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf) \
    -lz -lGL -lm -lpthread -ldl -w
fi
"$out"
