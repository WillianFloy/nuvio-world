#!/bin/bash
# Itens A e B da auditoria de 20/11 por comportamento: o menu lateral aberto
# empurra o conteudo SO na Padrao; no Preto puro a ilha da Moderna (barra
# Moderna desligada) e #000. Uma execucao por configuracao. Ver tests/menu_empurra_preto.c.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-menu-ab.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" != src/main.c ] && sources+=("$source")
done
cc "${sources[@]}" tests/menu_empurra_preto.c -Isrc -o "$work/test" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
falhou=0
# nome layout moderna rail preto espera-empurra espera-preto(- = nao mede) [modo do menu: 1 icones e textos (padrao), 0 so icones]
while read -r nome lay mod rail preto emp pr modo; do
  [ -z "$nome" ] && continue
  mkdir -p "$work/d-$nome"
  if [ "$pr" = "-" ]; then pr=""; fi
  rc=0
  env NUVIO_DADOS="$work/d-$nome" CONFIG="$nome" NUVIO_SHOT_LAYOUT="$lay" NUVIO_SHOT_MODERNA="$mod" \
      NUVIO_SHOT_RAIL="$rail" NUVIO_SHOT_MENU_MODO="${modo:-1}" NUVIO_TEMA_PRETO="$preto" ESPERA_EMPURRA="$emp" ${pr:+ESPERA_PRETO=$pr} \
      "$work/test" > "$work/$nome.log" 2>&1 || rc=$?
  grep -E "^(PASS|FAIL|\[$nome\])" "$work/$nome.log" || true
  [ $rc = 0 ] || falhou=1
done <<'M'
mod-world-fixa        0 1 1 0 0 -
mod-world-preto       0 1 1 1 0 0
mod-ilha-fixa         0 0 1 0 0 0
mod-ilha-recolhida    0 0 0 0 0 -
mod-ilha-preto        0 0 1 1 0 1
mod-ilha-preto-recol  0 0 0 1 0 1
pad-gaveta-fixa       1 1 1 0 1 -
pad-gaveta-recolhida  1 1 0 0 1 -
pad-ilha-fixa         1 0 1 0 0 -
pad-ilha-recolhida    1 0 0 0 0 -
pad-ilha-preto        1 0 1 1 0 1
pad-gaveta-preto      1 1 1 1 1 1
pad-so-icones         1 0 1 0 0 - 0
M
[ $falhou = 0 ] && echo "menu_empurra_preto: ok" || { echo "menu_empurra_preto: FALHA"; exit 1; }
