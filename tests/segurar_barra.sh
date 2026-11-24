#!/bin/bash
# A barrinha de segurar-OK so aparece no segurar, nao no toque.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wno-macro-redefined \
  tests/segurar_barra.c -o /tmp/nuvio-segurar-barra -lSDL2 -L/opt/homebrew/lib
/tmp/nuvio-segurar-barra
