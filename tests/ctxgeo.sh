#!/usr/bin/env bash
# Geometria das formas novas do menu de segurar OK (Folha, Desdobrar): pura, sem GL.
set -euo pipefail
cd "$(dirname "$0")/.."
cc -O1 -Wall -Isrc $(sdl2-config --cflags) tests/ctxgeo.c src/ctxgeo.c -o "${TMPDIR:-/tmp}/ctxgeo" -lm
"${TMPDIR:-/tmp}/ctxgeo"
