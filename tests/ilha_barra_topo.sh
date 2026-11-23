#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# World 23/11: corpo da ilha abaixo da barra do topo. Ver o .c.
#
#   bash tests/ilha_barra_topo.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc -DNV_TOUCH_UI -DNV_TESTE "${sources[@]}" tests/ilha_barra_topo.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-ilha-barra-topo \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-ilha-barra-topo
