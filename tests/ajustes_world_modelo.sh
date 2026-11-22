#!/bin/bash
# Regras puras dos Ajustes World (grade, segurar OK, rascunho, rolagem). Sem GL.
#   bash tests/ajustes_world_modelo.sh
set -eu
cd "$(dirname "$0")/.."
w=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ajw.XXXXXX"); trap 'rm -rf "$w"' EXIT
cc -std=c99 -Wall -Wextra -Werror -Isrc tests/ajustes_world_modelo.c -o "$w/t"
"$w/t"
