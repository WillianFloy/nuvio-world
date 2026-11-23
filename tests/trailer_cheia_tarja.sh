#!/bin/bash
# World (23/11): no Android o trailer em tela cheia vai encaixado (com tarja), sem janela maior que a tela.
# trailer.c real com -DNV_ANDROID e o video simulado; sem GL, sem rede.
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailer-cheia-tarja"
cc -DNV_ANDROID -O1 -w -Isrc $(sdl2-config --cflags 2>/dev/null || echo -I/opt/homebrew/include -I/opt/homebrew/include/SDL2) \
  -ffunction-sections -fdata-sections tests/trailer_cheia_tarja.c \
  $(sdl2-config --libs 2>/dev/null || echo -L/opt/homebrew/lib -lSDL2) -lm -Wl,--gc-sections -o "$B"
"$B"
