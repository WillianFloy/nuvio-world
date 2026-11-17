#!/bin/bash
# Tempo-base do movimento (350 ms) e ritmo de segurar a seta nas Collections (2.0.40).
set -e
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-motion-ritmo
mkdir -p "$D"
# So as constantes de tempo do layout.h (o cabecalho inteiro nao entra no teste).
python3 - "$D" <<'P'
import sys
s = open('src/layout.h').read()
i = s.index('#define NV_MOTION_MS'); j = s.index('\n', s.index('/s', s.index('#define NV_MOTION_W1')))
open(sys.argv[1] + '/motion_consts.h', 'w').write(s[i:j + 1])
P
cc -std=c11 -Wall -Wextra -Werror -I"$D" tests/motion_ritmo.c -o "$D/motion_ritmo" -lm
"$D/motion_ritmo"
python3 - <<'P'
m = open('src/main.c').read(); v = open('src/vertudo.c').read(); l = open('src/layout.h').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("!((vertudo_aberta() ||" in m and "? navritmo_baixou_vitrine(&navRitmo" in m and ": navritmo_baixou(&navRitmo" in m, "main.c: Vitrine (e fileiras de foco fixo da Home, so na horizontal) usam o passo novo, o resto o antigo")
ok("NV_MOTION_W2 * ajustes_navegacao_velocidade()" in v and "anim_mola2_tempo(&vitSv[r], vitSx[r], alvoX, dtV, vitW2())" in v, "fileira da Vitrine no tempo-base")
ok("vitFocoF = anim_mola(vitFocoF, r >= 0 ? 1.0f : 0.0f, dt, vitW1());" in v, "foco da Vitrine no tempo-base")
ok("#define VIT_HERO_DUR (NV_MOTION_MS / 1000.0f)" in v, "destaque da Vitrine no tempo-base")
ok("vitScroll = anim_mola2_tempo(&vitScrollV, vitScroll, alvo, dtV, (NV_MOLA2_SCROLL * ajustes_navegacao_velocidade_cartazes()));" in v, "rolagem vertical ainda como antes (etapa propria)")
ok("#define NV_MOTION_MS  350.0f" in l, "tempo-base 350 ms")
print("motion_ritmo.sh: tudo ok")
P
