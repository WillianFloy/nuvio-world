#!/bin/bash
# A tela dos Logs de diagnostico na TV (GL): capturas de cada fase e o QR lido
# da propria captura (OpenCV) e aberto. Ver tests/diaglog_shot.c/.py.
#   bash tests/diaglog_shot.sh [prefixo-das-capturas]
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-diagshot.XXXXXX")
trap 'rm -rf "$work"' EXIT
pref=${1:-$work/tv}
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" != src/main.c ] && sources+=("$source")
done
cc -DAJUSTES_TESTE "${sources[@]}" tests/diaglog_shot.c -Isrc -o "$work/shot" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
python3 tests/diaglog_shot.py "$work/shot" "$work" "$pref"
