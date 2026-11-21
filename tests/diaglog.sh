#!/bin/bash
# Sessoes de diagnostico (diaglog.h + logrede.c): fluxo completo com o servidor
# de verdade em 127.0.0.1 -- inclui a espera REAL dos 120 s (~2,5 min no total).
# DIAG_SHOTS=<pasta> guarda as capturas do Chrome (celular e computador).
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-diaglog.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -O1 -g -Wall -Wextra -pthread -Isrc src/diaglog.c src/logrede.c tests/diaglog_tv.c -o "$work/tv"
python3 tests/diaglog_fluxo.py "$work/tv" "$work"
