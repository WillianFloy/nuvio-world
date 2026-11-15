#!/bin/bash
# Geometria adaptativa das fileiras da Vitrine (cartoes inteiros, janela, recorte).
set -eu
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/vitgeom.c -lm -o "$out/t"
"$out/t"
