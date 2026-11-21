#!/usr/bin/env bash
# Automatica -> 720p so quando a TV nao segura 1080p (src/resolucao.h). Puro, sem SDL.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-res720; mkdir -p "$D"
cc -std=gnu11 -O1 -g -Wall -Wextra -Isrc tests/resolucao_720.c -o "$D/t"
"$D/t"
