#!/usr/bin/env bash
# 2.0.45: transformacao Home -> Collection (home.c + vertudo.c reais, GLES2 por software). Objetos em cache (NUVIO_OBJ).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
v = open('src/vertudo.c').read(); h = open('src/home.c').read(); a = open('src/app.c').read(); t = open('src/transicao.h').read()
def ok(c, msg):
    if not c: raise SystemExit("FAIL: " + msg)
ok("vooTem = anima && vooPedido;" in v and "!ajustes_animacoes_reduzidas() && !gfx_efeitos_minimos() && !vtTelefone()" in v, "so com o ajuste ligado, sem reduzidas/efeitos minimos/telefone")
ok("vertudo_colecao_voo(fo, orig, raioDe(orig.w / homeUnidade(), orig.h / homeUnidade()) * orig.h);" in h, "a Home passa o retangulo real do cartao (tela)")
ok("if (!detail_cobre_tela() && !vertudo_cobre_tela()) {" in a, "app.c: a Home fica por baixo ate o destino cobrir")
ok("tr_passo(&vitTr, dt * 1000.0f, vitTrPronto());" in v, "o relogio adaptativo le o portao do conteudo")
ok("gfx_transformar(homeRecuoOx, homeRecuoOy, 1.0f - TR_HOME_ESCALA * recuo, 0.0f, 0.0f)" in h, "a Home recua so por escala (dx = dy = 0: sem deslocamento vertical)")
ok("(1.0f-cinema)*(1.0f-recuo)" in h, "as fileiras da Home apagam com o recuo")
ok("home_vitrine_arte_banner(col_banner(collection)" in v, "D1: o banner da pasta e a camada de destaque da Vitrine")
ok("if (seguraTitulo) vitHeroEntra = 0.0f;" in v, "D1: o titulo so entra depois da transformacao")
ok("if (vooTem) { vooDesenhar(); vooDesenhado = 1; }" in v, "o cartao fica sob os textos da Vitrine")
ok("tex_lookahead_pedir(vitPrever[vitPreverI], vitPreverW)" in v and "colcache_ler(colcache_chave(src, base, cap)" in v, "D2: pre-carga so do colcache e com o cache ocioso")
ok(v.count("home_vitrine_arte_pronta(it0)") == 0, "D1: a arte do titulo nao e pedida durante o movimento")
ok("#define TR_TETO_MS    1000.0f" in t, "teto de 1000 ms")
ok("if (homeOcultaOrigem && foco.fileira == r && foco.coluna == c && !focoHero) continue;" in h, "o cartao de origem sai da fileira da Home (sem copia dupla)")
ok("ff, acc);   // 2.0.45: o anel entra com o cartaz" in v, "o anel de foco da Vitrine entra junto com o cartaz")
print("vit_transforma.sh: guardas ok")
P
D=${NUVIO_OBJ:-${TMPDIR:-/tmp}/nuvio-obj}
mkdir -p "$D"; rm -f "$D/src_home.c.o" "$D/src_main.c.o"
FLAGS="-w -O1 -Isrc -Itests $(sdl2-config --cflags)"
for s in src/*.c src/dts/*.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  o="$D/$(echo "$s" | tr / _).o"
  [ "$o" -nt "$s" ] && [ "$o" -nt src/layout.h ] && [ "$o" -nt src/transicao.h ] && [ "$o" -nt src/vertudo.h ] && [ "$o" -nt src/home.h ] || echo "$s"
done | xargs -P "$(nproc)" -I{} sh -c 'o="'"$D"'/$(echo {} | tr / _).o"; cc -c $0 {} -o "$o"' "$FLAGS"
cc $FLAGS "$D"/*.o tests/vit_transforma.c -o "$D/vit_transforma" -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
OUT="$D/vit_transforma.out"; : > "$OUT"
for L in ${NV_LAYOUTS:-0 1 2}; do
  rm -rf "$D/data$L"; mkdir -p "$D/data$L"
  echo "== layout $L" | tee -a "$OUT"
  SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 NUVIO_DADOS="$D/data$L" NUVIO_TESTE_DIR="$D/data$L" timeout -s KILL 600 "$D/vit_transforma" $L ${NV_SHOTS:+$NV_SHOTS-L$L} 2>&1 \
    | grep -E "^(PASS|FAIL|vit_transforma|\[medida\]|\[prever\])|Assertion|fixture" | tee -a "$OUT" || true
done
! grep -q "FAIL\|FALHOU" "$OUT" && grep -c "vit_transforma: ok" "$OUT" | grep -q "${NV_N:-3}"
