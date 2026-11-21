#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
trap 'rm -f "${TMPDIR:-/tmp}/nuvio-miniatura-$$"' EXIT
cc "${sources[@]}" tests/miniatura.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-miniatura-$$" \
  -O1 -g -Wall -Wextra -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
# libjpeg-turbo com ABI 6.2 (a da TV): quando houver uma em NUVIO_JPEG62_DIR,
# o teste cobre o caminho JPEG da miniatura; sem ela, o cru.
if [ -n "${NUVIO_JPEG62_DIR:-}" ]; then export LD_LIBRARY_PATH="$NUVIO_JPEG62_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"; fi
"${TMPDIR:-/tmp}/nuvio-miniatura-$$" "$@"
