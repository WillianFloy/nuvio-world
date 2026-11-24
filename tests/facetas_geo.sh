#!/usr/bin/env bash
# Facetas: posicionamento e curvas (puro, sem GL) — ver tests/facetas_geo.c
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-facetas; mkdir -p "$D"
cc -O1 -Wall -Isrc -Itests $(sdl2-config --cflags) tests/facetas_geo.c src/facetas.c -o "$D/facetas_geo" -lm 2>&1 | grep -v "^$" || true
"$D/facetas_geo"
