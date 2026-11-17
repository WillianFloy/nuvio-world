#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
h = open('src/home.c').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("colCamManter(pastaFocoId, agora, motionReduzido || gfx_efeitos_minimos());" in h, "hero da pasta: troca coordenada (camadas)")
ok("if (f && !pastaPronta(f) && (Uint32)(agora - colDesEm) < (Uint32)NV_HERO_ESPERA_MS) return;" in h, "troca so com banner+logo prontos (limite 600 ms), anterior visivel")
ok("pastaLogoUrl(f)" in h and "tex_obter_logo_larg(u, NV_COLLECTION_HERO_LOGO_MAX_W + 40.0f)" in h, "o logo da pasta e pedido junto com o banner")
ok("float la=i==0?1.0f:ht_ease(colCam[i].t/NV_HERO_ARTE_FADE_MS);" in h, "crossfade da arte em NV_HERO_ARTE_FADE_MS")
ok("if(i==colCamN-1&&al[k]<HT_PISO)al[k]=HT_PISO;" in h, "piso de alfa: linhas rasterizam na troca")
ok("static void dinArteColecao(float dt) {" in h and "dinColAnt" in h, "Dinamica: crossfade")
print("home_colhero.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/home_colhero.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/home_colhero.c -o "$D/home_colhero" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
for L in 0 1 2; do echo "== layout $L"; SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/home_colhero" $L 2>&1 | grep -E "^(PASS|FAIL|home_colhero)|Assertion|fixture" || true; done | tee "$D/colfoco.out"; ! grep -q "FAIL\|FALHOU" "$D/colfoco.out"
