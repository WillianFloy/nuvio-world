#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-herotexto; mkdir -p "$D"
cc -std=c11 -Wall -Wextra -Werror tests/herotexto.c -o "$D/herotexto" -lm
"$D/herotexto"
