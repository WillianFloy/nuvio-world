#!/usr/bin/env bash
# Menu de segurar OK nas formas novas (Folha, Desdobrar): comportamento e capturas (Linux, GLES2 por software):
#   bash tests/ctxnovo_shot.sh [pasta]
set -euo pipefail
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/nv-ctxnovo-shots}"; mkdir -p "$saida"
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] && [ -z "$(find src -name '*.h' -newer "$o" -print -quit)" ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cc $FLAGS $(ls "$D"/*.o | grep -v -E "_main.c.o|_t.c.o") tests/ctxnovo_shot.c -o "$D/ctxnovo_shot" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ctxnovo-shot.XXXXXX"); export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_SHOT_FONTE=3 timeout 900 "$D/ctxnovo_shot" "$saida" 2>&1 | grep -E "^  [a-zA-Zç]|luma|PASSOU|FALHOU|captura" | grep -v "^  \[" 
