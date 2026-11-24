#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Decisao do fundo congelado sob a Busca (src/fundoparado.h), sem GL. Ver o .c.
#
#   bash tests/fundoparado.sh
set -eu
cd "$(dirname "$0")/.."
cc tests/fundoparado.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-fundoparado \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-fundoparado
