#!/usr/bin/env bash
# Foco fixo na Home (2.0.41): guardas de codigo + teste dinamico com o home.c de verdade
# (Linux, GLES2 por software). Os objetos dos outros .c ficam em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
v = open('src/vertudo.c').read(); h = open('src/home.c').read(); a = open('src/app.c').read(); t = open('src/herotexto.h').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("vooTem = anima && vooPedido;" in v and "!ajustes_animacoes_reduzidas() && !gfx_efeitos_minimos() && !vtTelefone()" in v, "o voo so com o ajuste ligado, sem reduzidas/efeitos minimos/telefone")
ok("vertudo_colecao_voo(fo, orig, raioDe(orig.w / homeUnidade(), orig.h / homeUnidade()) * orig.h);" in h and "home_ancora_foco(&orig)" in h, "a Home passa o retangulo real do cartao (em coordenadas da tela)")
ok("if (!detail_cobre_tela() && !vertudo_cobre_tela()) {" in a, "app.c: a Home fica por baixo durante o voo")
ok("static void vooAquecer(void)" in v and "home_vitrine_cartao_aquecer(vitItem(r, c), w)" in v and "r < 3" in v, "pre-aquecimento: so as 3 primeiras fileiras visiveis")
ok("vooAquecer();" in v and "anim = ht_voo_fundo(vitEntradaT)" in v, "o fundo acompanha o voo e o conteudo e pedido em paralelo")
ok("static void vooDesenhar(void)" in v and "vooDesenhar(); }" in v, "o cartao do voo e desenhado por cima do conteudo")
ok(v.count("tex_obter_hero(ban)") == 1 and "tex_obter_larg_qualquer(capa, vooOrigem.w)" in v, "no maximo duas camadas de imagem (capa e banner), ambas ja em cache")
ok("HT_ENT_TOTAL_MS" in t and "HT_VOO_MS           380.0f" in t, "curvas do voo")
print("vit_voo.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D" "$D/data"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cp tests/vit_voo.c "$D/_t.c"
cc $FLAGS "$D"/*.o tests/vit_voo.c -o "$D/vit_voo" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
for L in 0 1 2; do echo "== layout $L"; SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data" NUVIO_TESTE_DIR="$D/data" timeout 300 "$D/vit_voo" $L 2>&1 | grep -E "^(PASS|FAIL|vit_voo)|Assertion|fixture" || true; done | tee "$D/vit_voo.out"; ! grep -q "FAIL\|FALHOU" "$D/vit_voo.out"
