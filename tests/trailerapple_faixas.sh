#!/bin/bash
# World (23/11): master reduzido da Apple com todos os audios e as legendas. trailerapple.c real, sem rede.
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailerapple-faixas"
cc -DNV_ANDROID -O1 -g -w -Isrc $(sdl2-config --cflags 2>/dev/null || echo -I/opt/homebrew/include -I/opt/homebrew/include/SDL2) \
  tests/trailerapple_faixas.c src/trailertt.c src/js.c \
  $(sdl2-config --libs 2>/dev/null || echo -L/opt/homebrew/lib -lSDL2) -lpthread -lm -o "$B"
"$B"
