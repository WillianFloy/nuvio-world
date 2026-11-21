#!/usr/bin/env bash
# GIF das Collections: Automatico (RAM) | Ligado | Desligado (src/gif.c, World 21/11).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-gifmodo; mkdir -p "$D"
cc -std=gnu11 -O1 -g -Wall -DNV_ANDROID -Isrc src/gif.c tests/gif_modo.c tests/gif_modo_gl.c -pthread -lm -o "$D/t"
for gb in 0.9 1.5 8; do NV_GIF_MEM_GB=$gb "$D/t"; done
echo "gif_modo: ok"
