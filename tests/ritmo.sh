#!/bin/bash
# Modo de desempenho: grid de apresentacao do Qualidade (ritmo.c, sem GL).
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nuvio-ritmo"
cat > "$out-stub.c" <<'S'
int ajustes_fps_modo(void) { return 2; }
S
cc tests/ritmo.c src/ritmo.c "$out-stub.c" -Isrc -o "$out" $(pkg-config --cflags --libs sdl2 2>/dev/null || sdl2-config --cflags --libs) -lm -w
"$out"
