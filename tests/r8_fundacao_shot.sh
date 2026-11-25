#!/bin/bash
# Fundacao das pecas R8 (Fase 2) com o desenho de verdade: material Grafite e
# Preto puro, foco, peso 800 (Montserrat ExtraBold), icones r8_* e a copia
# parada recuada/faixa. Ver tests/r8_fundacao_shot.c.
#   bash tests/r8_fundacao_shot.sh [pasta-da-imagem]
set -eu
cd "$(dirname "$0")/.."
T="${TMPDIR:-/tmp}"; T="${T%/}"
work=$(mktemp -d "$T/nuvio-r8-fund.XXXXXX")
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" = src/main.c ] || sources+=("$source"); done
if [ "$(uname)" = Darwin ]; then
  cc "${sources[@]}" tests/r8_fundacao_shot.c -Isrc -o "$work/shot" -O1 -I/opt/homebrew/include \
    -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
    -framework OpenGL -Wno-deprecated-declarations -Wno-macro-redefined
else
  cc "${sources[@]}" tests/r8_fundacao_shot.c -Isrc -o "$work/shot" -O1 -w $(sdl2-config --cflags) \
    -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
  export SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1
fi
NUVIO_DADOS="$work" "$work/shot" "${1:-$work}"
