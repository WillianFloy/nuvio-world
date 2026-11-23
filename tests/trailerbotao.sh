#!/bin/bash
# World (23/11): trailer do YouTube no botao (dublado, legendado, legenda). trailerbotao.c real, sem rede.
set -eu
cd "$(dirname "$0")/.."
B="${TMPDIR:-/tmp}/nuvio-trailerbotao"
cc -O1 -g -Wall -Isrc tests/trailerbotao.c src/trailerbotao.c -o "$B"
"$B"
