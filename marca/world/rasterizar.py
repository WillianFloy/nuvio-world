# Uso: python3 marca/world/rasterizar.py <logo.svg> <saida.png> <escala>
# Os textos dos SVGs sao <text> em Arial (nao curvas): aqui saem em Liberation
# Sans, de metricas identicas. Ver marca/world/LEIAME.md.
# Rasteriza um logo SVG do usuario: formas por rsvg; textos (Arial) desenhados
# com Liberation Sans (metricas identicas a Arial), nas posicoes exatas do SVG.
import re, subprocess, sys, tempfile, os
_tmp = os.path.join(tempfile.mkdtemp(), 't')
from PIL import Image, ImageDraw, ImageFont
src, out, K = sys.argv[1], sys.argv[2], float(sys.argv[3])
s = open(src).read()
vb = [float(v) for v in re.search(r'viewBox="([^"]+)"', s).group(1).split()]
W, H = int(vb[2]*K), int(vb[3]*K)
semtexto = re.sub(r'<g transform="matrix\([^)]*\)">\s*(<text.*?</text>\s*)+</g>', '', s, flags=re.S)
open(_tmp + '.svg','w').write(semtexto)
subprocess.run(['rsvg-convert','-w',str(W),'-h',str(H),_tmp + '.svg','-o',_tmp + '.png'],check=True)
img = Image.open(_tmp + '.png').convert('RGBA')
d = ImageDraw.Draw(img)
for g in re.finditer(r'<g transform="matrix\(([^)]*)\)">(.*?)</g>', s, flags=re.S):
    m = [float(v) for v in g.group(1).split(',')]
    for t in re.finditer(r'<text x="([\d.]+)px" y="([\d.]+)px" style="([^"]*)">(.*?)</text>', g.group(2), flags=re.S):
        x, y, st, corpo = float(t.group(1)), float(t.group(2)), t.group(3), t.group(4)
        bold = 'font-weight:700' in st
        fs = float(re.search(r'font-size:([\d.]+)px', st).group(1))
        cor = (255,255,255,255) if 'fill:white' in st else (0,0,0,255)
        ttf = '/usr/share/fonts/truetype/liberation/LiberationSans-%s.ttf' % ('Bold' if bold else 'Regular')
        px = fs*m[0]*K
        f = ImageFont.truetype(ttf, int(round(px)))
        # pedacos: (texto, letter-spacing)
        pedacos = []
        for p in re.finditer(r'<tspan style="letter-spacing:([-\d.]+)px">([^<]*)</tspan>|([^<]+)', corpo):
            if p.group(3): pedacos.append((p.group(3), 0.0))
            else: pedacos.append((p.group(2), float(p.group(1))))
        cx = (x*m[0]+m[4])*K; by = (y*m[3]+m[5])*K
        for txt, ls in pedacos:
            for ch in txt:
                d.text((cx, by), ch, font=f, fill=cor, anchor='ls')
                cx += f.getlength(ch) + ls*m[0]*K
img.save(out)
