#!/usr/bin/env bash
# Id do TMDB -> tt do IMDb para os trailers (src/trailertt.c). Rede falsa, sem SDL.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-trailertt; mkdir -p "$D"
cc -std=gnu11 -O1 -g -Wall -Wextra -fsanitize=address,undefined -DNV_TRAILERTT_TESTE -Isrc \
  tests/trailertt.c src/trailertt.c src/js.c -pthread -lm -o "$D/t"
"$D/t"
