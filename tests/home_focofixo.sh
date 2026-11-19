#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
h = open('src/home.c').read(); m = open('src/main.c').read(); v = open('src/vertudo.c').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("return t != FILEIRA_PALCO && t != FILEIRA_HEROI_DIVIDIDO;" in h, "foco fixo em TODO estilo de fileira (18/11), menos Palco e Heroi dividido; telefone fora")
ok("if (telefoneui_ativo() || homeRetratoTelefone()) return 0;" in h, "foco fixo so na TV (telefone/retrato como antes)")
ok("alvo = vitgeom_fixo_alvo(col, homeFixoTotal(r), homeFixoN(r), passo);" in h, "alvo da rolagem pelo modelo das Collections (vitgeom)")
ok("if (fileiras[r].tipo == FILEIRA_HEROI_DIVIDIDO) return esq + passo;" in h, "heroi dividido segue o caminho antigo")
ok("if (dir + folga - alvo > util) alvo = dir + folga - util;" in h and "if (esq - folga - alvo < 0.0f) alvo = esq - folga;" in h, "regra antiga (so na borda) mantida no telefone e como piso #103")
ok("homeFocoFixo(r) ? homeW2()" in h and "(NV_MOLA2_SCROLL * ajustes_navegacao_velocidade_cartazes())" in h, "mola da fileira: tempo-base so no foco fixo")
ok("homeFocoFixo(r) ? homeW1()" in h and ": alvo > animFoco[r][c] ? NV_MOLA_FOCO : NV_MOLA_DESFOCO" in h, "anel de foco: tempo-base so no foco fixo")
ok(h.count("homeFocoFixoFade(r);") == 2 and h.count("gfx_sem_esvanecer_x();") == 2, "o esvanecer das pontas liga e desliga em par (fileiras normais/cartaz e, 2.0.43, Collections da Home)")
ok(h.index("homeFocoFixoFade(r);") < h.index("gfx_sem_esvanecer_x();"), "o esvanecer liga antes de desligar")
ok("NV_MOTION_W2" in h and "NV_MOTION_W1" in h, "usa o tempo-base da 2.0.40")
ok("home_foco_fixo_ativo()" in m and "navritmo_baixou_vitrine(&navRitmo" in m, "main.c: ritmo de segurar das Collections nas fileiras fixas (so horizontal)")
# Collections (2.0.40) intactas
ok("anim_mola2_tempo(&vitSv[r], vitSx[r], alvoX, dtV, vitW2())" in v and "#define VIT_HERO_DUR (NV_MOTION_MS / 1000.0f)" in v, "Collections 2.0.40 intactas")
print("home_focofixo.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/home_focofixo.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/home_focofixo.c -o "$D/home_focofixo" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/home_focofixo" 2>&1 | grep -E "^(PASS|FAIL|home_focofixo)|Assertion"
