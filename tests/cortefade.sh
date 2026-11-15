#!/bin/bash
# Curva do esvanecer da sinopse cortada + guarda: so a linha de reticencias, so a Vitrine.
set -eu
cd "$(dirname "$0")/.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -O1 -g -fsanitize=address,undefined -Wall -Wextra -Isrc tests/cortefade.c -lm -o "$out/t"
"$out/t"
python3 - <<'PY'
import re
t = open("src/text.c").read(); h = open("src/home.c").read(); hd = open("src/text.h").read()
assert "void txt_corte_esvanecer(int ligado)" in hd
# o esvanecer so e usado no caminho das reticencias (linha cortada), nunca no texto que cabe
blk = t[t.index("static void desenhaBlocoLinha("):t.index("static float txt_bloco_impl(")]
assert blk.count("desenhaLinhaCorte(") == 2 and "txt_desenhar_alpha(l, x, y, alpha);\n    return;\n  }\n  {" in blk
assert blk.index("if (!reticencias)") < blk.index("desenhaLinhaCorte(")
# so a Vitrine liga; e sempre desliga depois
i = h.index("txt_corte_esvanecer(1)")
assert "if (vitTeto > 0.0f) txt_corte_esvanecer(1);" in h and "txt_corte_esvanecer(0);" in h[i:i+400]
assert h.count("txt_corte_esvanecer(1)") == 1
PY
echo "cortefade.sh: ok"
