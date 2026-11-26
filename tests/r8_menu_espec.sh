#!/bin/bash
# O MENU DE SEGURAR OK (Fase 3) contra as pranchas R8-MC (Folha) e R8-MA
# (Desdobrar), quadro a quadro (60/s), na Home real: a superficie, a pilula de
# foco, o titulo, o veu da Home congelada e o deslocamento das vizinhas. Depois,
# o comportamento (abrir na forma do ajuste, fechar e reabrir sem salto, foco,
# a Home volta viva, o painel de Salvos fora).
#   bash tests/r8_menu_espec.sh
set -eu
cd "$(dirname "$0")/.."
T="${TMPDIR:-/tmp}"; T="${T%/}"
work=$(mktemp -d "$T/nuvio-r8-menu.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 tests/r8_menu_espec.py MC "$work/mc.txt"
python3 tests/r8_menu_espec.py MA "$work/ma.txt"
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" = src/main.c ] || sources+=("$source"); done
if [ "$(uname)" = Darwin ]; then
  cc "${sources[@]}" tests/r8_menu_quadros_shot.c -Isrc -o "$work/shot" -O1 -I/opt/homebrew/include \
    -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
    -framework OpenGL -Wno-deprecated-declarations -Wno-macro-redefined
else
  cc "${sources[@]}" tests/r8_menu_quadros_shot.c -Isrc -o "$work/shot" -O1 -w $(sdl2-config --cflags) \
    -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lm -lpthread -ldl
  export SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1
fi
rc=0
for p in MC MA; do
  mkdir -p "$work/$p"
  if ! NUVIO_DADOS="$work/$p" NV_ESPEC="$work/$(echo $p | tr A-Z a-z).txt" NV_POSTER=1 \
       NUVIO_MENU_SEGURAR=$([ $p = MA ] && echo 1 || echo 0) "$work/shot" $p "$work" 0 > "$work/$p.log" 2>&1; then rc=1; fi
  grep -E "FALHOU|r8_menu_espec" "$work/$p.log" || { echo "$p: sem resultado"; tail -5 "$work/$p.log"; rc=1; }
done
# Comportamento: Folha por padrao e Desdobrar gravado no ajuste.
for d in "" 1; do
  mkdir -p "$work/comp$d"
  if ! env ${d:+NV_DESDOBRAR=1} NUVIO_DADOS="$work/comp$d" NV_POSTER=1 "$work/shot" COMP "$work" 0 > "$work/comp$d.log" 2>&1; then rc=1; fi
  grep -E "^ok|FALHOU|comportamento" "$work/comp$d.log" || { echo "COMP$d: sem resultado"; rc=1; }
done
# Reduzir animacoes, nas duas formas.
for d in "" 1; do
  mkdir -p "$work/red$d"
  if ! env ${d:+NV_DESDOBRAR=1} NV_REDUZIDO=1 NUVIO_DADOS="$work/red$d" NV_POSTER=1 "$work/shot" COMP "$work" 0 > "$work/red$d.log" 2>&1; then rc=1; fi
  grep -E "reduzidas|FALHOU|comportamento" "$work/red$d.log" || { echo "RED$d: sem resultado"; rc=1; }
done
exit $rc
