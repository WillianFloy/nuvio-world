#!/bin/bash
# Mola de rolagem em tempo real (independente do FPS).
set -eu
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/animtempo.c -lm -o "$out/t"
"$out/t"
