#!/bin/bash
# World (22/11): IMDb sem resposta (403) nao vira tempestade de pedidos. Sem rede (rede_baixar_com substituido).
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailerimdb-falha"
cc -O1 -g -Wall -Isrc $(sdl2-config --cflags 2>/dev/null || echo -I/opt/homebrew/include -I/opt/homebrew/include/SDL2) \
  src/trailerimdb.c src/js.c src/perfiltv.c tests/trailerimdb_falha.c $(sdl2-config --libs 2>/dev/null || echo -L/opt/homebrew/lib -lSDL2) -lpthread -o "$B"
"$B"
