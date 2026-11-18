#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-transicao; mkdir -p "$D"
cc -std=c11 -Wall -Wextra -Werror tests/transicao.c -o "$D/transicao" -lm
"$D/transicao"
