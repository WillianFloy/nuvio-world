#!/usr/bin/env bash
# World (23/11): trailer em tela cheia em escala real (local, GL por software; nao prova nada sobre a TV).
set -euo pipefail
cd "$(dirname "$0")/.."
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
mkdir -p "$D/hooks"; cc $FLAGS -DNV_SHOT_HOOKS -c src/trailer.c -o "$D/hooks/trailer_hooks.o"   # fora de $D/*.o: os outros testes linkam essa pasta inteira
OBJS=$(ls "$D"/src_*.o | grep -v "/src_trailer.c.o$")
cc $FLAGS $OBJS "$D/hooks/trailer_hooks.o" tests/trailer_cheia_escala.c -o "$D/trailer_cheia_escala" \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
W=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trailer-cheia.XXXXXX")
NUVIO_DADOS="$W" SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 timeout 600 "$D/trailer_cheia_escala" 2>&1 | grep -aE "^(PASS|FAIL|canto|trailer_cheia|Assertion)"
RES=${PIPESTATUS[0]}
rm -rf "$W"
exit $RES
