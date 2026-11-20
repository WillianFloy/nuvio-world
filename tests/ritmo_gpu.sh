#!/bin/bash
# Frame pacing com a GPU no limite: 60 Hz com a Home a ~36 fps (auditoria 20/11,
# P1). Caminho de producao (swapint.h + ritmoquadro.h) alimentado por carimbos.
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-ritmo-gpu"
cc -std=c99 -O1 -Wall -Wextra tests/ritmo_gpu.c -o "$out" -lm
"$out"
