#!/usr/bin/env bash
# Ritmo da navegacao por setas (Navegação geral rápida desligada) + guardas de integracao.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${TMPDIR:-/tmp}/nuvio-navritmo
mkdir -p "$D"
cc -std=c11 -Wall -Wextra -Werror tests/navritmo.c -o "$D/navritmo"
"$D/navritmo"
python3 - <<'P'
a=open("src/ajustes.c").read(); m=open("src/main.c").read()
def ok(c,t):
    if not c: raise SystemExit("FAIL: "+t)
ok('ESC("Navegação geral rápida", V_LIGA, 2)' in a, "rotulo novo")
ok('"fastHorizontalNavigationEnabled"' in a, "chave sincronizada preservada")
ok("navritmo_baixou(" in m and "ajustes_navegacao_horizontal_rapida()" in m, "main.c aplica o ritmo")
ok("navritmo_soltou(" in m, "KEYUP reinicia o toque")
ap=open("src/app.c").read()
ok("app_navegacao_cartazes() &&" in m and "int app_navegacao_cartazes(void)" in ap, "ritmo so entre cartazes e Collections")
ok("menu_aberto() || spainel_aberto() || ctx_aberto()" in ap, "menus e paineis fora do ritmo")
print("navritmo.sh: tudo ok")
P
