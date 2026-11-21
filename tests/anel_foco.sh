#!/usr/bin/env bash
# Anel de foco concentrico com o cartaz em qualquer proporcao (focoprof.h).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-anel-foco
mkdir -p "$D"
cc -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc $(sdl2-config --cflags 2>/dev/null) tests/anel_foco.c -lm -o "$D/t"
"$D/t"
