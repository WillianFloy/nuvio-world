#!/bin/bash
# Sync nao reconcilia as Collections quando a conta manda o mesmo estado
# (a trava de 140-170 ms por ciclo nos logs da TCL, 20/11/2026).
set -eu
cd "$(dirname "$0")/.."
bin="$(mktemp "${TMPDIR:-/tmp}/nuvio-synccol.XXXXXXXX")"
trap 'rm -f "$bin"' EXIT
cc -O1 -g -Isrc -pthread -ffunction-sections -fdata-sections -Wall -Wno-deprecated-declarations -Wno-unused-function \
  tests/sync_colecoes_iguais.c tests/stub_contapend.c src/catordem.c \
  src/catordemcache.c src/contacache.c src/js.c src/jsw.c -o "$bin" $( [ "$(uname)" = Darwin ] && echo -Wl,-dead_strip || echo -Wl,--gc-sections)
d="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-synccol-dir.XXXXXXXX")"
NV_T_DIR="$d" "$bin"; rc=$?
rm -rf "$d"
exit $rc
