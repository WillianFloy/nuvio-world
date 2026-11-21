#!/bin/bash
# World sem vidro (NV_SEM_VIDRO): opcao escondida, vidro sempre desligado.
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
out="${TMPDIR:-/tmp}/nuvio-ajustes-menu-posicao"
dados=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-sem-vidro.XXXXXX")
trap 'rm -rf "$dados"' EXIT
if [ "$(uname)" = Darwin ]; then
  cc "${sources[@]}" tests/sem_vidro.c -DNV_SEM_VIDRO -Isrc -o "$out" -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -w
else
  cc "${sources[@]}" tests/sem_vidro.c -DNV_SEM_VIDRO -Isrc -o "$out" -O1 $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf) \
    -lz -lGL -lm -lpthread -ldl -w
fi
NUVIO_DADOS="$dados" "$out"
