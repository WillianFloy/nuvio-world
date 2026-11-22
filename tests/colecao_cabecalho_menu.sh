#!/usr/bin/env bash
# World 22/11: cabecalho da Colecao x menu do topo (ver o .c). Linux, GLES2 por software.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
v = open('src/vertudo.c').read(); m = open('src/menu.c').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("static float vtIlhaY(void) { return NV_ILHA_Y; }" in v, "o cabecalho da colecao fica no lugar de sempre (nao desce para baixo do menu)")
ok("float menu_topo_abertura(void)" in m and "vtCabMenu()" in v, "o cabecalho apaga com a abertura do menu do topo")
print("colecao_cabecalho_menu.sh: guardas ok")
P
D=${NUVIO_OBJ_CAB:-${TMPDIR:-/tmp}/nuvio-obj-cab}
mkdir -p "$D"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/vertudo.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] && [ "$o" -nt src/menu.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cc $FLAGS "$D"/*.o tests/colecao_cabecalho_menu.c -o "$D/colecao_cabecalho_menu" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
rm -rf "$D/data"; mkdir -p "$D/data"
OUT=$(SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 900 "$D/colecao_cabecalho_menu" 2>&1 | grep -E "^(PASS|FAIL|colecao_cabecalho_menu|\[medida\])|Assertion" || true)
echo "$OUT"
! echo "$OUT" | grep -q "FAIL\|FALHOU\|Assertion" && echo "$OUT" | grep -q "colecao_cabecalho_menu: ok"
