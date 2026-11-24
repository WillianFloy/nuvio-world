#!/bin/bash
# "Marcar como assistido" nao pode vazar entre titulos de id nao-IMDb (tmdb:, kitsu:).
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-visto-id-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
export NUVIO_DADOS="$tmp/dados"; mkdir -p "$NUVIO_DADOS"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/visto_id_nao_imdb.c -Isrc -o "$tmp/teste" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/teste"
