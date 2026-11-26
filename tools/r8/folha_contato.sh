#!/bin/bash
# FOLHA DE CONTATO DO MENU DE SEGURAR OK: todas as combinacoes de forma
# (Folha/Desdobrar) x origem (poster, borda, destaque, pasta) x tom (Grafite,
# Preto puro), em 1280 x 720 (o drawable da TCL), com o relogio desenhado pela
# regra do app, em tres instantes (abrindo, aberto, fechando). Uma imagem so,
# para revisar o encaixe antes de cada APK de teste.
#   bash tools/r8/folha_contato.sh [saida.png] [instantes...]
# Instantes no tempo das pranchas (o menu abre em 1,3 s e fecha em 5,28 s).
# Variaveis: NV_SEM_RELOGIO=1 (relogio desligado), FC_1080=1 (1920 x 1080).
set -eu
cd "$(dirname "$0")/../.."
saida="${1:-${TMPDIR:-/tmp}/r8-folha-contato.png}"
shift || true
instantes=("$@")
[ ${#instantes[@]} -gt 0 ] || instantes=(1.40 2.20 5.40)
T="${TMPDIR:-/tmp}"; T="${T%/}"
work=$(mktemp -d "$T/nuvio-r8-contato.XXXXXX")
trap 'rm -rf "$work"' EXIT
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
[ -n "${FC_1080:-}" ] || export NV_720=1
for forma in MC MA; do
  for origem in poster borda hero colecao; do
    for tom in grafite preto; do
      d="$work/$forma-$origem-$tom"; mkdir -p "$d/dados"
      env NUVIO_DADOS="$d/dados" NV_POSTER=1 NV_ORIGEM="$origem" \
          $([ "$forma" = MA ] && echo NV_DESDOBRAR=1) $([ "$tom" = preto ] && echo NV_PRETO=1) \
          "$work/shot" "$forma" "$d" "${instantes[@]}" > "$d/log" 2>&1 || { echo "falhou: $forma $origem $tom"; tail -3 "$d/log"; }
    done
  done
done
python3 - "$work" "$saida" "${instantes[@]}" <<'EOF'
import os, sys
from PIL import Image, ImageDraw, ImageFont
work, saida, inst = sys.argv[1], sys.argv[2], sys.argv[3:]
W, H, ROT = 640, 360, 26
casos = [(f, o, t) for f in ("MC", "MA") for o in ("poster", "borda", "hero", "colecao") for t in ("grafite", "preto")]
nome = {"MC": "Folha", "MA": "Desdobrar"}
img = Image.new("RGB", (W * len(inst), (H + ROT) * len(casos)), (40, 40, 40))
dr = ImageDraw.Draw(img)
fonte = ImageFont.truetype("deploy/app/fonts/Montserrat-Medium.ttf", 16)
for i, (f, o, t) in enumerate(casos):
    for j, q in enumerate(inst):
        p = os.path.join(work, f"{f}-{o}-{t}", f"app-{f}-{float(q):.3f}.png")
        x, y = j * W, i * (H + ROT)
        dr.text((x + 8, y + 6), f"{nome[f]} · {o} · {t} · t={q}", fill=(235, 235, 235), font=fonte)
        if os.path.exists(p):
            img.paste(Image.open(p).convert("RGB").resize((W, H)), (x, y + ROT))
        else:
            dr.text((x + 8, y + ROT + 8), "sem captura", fill=(255, 120, 120), font=fonte)
img.save(saida)
print(saida)
EOF
