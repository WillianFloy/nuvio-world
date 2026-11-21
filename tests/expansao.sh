#!/usr/bin/env bash
# Expansao do cartaz em repouso: entrada e saida animadas (expansao.h).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-expansao
mkdir -p "$D"
cc -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/expansao.c -lm -o "$D/t"
"$D/t"
