#!/bin/bash
# World (22/11): a Apple busca pelo titulo em ingles do TMDB. Sem rede (respostas falsas).
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailerapple-ingles"
cc -O1 -g -Wall -Wno-unused-function -Isrc $(sdl2-config --cflags 2>/dev/null || echo -I/opt/homebrew/include -I/opt/homebrew/include/SDL2) \
  src/trailerapple.c src/trailertt.c src/js.c tests/trailerapple_ingles.c \
  $(sdl2-config --libs 2>/dev/null || echo -L/opt/homebrew/lib -lSDL2) -lpthread -lm -o "$B"
"$B"
