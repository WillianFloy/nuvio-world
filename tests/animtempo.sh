#!/bin/bash
# Mola de rolagem em tempo real (independente do FPS).
set -eu
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/animtempo.c -lm -o "$out/t"
"$out/t"
# Guarda: a rolagem vertical da Vitrine e por relogio de parede (assenta no ponto
# sob a sinopse em tempo fixo), nao por mola por quadro.
grep -q "alvoAnt" src/vertudo.c
! grep -q "anim_mola2(&vitScrollV" src/vertudo.c
echo "animtempo: guarda da rolagem vertical ok"
