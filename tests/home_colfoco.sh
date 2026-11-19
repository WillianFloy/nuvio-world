#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
h = open('src/home.c').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("return t != FILEIRA_PALCO && t != FILEIRA_HEROI_DIVIDIDO;" in h, "foco fixo inclui as Collections da Home (CATALOGOS) e todo estilo (18/11)")
ok("if (telefoneui_ativo() || homeRetratoTelefone()) return 0;" in h, "so na TV")
ok("const int focoFixoCol = homeFocoFixo(r);" in h and "if (focoFixoCol) gfx_sem_esvanecer_x();" in h, "esvanecer das pontas liga e desliga em par nas Collections")
print("home_colfoco.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/home_colfoco.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/home_colfoco.c -o "$D/home_colfoco" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
for L in 0 1 2; do echo "== layout $L"; SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/home_colfoco" $L 2>&1 | grep -E "^(PASS|FAIL|home_colfoco)|Assertion|fixture" || true; done | tee "$D/colfoco.out"; ! grep -q "FAIL\|FALHOU" "$D/colfoco.out"
