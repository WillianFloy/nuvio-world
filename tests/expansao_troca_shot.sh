#!/usr/bin/env bash
# TROCA DE ARTE DA EXPANSAO SEM SALTO (World, 21/11; video da TCL: "a troca do poster vertical
# pro horizontal da uma bugada, na entrada e na saida"). Precisa de GL (offscreen) e das artes do
# deploy: nao entra no CI. Congela o cartao aberto em p = 0,40 ... 0,90 (NV_EXP_PONTOS em
# tests/homelayouts_shot.c) e mede a arte dentro da moldura:
#   - dos dois lados da troca (49% -> 51%) a arte e a mesma imagem, so 2% mais larga;
#   - depois da troca a deitada continua em COVER: aos 60% e o centro da arte dos 90%.
# Antes da correcao o GFX_CARD caia no contain entre ~50% e ~70% (moldura longe dos 16:9): a
# imagem encolhia com faixas e voltava. Medido: 42,6 e 32,8 antes; 5,2 e 9,9 depois.
#   bash tests/expansao_troca_shot.sh [pasta]
set -euo pipefail
cd "$(dirname "$0")/.."
out="${1:-${TMPDIR:-/tmp}/nuvio-expansao-troca}"
rm -rf "$out"; mkdir -p "$out"
NV_EXPANSAO=4 NV_EXP_PONTOS=1 NV_RECTS=1 \
NV_AJ=$'focusedPosterBackdropExpandEnabled 0\nfocusedPosterBackdropExpandDelaySeconds 1' \
  bash tests/homelayouts_shot.sh "$out" 0 > "$out/run.log" 2>&1
python3 - "$out" <<'PY'
import sys
from PIL import Image, ImageChops, ImageStat
d = sys.argv[1]; L = open(d + '/run.log').read().split('\n')
def card(p):
    f = False; r = None
    for l in L:
        if l.startswith('[cartao] ---'): f = l.endswith('h-exp-%s.bmp' % p); continue
        if f and 'foco=1' in l: r = [float(v) for v in l.split()[1:5]]; break
    assert r, 'sem retangulo do cartao em ' + p
    im = Image.open('%s/h-exp-%s.png' % (d, p)).convert('RGB'); k = im.width / 2240.0; m = 12
    x, y, w, h = r
    return im.crop((int(x*k)+m, int(y*k)+m, int((x+w)*k)-m, int((y+h)*k)-m))
def dif(a, b):
    b = b.resize(a.size, Image.LANCZOS)
    return sum(ImageStat.Stat(ImageChops.difference(a, b)).mean) / 3
a, b = card('p049'), card('p051')
t = dif(a, b); print('troca 49%% -> 51%%: %.1f' % t)
c60, c90 = card('p060'), card('p090')
cw = int(c90.height * c60.width / c60.height)
u = dif(c60, c90.crop(((c90.width - cw)//2, 0, (c90.width + cw)//2, c90.height)))
print('60%% = centro dos 90%% (cover): %.1f' % u)
assert t < 15, 'a arte salta na troca da metade do caminho'
assert u < 18, 'a deitada nao esta em cover depois da troca (contain)'
print('expansao_troca_shot: ok')
PY
