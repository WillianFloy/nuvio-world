#!/bin/bash
# Mola de rolagem em tempo real (independente do FPS).
set -eu
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/animtempo.c -lm -o "$out/t"
"$out/t"
# Guarda: a atualizacao da Vitrine mede na mesma escala do desenho (vtEscala),
# senao o passo do alvo difere do desenhado e a fileira focada desce a cada tecla.
grep -q "gfx_escala_sair(vtEscala());\n  vitAtualizarCorpo" src/vertudo.c || python3 - <<'P'
import sys
s=open("src/vertudo.c").read()
sys.exit(0 if "gfx_escala_sair(vtEscala());\n  vitAtualizarCorpo(dt, agora);" in s else 1)
P
grep -q "anim_mola2_tempo(&vitScrollV" src/vertudo.c
echo "animtempo: guarda da escala da Vitrine ok"
