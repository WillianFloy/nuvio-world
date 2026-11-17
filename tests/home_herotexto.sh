#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
h = open('src/home.c').read(); v = open('src/vertudo.c').read(); l = open('src/layout.h').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok('#define NV_HERO_ARTE_FADE_MS  NV_MOTION_MS' in l and '#define NV_HERO_FADE_MS       220.0f' in l, 'crossfade da arte = tempo-base; palco (NV_HERO_FADE_MS) intacto')
ok('heroSai -= dt * (1000.0f / NV_HERO_ARTE_FADE_MS);' in h, 'heroSai usa a constante propria do Hero')
ok('palcoFundoT' in h and 'NV_HERO_FADE_MS' in h, 'palco continua na constante antiga')
ok('const float aLogoImg = el ? el->aLogo : aTexto * heroEntra;' in h, 'chamadas antigas (el == NULL) iguais ao legado')
ok('heroDesliza = 1.0f;' in h and 'heroTextoAtivo = 0;' in h, 'deslizar sem texto escalonado')
ok('if (!motionReduzido && !gfx_efeitos_minimos()' in h, 'reduzidas / efeitos minimos: troca como antes')
ok('NV_HERO_ESPERA_MS     600' in l and 'VIT_HERO_DUR (NV_MOTION_MS / 1000.0f)' in v and 'home_vitrine_texto_el(' in v, 'esperas e pilha das Collections intactas')
print('home_herotexto.sh: guardas ok')
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/home_herotexto.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/home_herotexto.c -o "$D/home_herotexto" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/home_herotexto" 2>&1 | grep -E "^(PASS|FAIL|home_herotexto)|Assertion"
