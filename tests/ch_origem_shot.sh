#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# De onde Busca, Salvos e Central nascem; segurar CH+ sem relogio. Nao entra na suite (*_shot).
#
#   bash tests/ch_origem_shot.sh /tmp/nv-ch_origem_shot
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/nv-ch_origem_shot}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ch-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ch_origem_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$NUVIO_DADOS/shot" "$saida/s"
for f in "$saida"/s-*.bmp; do
  [ -e "$f" ] || continue
  sips -s format png "$f" --out "${f%.bmp}.png" >/dev/null 2>&1 && rm -f "$f"
done
