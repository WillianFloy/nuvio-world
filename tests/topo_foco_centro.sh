#!/bin/bash
# FOCO CENTRADO NO "ICONES E TEXTOS" DO MENU DO TOPO (World, 20/11).
# Para Moderna e Padrao, Pequeno/Medio/Grande e cada item (1..6 passos para a
# direita), o fundo do foco (capsula, ja assentada) e o grupo icone+rotulo
# DESENHADOS tem de ter a mesma sobra dos dois lados (|esq - dir| <= 2 px).
# Usa o log NUVIO_TOPO_GEO do menu.c (geometria do desenho, nao uma copia).
set -eu
export TMPDIR="${TMPDIR:-/tmp}"; TMPDIR="${TMPDIR%/}"
cd "$(dirname "$0")/.."
bin="$TMPDIR/nuvio-topo-foco-centro"
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
cc "${sources[@]}" tests/vidro_menu_shot.c -Isrc -o "$bin" -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations -Wno-macro-redefined
falhas=0
for lay in 0 1; do for tam in 0 1 2; do for fo in 1 2 3 4 5 6; do
  d=$(mktemp -d "$TMPDIR/nuvio-vidro-menu-dados.XXXXXX")
  linha=$(NUVIO_DADOS=$d NUVIO_TOPO_GEO=1 NUVIO_SHOT_SO_MENU=1 NUVIO_SHOT_TOPO=1 NUVIO_SHOT_LAYOUT=$lay \
    NUVIO_SHOT_MENU_MODO=1 NUVIO_SHOT_TOPO_TAM=$tam NUVIO_SHOT_FOCO=$fo "$bin" "$d/s" 2>/dev/null | grep '\[topogeo\]' | tail -1 || true)
  rm -rf "$d"
  if [ -z "$linha" ]; then echo "FAIL: layout $lay tamanho $tam passo $fo: o foco nao cobre pilula nenhuma"; falhas=$((falhas+1)); continue; fi
  ok=$(echo "$linha" | awk '{for(i=1;i<=NF;i++){if($i~/^esq=/)e=substr($i,5); if($i~/^dir=/)d=substr($i,5)} x=e-d; if(x<0)x=-x; print (x<=2.0)?1:0}')
  [ "$ok" = 1 ] || { echo "FAIL: layout $lay tamanho $tam passo $fo: $linha"; falhas=$((falhas+1)); }
done; done; done
[ "$falhas" = 0 ] || exit 1
echo "PASS: foco centrado em 36 combinacoes (2 layouts x 3 tamanhos x 6 itens)"
