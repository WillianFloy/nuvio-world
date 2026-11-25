#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# O lado "app" do comparador da secao 8: grava os quadros da peca nos instantes
# da prancha. Nao entra na suite (*_shot).
#
#   bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4
#   python3 tools/r8/comparar.py BU /tmp/r8-app /tmp/r8-cmp 0.8 0.95 1.1 1.4
set -eu
cd "$(dirname "$0")/.."
mkdir -p "$2"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-r8-quadros.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/r8_app_quadros_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$NUVIO_DADOS/shot" "$@"
