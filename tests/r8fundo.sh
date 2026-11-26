#!/bin/bash
# A copia parada do menu de segurar OK: quando pintar (r8fundo.h), sem GL.
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
if [ "$(uname)" = Darwin ]; then
  cc tests/r8fundo.c -Isrc -o "$TMPDIR/nuvio-r8fundo" -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
else
  cc tests/r8fundo.c -Isrc -o "$TMPDIR/nuvio-r8fundo" -O1 -g $(sdl2-config --cflags)
fi
"$TMPDIR/nuvio-r8fundo"
