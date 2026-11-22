#!/usr/bin/env bash
# B5 (2.0.46): abrir/fechar o detalhe a partir de um poster. Comportamento com home.c + detail.c de verdade (GLES2 por software);
# nao prova nada sobre a TV. Roda a Moderna (0) e o Padrao (1): os dois layouts de TV em uso.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cc $FLAGS "$D"/*.o tests/detalhe_transicao.c -o "$D/detalhe_transicao" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
RES=0
for L in 0 1; do
  OUT=$(NV_LAYOUT=$L SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 900 "$D/detalhe_transicao" 2>&1 | grep -aE "^(PASS|FAIL|#####|[a-z]+: |Assertion|detalhe_transicao)" || true)
  echo "$OUT"
  if echo "$OUT" | grep -q "FAIL\|FALHOU\|Assertion" || ! echo "$OUT" | grep -q "detalhe_transicao: ok"; then RES=1; fi
done
[ "$RES" = 0 ] && echo "detalhe_transicao: ok (Moderna e Padrao)"
exit $RES
