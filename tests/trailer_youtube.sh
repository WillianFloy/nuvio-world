#!/bin/bash
# World (23/11): trailer do YouTube no botao (Android, WebView). trailer.c real com -DNV_ANDROID; sem GL, sem rede.
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailer-youtube"
cc -DNV_ANDROID -O1 -w -Isrc $(sdl2-config --cflags 2>/dev/null || echo -I/opt/homebrew/include -I/opt/homebrew/include/SDL2) \
  -ffunction-sections -fdata-sections tests/trailer_youtube.c \
  $(sdl2-config --libs 2>/dev/null || echo -L/opt/homebrew/lib -lSDL2) -lm -Wl,--gc-sections -o "$B"
"$B"
