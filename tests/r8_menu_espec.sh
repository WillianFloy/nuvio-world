#!/bin/bash
# O MENU DE SEGURAR OK (Fase 3) contra a prancha R8-MC (Folha), quadro a quadro
# (60/s), na Home real: a gaveta, a pilula de foco, o titulo e o veu da Home
# congelada. Depois, o comportamento (abrir a Folha, fechar e reabrir sem
# salto, foco, a Home volta viva, o painel de Salvos fora). O Desdobrar saiu do
# app em 27/11: o caso "Desdobrar gravado" confere que esses dados abrem a Folha.
#   bash tests/r8_menu_espec.sh
set -eu
cd "$(dirname "$0")/.."
T="${TMPDIR:-/tmp}"; T="${T%/}"
work=$(mktemp -d "$T/nuvio-r8-menu.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 tests/r8_menu_espec.py MC "$work/mc.txt"
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
mkdir -p "$work/MC"
if ! NUVIO_DADOS="$work/MC" NV_ESPEC="$work/mc.txt" NV_POSTER=1 "$work/shot" MC "$work" 0 > "$work/MC.log" 2>&1; then rc=1; fi
grep -E "FALHOU|r8_menu_espec|abertura: a curva" "$work/MC.log" || { echo "MC: sem resultado"; tail -5 "$work/MC.log"; rc=1; }
# Comportamento: Folha por padrao e com o Desdobrar gravado nos dados antigos.
for d in "" 1; do
  mkdir -p "$work/comp$d"
  if ! env ${d:+NV_DESDOBRAR_GRAVADO=1} NUVIO_DADOS="$work/comp$d" NV_POSTER=1 "$work/shot" COMP "$work" 0 > "$work/comp$d.log" 2>&1; then rc=1; fi
  grep -E "^ok|FALHOU|comportamento" "$work/comp$d.log" || { echo "COMP$d: sem resultado"; rc=1; }
done
# Reduzir animacoes, nos dois casos.
for d in "" 1; do
  mkdir -p "$work/red$d"
  if ! env ${d:+NV_DESDOBRAR_GRAVADO=1} NV_REDUZIDO=1 NUVIO_DADOS="$work/red$d" NV_POSTER=1 "$work/shot" COMP "$work" 0 > "$work/red$d.log" 2>&1; then rc=1; fi
  grep -E "reduzidas|FALHOU|comportamento" "$work/red$d.log" || { echo "RED$d: sem resultado"; rc=1; }
done
exit $rc
