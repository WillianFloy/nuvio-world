#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
v = open('src/vertudo.c').read(); a = open('src/ajustes.c').read(); h = open('src/herotexto.h').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("vitEntradaT = !anima ? 1.0e6f : vooTem ? 0.0f : HT_VOO_SEM_CARTAO_MS;" in v and "ajustes_col_entrada() && !ajustes_animacoes_reduzidas() && !gfx_efeitos_minimos() && !vtTelefone()" in v, "entrada so com o ajuste ligado, sem reduzidas/efeitos minimos/telefone (2.0.44: o relogio parte do clique, ou de 190 ms sem cartao)")
ok("ht_ent_lin(r, vitEntradaT)" in v and "ht_ent_cab(vitEntradaT)" in v, "cabecalho e fileiras usam as curvas de entrada")
ok('"colEntradaLocal"' in a and "AJ_COL_ENTRADA" in a, "ajuste persistido (colEntradaLocal)")
ok("HT_ENT_TOTAL_MS" in h, "curvas de entrada")
print("home_colentrada.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/home_colentrada.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/home_colentrada.c -o "$D/home_colentrada" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
for cfg in "0 0" "1 0" "0 1"; do echo "== ajuste/minimos: $cfg"; SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/home_colentrada" $cfg 2>&1 | grep -E "^(PASS|FAIL|home_colentrada)|Assertion|fixture" || true; done | tee "$D/colentrada.out"; ! grep -q "FAIL\|FALHOU" "$D/colentrada.out"
