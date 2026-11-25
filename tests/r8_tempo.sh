#!/bin/bash
# O modulo de tempo das pecas R8 (src/r8tempo.c) contra a especificacao
# exportada das pranchas aprovadas (tools/r8, 60 fps): ver tests/r8_tempo.c.
#   bash tests/r8_tempo.sh
set -eu
cd "$(dirname "$0")/.."
T="${TMPDIR:-/tmp}"; T="${T%/}"
python3 tests/r8_tempo_ref.py "$T/r8_tempo_ref.txt"
cc -O1 -Wall -Wextra -Isrc tests/r8_tempo.c src/r8tempo.c -lm -o "$T/nuvio-r8-tempo"
"$T/nuvio-r8-tempo" "$T/r8_tempo_ref.txt"
