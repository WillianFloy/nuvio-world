# Base das pranchas da rodada 24/11 (noite): primitivas, cenas da Home e o motor
# de animacao. Cada elemento animado e uma funcao do tempo t (s) -> propriedades;
# a mesma funcao gera o quadro estatico e os keyframes CSS da previa ao vivo.
import math

SW, SH = 1920, 1080
INK, G1, G2, G3 = "#0B0B0C", "#151517", "#1E1E21", "#2A2A2E"
BONE, MUT, DIM = "#F4F2EE", "#B4B1AB", "#8C8A85"
ACC = "#FF4D6D"
FONT = "Montserrat"
CYCLE = 7.0

FIGS = [
 "radial-gradient(120% 90% at 78% 18%, #9a6f45 0%, rgba(154,111,69,0) 55%), radial-gradient(90% 80% at 12% 90%, #1c5a63 0%, rgba(28,90,99,0) 60%), linear-gradient(160deg,#2a2018,#0a1417)",
 "radial-gradient(110% 90% at 20% 15%, #6a3d8a 0%, rgba(106,61,138,0) 55%), radial-gradient(90% 80% at 85% 85%, #c0583a 0%, rgba(192,88,58,0) 60%), linear-gradient(160deg,#1a1226,#140c0c)",
 "radial-gradient(110% 90% at 70% 20%, #2f7d9f 0%, rgba(47,125,159,0) 55%), radial-gradient(90% 80% at 15% 85%, #d7a04a 0%, rgba(215,160,74,0) 60%), linear-gradient(160deg,#0f1c26,#1a1410)",
 "radial-gradient(110% 90% at 30% 20%, #9a3d3d 0%, rgba(154,61,61,0) 55%), linear-gradient(160deg,#1f1414,#0c0c10)",
 "radial-gradient(110% 90% at 60% 30%, #3d8a5a 0%, rgba(61,138,90,0) 55%), linear-gradient(160deg,#101c16,#0c0c10)",
 "radial-gradient(110% 90% at 40% 25%, #b07a2a 0%, rgba(176,122,42,0) 55%), radial-gradient(80% 70% at 80% 90%, #2a4f9a 0%, rgba(42,79,154,0) 60%), linear-gradient(160deg,#1a1408,#0a0c14)",
 "radial-gradient(110% 90% at 65% 25%, #5d6d7d 0%, rgba(93,109,125,0) 55%), linear-gradient(160deg,#141820,#0a0a0c)",
 "radial-gradient(110% 90% at 35% 30%, #a3466e 0%, rgba(163,70,110,0) 55%), radial-gradient(80% 70% at 80% 85%, #2e6f6a 0%, rgba(46,111,106,0) 60%), linear-gradient(160deg,#1c1018,#0a1010)",
]
NAMES = ["Duna: Parte Dois", "Moana", "Severance", "Fallout", "Andor", "O Urso", "Slow Horses", "Shōgun"]

def clamp(v, a, b): return max(a, min(b, v))
def seg(t, a, b): return clamp((t - a) / (b - a), 0.0, 1.0)
def lerp(a, b, k): return a + (b - a) * k
def dec(k):                      # desaceleracao (entrada): cubic-bezier(.2,.8,.2,1) aproximada
    k = clamp(k, 0, 1); return 1 - (1 - k) ** 3.2
def acc(k):                      # aceleracao (saida)
    k = clamp(k, 0, 1); return k ** 2.4
def mola(k):                     # foco: chega com 4 % de repique e assenta
    k = clamp(k, 0, 1)
    if k >= 1: return 1.0
    return 1 - math.exp(-7.5 * k) * math.cos(10.5 * k) * (1 - k) - 0.0 * k
def tw(s, px, w=0.56): return len(s) * px * w

# ---------------------------------------------------------------- icones (traco 2, cantos redondos)
IC = {
 "bookmark": '<path d="M7 4h10a1 1 0 0 1 1 1v15l-6-4-6 4V5a1 1 0 0 1 1-1z"/>',
 "check": '<path d="M5 12.5l4.2 4.2L19 7"/>',
 "eye": '<path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12z"/><circle cx="12" cy="12" r="3"/>',
 "eyeoff": '<path d="M3 3l18 18"/><path d="M10.6 5.6A9.6 9.6 0 0 1 12 5.5c6 0 9.5 6.5 9.5 6.5a16 16 0 0 1-3 3.6M6.3 6.8C3.9 8.5 2.5 12 2.5 12S6 18.5 12 18.5c1.6 0 3-.4 4.2-1"/>',
 "rows": '<rect x="3.5" y="4.5" width="17" height="6" rx="1.5"/><rect x="3.5" y="13.5" width="17" height="6" rx="1.5"/>',
 "x": '<path d="M6 6l12 12M18 6L6 18"/>',
 "info": '<circle cx="12" cy="12" r="9"/><path d="M12 11v5M12 7.6v.4"/>',
 "users": '<circle cx="9" cy="8.5" r="3.2"/><path d="M3.5 19c.6-3 2.8-4.6 5.5-4.6s4.9 1.6 5.5 4.6"/><path d="M15.5 5.6a3 3 0 0 1 0 5.8M17.6 14.6c1.6.6 2.6 2 2.9 4.4"/>',
 "play": '<path d="M8 5.5v13l10.5-6.5z"/>',
 "search": '<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5"/>',
 "home": '<path d="M4 11l8-6.5 8 6.5V20H4z"/>',
 "compass": '<circle cx="12" cy="12" r="9"/><path d="M15.5 8.5l-2 5-5 2 2-5z"/>',
 "library": '<path d="M5 4v16M9.5 4v16M14 5l4.5 14.5"/>',
 "tv": '<rect x="3" y="5" width="18" height="12" rx="2"/><path d="M8 20.5h8"/>',
 "cal": '<rect x="3.5" y="5" width="17" height="15" rx="2"/><path d="M3.5 10h17M8 3v4M16 3v4"/>',
 "user": '<circle cx="12" cy="8.5" r="3.8"/><path d="M4.5 20c1-3.6 4-5.4 7.5-5.4s6.5 1.8 7.5 5.4"/>',
 "gear": '<circle cx="12" cy="12" r="3"/><path d="M12 3v2.5M12 18.5V21M3 12h2.5M18.5 12H21M5.6 5.6l1.8 1.8M16.6 16.6l1.8 1.8M5.6 18.4l1.8-1.8M16.6 7.4l1.8-1.8"/>',
 "mic": '<rect x="9" y="3.5" width="6" height="11" rx="3"/><path d="M5.5 11.5a6.5 6.5 0 0 0 13 0M12 18v3"/>',
 "phone": '<rect x="7" y="3" width="10" height="18" rx="2"/><path d="M11 18h2"/>',
 "hdr": '<rect x="3" y="6" width="18" height="12" rx="2"/><path d="M7 10v4M10 10v4M7 12h3M13 10h2a2 2 0 0 1 0 4h-2z"/>',
 "wave": '<path d="M4 10v4M8 7v10M12 4v16M16 7v10M20 10v4"/>',
 "spark": '<path d="M12 3v4M12 17v4M3 12h4M17 12h4M6 6l2.5 2.5M15.5 15.5L18 18M6 18l2.5-2.5M15.5 8.5L18 6"/>',
 "moon": '<path d="M19 14.5A7.5 7.5 0 0 1 9.5 5a7.5 7.5 0 1 0 9.5 9.5z"/>',
 "film": '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M7 4v16M17 4v16M3 9h4M3 15h4M17 9h4M17 15h4"/>',
 "layers": '<path d="M12 4l9 4.5-9 4.5-9-4.5z"/><path d="M3 13l9 4.5 9-4.5"/>',
 "badge": '<circle cx="12" cy="10" r="5.5"/><path d="M9 15l-1.5 6L12 19l4.5 2L15 15"/>',
 "edit": '<path d="M4 20h4L19 9l-4-4L4 16z"/>',
 "source": '<path d="M4 7h16M4 12h16M4 17h10"/>',
 "folder": '<path d="M3.5 6.5a1.5 1.5 0 0 1 1.5-1.5h4.5l2 2.5H19a1.5 1.5 0 0 1 1.5 1.5v8.5A1.5 1.5 0 0 1 19 19H5a1.5 1.5 0 0 1-1.5-1.5z"/>',
 "pin": '<path d="M9 4h6l-1 6 3.5 3.5h-11L10 10z"/><path d="M12 13.5V20"/>',
 "loader": '<path d="M12 3a9 9 0 1 0 9 9"/>',
}
def ico(n, sz=28, color="currentColor", sw=2):
    return (f'<svg width="{sz}" height="{sz}" viewBox="0 0 24 24" fill="none" stroke="{color}" stroke-width="{sw}" '
            f'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" style="display:block;flex:none">{IC[n]}</svg>')

# ---------------------------------------------------------------- primitivas
def box(x, y, w, h, style="", inner="", cls=""):
    c = f' class="{cls}"' if cls else ""
    return f'<div{c} style="position:absolute;left:{x:.1f}px;top:{y:.1f}px;width:{w:.1f}px;height:{h:.1f}px;{style}">{inner}</div>'
def txt(x, y, s, size, color=BONE, weight=500, extra="", w=None):
    ww = f"width:{w:.0f}px;" if w else "white-space:nowrap;"
    return (f'<div style="position:absolute;left:{x:.1f}px;top:{y:.1f}px;{ww}font-size:{size}px;line-height:1.18;'
            f'font-weight:{weight};color:{color};{extra}">{s}</div>')
def head(t, sub, sub_w=1720):
    return txt(96, 56, t, 44, BONE, 700, "letter-spacing:-.02em") + txt(96, 118, sub, 23, MUT, 400, w=sub_w)

# ---------------------------------------------------------------- cenas da Home (1920x1080)
ROW_Y, PW_, PH_, GAP = 560, 212, 318, 24
def poster_rect(i, x0=96, y=ROW_Y, w=PW_, h=PH_, gap=GAP):
    return (x0 + i * (w + gap), y, w, h)

def art(x, y, w, h, fig, r=12, label=None, extra="", lsize=None):
    lab = ""
    if label:
        ls = lsize or max(14, min(26, w * 0.1))
        lab = (f'<div style="position:absolute;left:{w*0.08:.0f}px;bottom:{h*0.08:.0f}px;right:{w*0.08:.0f}px;font-size:{ls:.0f}px;'
               f'font-weight:800;line-height:1.02;letter-spacing:-.01em;color:rgba(255,255,255,.92);text-transform:uppercase">{label}</div>')
    return box(x, y, w, h, f"border-radius:{r}px;background:{FIGS[fig % len(FIGS)]};overflow:hidden;{extra}", lab)

def topbar(alpha=1.0, active="home", clock=True, y=40):
    icons = ["search", "home", "compass", "library", "tv", "cal", "user", "gear"]
    o = box(96, y, 52, 52, f"border-radius:26px;background:#3a6fd8;opacity:{alpha}",
            f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:#fff;font-weight:700;font-size:20px">W</div>')
    x = 176
    pill_w = len(icons) * 56 + 16
    o += box(x, y, pill_w, 52, f"border-radius:26px;background:rgba(11,11,12,.72);box-shadow:0 0 0 1.5px rgba(255,255,255,.08);opacity:{alpha}")
    for i, n in enumerate(icons):
        cx = x + 8 + i * 56
        on = n == active
        st = f"color:{BONE if on else DIM};opacity:{alpha};display:flex;align-items:center;justify-content:center;"
        if on: st += "border-radius:22px;background:rgba(244,242,238,.12);"
        o += box(cx + 2, y + 4, 52, 44, st, ico(n, 24))
    if clock:
        o += box(1824 - 120, y, 120, 52, f"border-radius:26px;background:rgba(11,11,12,.72);box-shadow:0 0 0 1.5px rgba(255,255,255,.08);opacity:{alpha}",
                 f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:600;color:{BONE}">18:44</div>')
    return o

def hero(fig=0, title="DUNA: PARTE DOIS", h=620, button=True):
    o = box(0, 0, SW, h, f"background:{FIGS[fig]}")
    o += box(0, 0, SW, h, "background:linear-gradient(90deg,rgba(8,8,10,.92) 0%,rgba(8,8,10,.55) 38%,rgba(8,8,10,0) 70%),linear-gradient(0deg,#08080A 0%,rgba(8,8,10,0) 45%)")
    o += txt(96, 176, title, 64, BONE, 800, "letter-spacing:-.01em")
    o += txt(96, 258, "Filme · Ficção científica · 2024 · 2 h 46", 22, MUT, 500)
    o += txt(96, 300, "Paul Atreides se une a Chani e aos Fremen enquanto busca vingança contra os conspiradores que destruíram sua família.", 22, "#D8D5CF", 400, w=720)
    if button:
        o += box(96, 400, 176, 56, f"border-radius:28px;background:{BONE};display:flex;align-items:center;justify-content:center;gap:10px;color:{INK};font-weight:700;font-size:20px",
                 ico("play", 22, INK) + "Ver título")
    return o

def poster_row(focus=2, n=8, y=ROW_Y, label="Populares", dim_others=0.0, shift=None, fscale=1.06, ring=True, skip_focus=False):
    """shift: funcao(i) -> dx (as vizinhas se afastam)."""
    o = txt(96, y - 46, label, 26, BONE, 700)
    for i in range(n):
        x, yy, w, h = poster_rect(i, y=y)
        dx = shift(i) if shift else 0
        if i == focus:
            if skip_focus: continue
            sw, sh = w * fscale, h * fscale
            fx, fy = x + dx - (sw - w) / 2, yy - (sh - h) / 2
            ex = "box-shadow:0 0 0 3px #F4F2EE,0 18px 40px rgba(0,0,0,.55);" if ring else "box-shadow:0 18px 40px rgba(0,0,0,.55);"
            o += art(fx, fy, sw, sh, i, 14, NAMES[i % len(NAMES)], ex)
        else:
            op = 1 - dim_others
            o += art(x + dx, yy, w, h, i, 12, NAMES[i % len(NAMES)], f"opacity:{op:.3f}")
    return o

def focus_rect(i, y=ROW_Y, fscale=1.06):
    x, yy, w, h = poster_rect(i, y=y)
    sw, sh = w * fscale, h * fscale
    return (x - (sw - w) / 2, yy - (sh - h) / 2, sw, sh)

def next_row(y=ROW_Y + PH_ + 96, label="Continuar assistindo"):
    o = txt(96, y - 46, label, 26, BONE, 700)
    for i in range(5):
        o += art(96 + i * 408, y, 384, 216, i + 3, 12)
    return o

def home(focus=2, fig=0, bar=True, dim=0.0, shift=None, skip_focus=False):
    o = box(0, 0, SW, SH, "background:#08080A")
    o += hero(fig, h=600)
    o += poster_row(focus, dim_others=dim, shift=shift, skip_focus=skip_focus)
    o += next_row()
    if bar: o += topbar()
    return o

# ---------------------------------------------------------------- pagina .dc.html
def page(title, body, styles="", w=SW, h=SH):
    return f'''<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<title>{title}</title>
<script src="./support.js"></script>
</head>
<body>
<x-dc>
<helmet>
<link rel="preconnect" href="https://fonts.googleapis.com"><link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@400;500;600;700;800&amp;display=swap" rel="stylesheet">
<style>
body{{margin:0;background:#000}}
*{{box-sizing:border-box}}
{styles}
</style>
</helmet>
<div style="width:{w}px;height:{h}px;position:relative;overflow:hidden;background:#000;color:{BONE};font-family:{FONT},system-ui,sans-serif;-webkit-font-smoothing:antialiased">{body}</div>
</x-dc>
<script type="text/x-dc" data-dc-script data-props='{{"$preview":{{"width":{w},"height":{h}}}}}'>
class Component extends DCLogic {{
renderVals() {{
return {{}};
}}
}}
</script>
</body>
</html>
'''

def frame(scene_html, x, y, w, label=None, note=None, note_w=None):
    s = w / SW; h = w * SH / SW
    o = ""
    if label: o += txt(x, y - 38, label, 20, MUT, 600, "letter-spacing:.04em")
    o += (f'<div style="position:absolute;left:{x}px;top:{y}px;width:{w}px;height:{h:.0f}px;overflow:hidden;border-radius:8px;box-shadow:0 0 0 1.5px rgba(255,255,255,.12)">'
          f'<div style="position:absolute;left:0;top:0;width:{SW}px;height:{SH}px;transform:scale({s:.5f});transform-origin:0 0;overflow:hidden">{scene_html}</div></div>')
    if note: o += txt(x, y + h + 10, note, 18, MUT, 400, w=note_w or w)
    return o

def crop(scene_html, cx, cy, cw, ch, scale, x, y, label=None, note=None):
    o = ""
    if label: o += txt(x, y - 38, label, 20, MUT, 600, "letter-spacing:.04em")
    ow, oh = cw * scale, ch * scale
    o += (f'<div style="position:absolute;left:{x}px;top:{y}px;width:{ow:.0f}px;height:{oh:.0f}px;overflow:hidden;border-radius:8px;box-shadow:0 0 0 1.5px rgba(255,255,255,.12)">'
          f'<div style="position:absolute;left:{-cx*scale:.1f}px;top:{-cy*scale:.1f}px;width:{SW}px;height:{SH}px;transform:scale({scale});transform-origin:0 0;overflow:hidden">{scene_html}</div></div>')
    if note: o += txt(x, y + oh + 10, note, 18, MUT, 400, w=ow)
    return o

# ---------------------------------------------------------------- motor de animacao
class Anim:
    """Elementos: (id, inner_html, base_style, fn(t)->dict). Props: x y w h o (opacidade) s (escala) r (raio)
    bg (cor) e extra (css estatico). O mesmo fn gera o quadro parado e os keyframes."""
    def __init__(self, prefix, cycle=CYCLE, steps=140):
        self.p, self.cycle, self.steps, self.els = prefix, cycle, steps, []
    def add(self, eid, fn, inner="", style=""):
        self.els.append((eid, fn, inner, style))
    @staticmethod
    def _css(d):
        s = f"left:{d['x']:.1f}px;top:{d['y']:.1f}px;width:{max(0,d['w']):.1f}px;height:{max(0,d['h']):.1f}px;opacity:{clamp(d.get('o',1),0,1):.3f};"
        if 'r' in d: s += f"border-radius:{d['r']:.1f}px;"
        s += f"transform:scale({d.get('s',1):.4f});"
        if 'bg' in d: s += f"background:{d['bg']};"
        return s
    def static(self, t):
        o = ""
        for eid, fn, inner, style in self.els:
            d = fn(t)
            o += f'<div style="position:absolute;{style};{self._css(d)}">{inner}</div>'
        return o
    def live(self):
        o, css = "", ""
        for k, (eid, fn, inner, style) in enumerate(self.els):
            name = f"{self.p}{k}"
            fr = []
            for j in range(self.steps + 1):
                t = self.cycle * j / self.steps
                fr.append(f"{100*j/self.steps:.2f}%{{{self._css(fn(t))}}}")
            css += f"@keyframes {name}{{{''.join(fr)}}}\n"
            o += f'<div style="position:absolute;{style};{self._css(fn(0))}animation:{name} {self.cycle}s linear infinite">{inner}</div>'
        return o, css

def live_frame(inner, x, y, w, label=None, note=None):
    s = w / SW; h = w * SH / SW
    o = ""
    if label: o += txt(x, y - 38, label, 20, MUT, 600, "letter-spacing:.04em")
    o += (f'<div style="position:absolute;left:{x}px;top:{y}px;width:{w}px;height:{h:.0f}px;overflow:hidden;border-radius:10px;box-shadow:0 0 0 1.5px rgba(255,255,255,.16)">'
          f'<div style="position:absolute;left:0;top:0;width:{SW}px;height:{SH}px;transform:scale({s:.5f});transform-origin:0 0;overflow:hidden">{inner}</div></div>')
    if note: o += txt(x, y + h + 10, note, 18, MUT, 400, w=w)
    return o

def timeline_bar(x, y, w, marks, cycle=CYCLE):
    """marks: lista (t0, t1, rotulo). Desenha a regua do ciclo."""
    o = box(x, y + 18, w, 2, f"background:rgba(255,255,255,.18)")
    for t0, t1, lab in marks:
        x0, x1 = x + w * t0 / cycle, x + w * t1 / cycle
        o += box(x0, y + 12, max(3, x1 - x0), 14, f"border-radius:7px;background:rgba(244,242,238,.28)")
        o += txt(x0, y + 34, lab, 15, MUT, 500)
    for s in range(int(cycle) + 1):
        o += txt(x + w * s / cycle - 6, y - 8, f"{s}s", 13, DIM, 500)
    return o
