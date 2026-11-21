#!/usr/bin/env bash
# Pedagio de imagens (pedagio.h): elegibilidade, URL, degraus e disjuntor.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-pedagio
mkdir -p "$D"
cc -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/pedagio.c src/pedagio.c -lpthread -o "$D/t"
"$D/t"
