#!/bin/bash
# Ritmo do vidro decidido pela escada de efeitos (gpunivel.c), pelo caminho de
# producao (gpun_medir / gpun_relatar / gpun_quadro_inicio). Ver tests/gpun_vidro.c.
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-gpun-vidro"
if [ "$(uname)" = Darwin ]; then
  gl=(-framework OpenGL -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
else
  gl=($(sdl2-config --cflags) -lGLESv2)
fi
cc src/gpunivel.c src/perfiltv.c tests/gpun_vidro.c -Isrc -o "$out" -DNV_GPUN_TESTE -DNV_WEBOS \
  -O1 -g -Wall -Wextra -Wno-deprecated-declarations "${gl[@]}" -lm
"$out"
