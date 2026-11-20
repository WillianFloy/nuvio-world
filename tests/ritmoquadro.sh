#!/bin/bash
# Frame time / frame pacing / latencia / cadencia (ritmoquadro.h), simulado.
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-ritmoquadro"
cc -std=c99 -O1 tests/ritmoquadro.c -o "$out" -lm
"$out"
