#!/usr/bin/env bash
# Miniaturas locais (miniatura.c), so o modulo: criterio, ida e volta,
# corrupcao. Roda no CI (sem SDL2_image). O caminho completo e miniatura.sh.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-miniatura-mod
mkdir -p "$D"
cc -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc $(sdl2-config --cflags) \
  tests/miniatura_mod.c src/miniatura.c src/jpegrapido.c $(sdl2-config --libs) -ldl -lpthread -o "$D/t"
if [ -n "${NUVIO_JPEG62_DIR:-}" ]; then export LD_LIBRARY_PATH="$NUVIO_JPEG62_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; fi
"$D/t"
