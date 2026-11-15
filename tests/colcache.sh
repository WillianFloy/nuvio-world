#!/usr/bin/env bash
# Cache das fileiras das Collections (2.0.34): memoria comprimida + disco.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-colcache-t
mkdir -p "$D"
cc -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/colcache.c src/colcache.c -lpthread -o "$D/t"
"$D/t"
