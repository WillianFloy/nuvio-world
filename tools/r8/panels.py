# CH+ (Salvos), CH+ segurado (Central), CH- (Busca) e o indicador de segurar CH+.
from base import *
from menus import row_label_html

P_OPEN0, P_OPEN1, P_CLOSE0, P_CLOSE1 = 0.8, 1.3, 5.4, 5.68
def pm(t):
    if t < P_OPEN0: return 0.0
    if t < P_CLOSE0: return dec(seg(t, P_OPEN0, P_OPEN1))
    return 1 - acc(seg(t, P_CLOSE0, P_CLOSE1))
def pf(t):   # foco: 0 -> 1 -> 2 -> 1
    return mola(seg(t, 2.2, 2.47)) + mola(seg(t, 2.9, 3.17)) - mola(seg(t, 3.7, 3.97))
def pc(t, i=0, step=0.045, start=0.14):
    a = seg(t, P_OPEN0 + start + i * step, P_OPEN0 + start + i * step + 0.22)
    return a * (1 - seg(t, P_CLOSE0 - 0.02, P_CLOSE0 + 0.09))
P_MARKS = [(0.8, 1.3, "abrir"), (2.2, 3.97, "foco"), (5.4, 5.68, "fechar")]

def home_plain(bar=True, fig=0):
    return box(0, 0, SW, SH, "background:#08080A") + hero(fig, h=600) + poster_row(2) + next_row() + (topbar() if bar else "")

SAVED = [("Severance", "T2 E4 · faltam 38 min", 0.62, 2), ("Fallout", "T1 E6 · faltam 51 min", 0.3, 3),
         ("Duna: Parte Dois", "Faltam 1 h 12", 0.45, 0), ("O Urso", "T3 E2 · faltam 22 min", 0.8, 5), ("Shōgun", "T1 E9 · faltam 44 min", 0.15, 7)]

# ================================================================ S1 · Estante (CH+ toque)
def build_S1():
    a = Anim("S1")
    a.add("bg", lambda t: dict(x=0, y=-60 * pm(t), w=SW, h=SH, s=lerp(1, 0.96, pm(t)), o=1 - 0.65 * pm(t)), home_plain(False), "transform-origin:50% 0%")
    # o topo vira a arte do salvo em foco (troca cruzada a cada foco)
    for k in range(3):
        fig = SAVED[k][3]
        def fo(t, k=k):
            f = pf(t); w = max(0.0, 1 - abs(f - k))
            return dict(x=0, y=0, w=SW, h=640, o=pm(t) * w, s=1.0 + 0.02 * (1 - w))
        a.add(f"hero{k}", fo, '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(8,8,10,.9),rgba(8,8,10,.2) 60%),linear-gradient(0deg,#08080A,rgba(8,8,10,0) 50%)"></div>',
              f"background:{FIGS[fig]};transform-origin:60% 40%")
        def tt(t, k=k):
            f = pf(t); w = max(0.0, 1 - abs(f - k) * 1.6)
            return dict(x=96 + 20 * (1 - w) * (1 if f < k else -1), y=200, w=1100, h=220, o=pc(t, 0) * w)
        a.add(f"heroT{k}", tt, txt(0, 0, SAVED[k][0].upper(), 60, BONE, 800) + txt(0, 84, SAVED[k][1], 24, MUT, 600)
              + box(0, 132, 360, 5, "border-radius:3px;background:rgba(255,255,255,.18)") + box(0, 132, 360 * SAVED[k][2], 5, f"border-radius:3px;background:{BONE}"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("panel", lambda t: dict(x=0, y=lerp(1080, 500, pm(t)), w=SW, h=580, o=1 if pm(t) > 0.01 else 0), "",
          "background:linear-gradient(180deg,rgba(8,8,10,0) 0%,rgba(8,8,10,.94) 90px,#08080A 140px)")
    tabs = [("Continuar", "6"), ("Minha lista", "12"), ("Assistidos", ""), ("Categorias", "")]
    x = 96
    for i, (tb, n) in enumerate(tabs):
        w = tw(tb, 24, 0.58) + (40 if n else 0) + 44
        st = "border-radius:26px;" + ("background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE;" if i == 0 else "")
        inner = (f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;gap:10px;font-size:24px;font-weight:700;color:{BONE if i == 0 else MUT}">{tb}'
                 + (f'<span style="font-size:18px;color:{MUT}">{n}</span>' if n else "") + "</div>")
        a.add(f"tab{i}", (lambda x, w, i: lambda t: dict(x=x, y=lerp(1080, 548, pm(t)) + 14 * (1 - pc(t, i)), w=w, h=52, o=pc(t, i)))(x, w, i), inner, st)
        x += w + 12
    for i, (nm, sub, pr, fig) in enumerate(SAVED):
        bx = 96 + i * 408
        def card(t, i=i, bx=bx):
            f = pf(t); w = max(0.0, 1 - abs(f - i)); s = 1 + 0.06 * w
            return dict(x=bx, y=lerp(1080, 632, pm(t)) + 24 * (1 - pc(t, i + 2)), w=384, h=216, o=pc(t, i + 2), s=s, r=12)
        a.add(f"card{i}", card, box(16, 198, 352 * pr, 4, f"border-radius:2px;background:{BONE}"), f"background:{FIGS[fig]};transform-origin:50% 50%")
        a.add(f"cap{i}", (lambda i, bx: lambda t: dict(x=bx, y=lerp(1080, 632, pm(t)) + 236, w=384, h=60, o=pc(t, i + 2) * (0.55 + 0.45 * max(0.0, 1 - abs(pf(t) - i)))))(i, bx),
              txt(0, 0, nm, 22, BONE, 700) + txt(0, 30, sub, 18, MUT, 500))
    def ring(t):
        f = pf(t); x = 96 + f * 408
        return dict(x=x - 12, y=lerp(1080, 632, pm(t)) - 7, w=408, h=230, o=pc(t, 3), r=16)
    a.add("ring", ring, "", "box-shadow:inset 0 0 0 3px #F4F2EE")
    a.add("shelf2", lambda t: dict(x=96, y=lerp(1080, 960, pm(t)), w=600, h=40, o=pc(t, 8)), txt(0, 0, "Minha lista", 24, BONE, 700))
    return a

# ================================================================ S2 · Coluna editorial
S2_X, S2_W = 1184, 640
def build_S2():
    a = Anim("S2")
    a.add("bg", lambda t: dict(x=-30 * pm(t), y=0, w=SW, h=SH, s=lerp(1, 0.985, pm(t))), home_plain(False), "transform-origin:0% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.5 * pm(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("sheet", lambda t: dict(x=S2_X + 60 * (1 - pm(t)), y=112, w=S2_W, h=920, r=22, o=seg(pm(t), 0, 0.4)), "",
          "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),-30px 0 80px rgba(0,0,0,.55)")
    a.add("h1", lambda t: dict(x=S2_X + 36, y=140 + 8 * (1 - pc(t, 0)), w=560, h=90, o=pc(t, 0)),
          txt(0, 0, "Salvos", 40, BONE, 800) + txt(0, 54, "18 títulos · 6 para retomar", 19, MUT, 500))
    for i, (tb, on) in enumerate([("Continuar", 1), ("Minha lista", 0), ("Assistidos", 0)]):
        w = tw(tb, 20, .6) + 40
        a.add(f"tb{i}", (lambda i, w, x: lambda t: dict(x=x, y=242, w=w, h=44, o=pc(t, 1 + i * 0.5)))(i, w, S2_X + 36 + sum(tw(q, 20, .6) + 52 for q, _ in [("Continuar", 1), ("Minha lista", 0), ("Assistidos", 0)][:i])),
              f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:20px;font-weight:700;color:{BONE if on else MUT}">{tb}</div>',
              "border-radius:22px;" + ("background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE" if on else ""))
    EXP, ROW = 470, 118
    def rowy(i, f):
        y = 312
        for j in range(i):
            w = max(0.0, 1 - abs(f - j)); y += lerp(ROW, EXP, w) + 10
        return y
    for i, (nm, sub, pr, fig) in enumerate(SAVED[:4]):
        def thumb(t, i=i):
            f = pf(t); w = max(0.0, 1 - abs(f - i)); y = rowy(i, f)
            tw_, th = lerp(176, 568, w), lerp(99, 320, w)
            return dict(x=S2_X + 36, y=y, w=tw_, h=th, r=lerp(10, 14, w), o=pc(t, 2 + i))
        a.add(f"th{i}", thumb, box(0, 0, 0, 0, ""), f"background:{FIGS[fig]}")
        def lab(t, i=i):
            f = pf(t); w = max(0.0, 1 - abs(f - i)); y = rowy(i, f)
            return dict(x=lerp(S2_X + 232, S2_X + 36, w), y=lerp(y + 18, y + 334, w), w=520, h=70, o=pc(t, 2 + i))
        a.add(f"lb{i}", lab, txt(0, 0, nm, 23, BONE, 700) + txt(0, 32, sub, 18, MUT, 500))
        def acts(t, i=i):
            f = pf(t); w = max(0.0, 1 - abs(f - i)); y = rowy(i, f)
            return dict(x=S2_X + 36, y=y + 404, w=568, h=52, o=pc(t, 2 + i) * seg(w, 0.6, 1))
        a.add(f"ac{i}", acts, box(0, 0, 150, 52, f"border-radius:26px;background:{BONE}", f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;gap:8px;color:{INK};font-weight:700;font-size:19px">{ico("play",20,INK)}Retomar</div>')
              + box(162, 0, 150, 52, "border-radius:26px;box-shadow:inset 0 0 0 1.5px rgba(255,255,255,.25)", f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-weight:600;font-size:19px;color:{BONE}">Detalhes</div>')
              + box(324, 0, 150, 52, "border-radius:26px;box-shadow:inset 0 0 0 1.5px rgba(255,255,255,.25)", f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-weight:600;font-size:19px;color:{BONE}">Remover</div>'))
    return a

# ================================================================ C1 · Faixa (CH+ segurado)
PILL = (1704, 40, 120, 52)
BAND = (96, 36, 1728, 236)
TILES = [("IMAGEM", [("film", "Qualidade", "Automática"), ("hdr", "Dolby Vision", "Ligado"), ("spark", "HDR", "Auto")]),
         ("SOM", [("wave", "Dolby Atmos", "Ligado"), ("source", "Fonte de áudio", "Automática")]),
         ("APP", [("play", "Trailer auto", "Ligado"), ("moon", "Descanso", "2 min"), ("badge", "Selos em cor", "Ligado")])]
def tile_html(ic, lab, val):
    return (f'<div style="position:absolute;left:18px;top:18px;color:{BONE}">{ico(ic, 28)}</div>'
            f'<div style="position:absolute;left:18px;bottom:44px;font-size:19px;font-weight:700;color:{BONE};white-space:nowrap">{lab}</div>'
            f'<div style="position:absolute;left:18px;bottom:18px;font-size:17px;font-weight:500;color:{MUT};white-space:nowrap">{val}</div>')

def central_band(a, m_fn=None, c_fn=None, foc=None):
    m_fn = m_fn or pm; c_fn = c_fn or pc; foc = foc or pf
    def band(t):
        m = m_fn(t)
        k1, k2 = dec(seg(m, 0, 0.7)), dec(seg(m, 0.25, 1))
        x = lerp(PILL[0], BAND[0], k1); y = lerp(PILL[1], BAND[1], k2)
        w = lerp(PILL[2], BAND[2], k1); h = lerp(PILL[3], BAND[3], k2)
        return dict(x=x, y=y, w=w, h=h, r=lerp(26, 28, k2), o=1 if m > 0.01 else 0)
    a.add("band", band, "", "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 80px rgba(0,0,0,.6)")
    # o relogio: sai da pilula e vira o cabecalho da faixa (mesmo texto, maior)
    def clock(t):
        m = m_fn(t); k = dec(seg(m, 0.1, 1))
        return dict(x=lerp(PILL[0] + 26, BAND[0] + 40, k), y=lerp(PILL[1] + 10, BAND[1] + 38, k), w=300, h=80, s=1, o=1 if m > 0.01 else 0)
    a.add("clock", clock, f'<div style="font-size:30px;font-weight:600;color:{BONE};transform-origin:0 0;white-space:nowrap">18:44</div>')
    a.add("date", lambda t: dict(x=BAND[0] + 40, y=BAND[1] + 86, w=300, h=80, o=c_fn(t, 6)),
          txt(0, 0, "Terça, 24 de nov.", 20, MUT, 500) + txt(0, 32, "Wi-Fi · Sala", 18, DIM, 500))
    # ladrilhos: os mais proximos do relogio (direita) entram primeiro
    x, idx, flat = 420, 0, []
    for gi, (gname, tiles) in enumerate(TILES):
        a.add(f"g{gi}", (lambda x, gi: lambda t: dict(x=x, y=BAND[1] + 30, w=200, h=24, o=c_fn(t, 7 - gi * 2)))(x, gi),
              txt(0, 0, gname, 15, DIM, 700, "letter-spacing:.12em"))
        for (ic, lab, val) in tiles:
            flat.append((x, ic, lab, val)); x += 160 + 12
        x += 28
    n = len(flat)
    for j, (tx, ic, lab, val) in enumerate(flat):
        def tile(t, j=j, tx=tx):
            f = 7 - foc(t); w = max(0.0, 1 - abs(f - j))
            return dict(x=tx, y=BAND[1] + 64 - 6 * w + 10 * (1 - c_fn(t, n - j, 0.035)), w=160, h=148, r=18, o=c_fn(t, n - j, 0.035))
        a.add(f"t{j}", tile, tile_html(ic, lab, val), "background:rgba(255,255,255,.05)")
    def ring(t):
        f = 7 - foc(t); tx = flat[0][0]
        # interpola entre ladrilhos (pode cruzar grupos)
        i = int(clamp(math.floor(f), 0, n - 2)); k = f - i
        xx = lerp(flat[i][0], flat[i + 1][0], k)
        return dict(x=xx - 4, y=BAND[1] + 60 - 6, w=168, h=156, r=21, o=c_fn(t, 2))
    a.add("ring", ring, "", "box-shadow:inset 0 0 0 3px #F4F2EE;background:rgba(244,242,238,.08)")

def build_C1():
    a = Anim("C1")
    a.add("bg", lambda t: dict(x=0, y=24 * pm(t), w=SW, h=SH, s=lerp(1, 0.985, pm(t))), home_plain(False), "transform-origin:50% 100%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.5 * pm(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=-14 * pm(t), w=SW, h=140, o=1 - seg(pm(t), 0, 0.5)), topbar(clock=False))
    a.add("pill", lambda t: dict(x=PILL[0], y=PILL[1], w=PILL[2], h=PILL[3], r=26, o=1 if pm(t) < 0.02 else 0),
          f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:600;color:{BONE}">18:44</div>',
          "background:rgba(11,11,12,.72);box-shadow:0 0 0 1.5px rgba(255,255,255,.08)")
    central_band(a)
    return a

# ================================================================ C2 · Coluna (CH+ segurado)
C2 = (1304, 36, 520, 900)
C2_ROWS = [("IMAGEM", None, None), ("film", "Qualidade", "Automática"), ("hdr", "Dolby Vision", "Ligado"), ("SOM", None, None),
           ("wave", "Dolby Atmos", "Ligado"), ("source", "Fonte de áudio", "Automática"), ("APP", None, None),
           ("play", "Trailer auto", "Ligado"), ("moon", "Descanso", "2 min"), ("badge", "Selos em cor", "Ligado"), ("edit", "Editar atalhos", "")]
def build_C2():
    a = Anim("C2")
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.985, pm(t))), home_plain(False), "transform-origin:0% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.45 * pm(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1 - 0.6 * pm(t)), topbar(clock=False))
    def card(t):
        m = pm(t); k1, k2 = dec(seg(m, 0, 0.55)), dec(seg(m, 0.15, 1))
        return dict(x=lerp(PILL[0], C2[0], k1), y=C2[1], w=lerp(PILL[2], C2[2], k1), h=lerp(PILL[3] + 8, C2[3], k2), r=26, o=1)
    a.add("card", card, "", "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 80px rgba(0,0,0,.6)")
    def clock(t):
        k = dec(seg(pm(t), 0.1, 1))
        return dict(x=lerp(PILL[0] + 26, C2[0] + 32, k), y=lerp(PILL[1] + 10, C2[1] + 26, k), w=300, h=60, o=1)
    a.add("clock", clock, f'<div style="font-size:30px;font-weight:600;color:{BONE}">18:44</div>')
    a.add("date", lambda t: dict(x=C2[0] + 140, y=C2[1] + 34, w=320, h=40, o=pc(t, 0)), txt(0, 0, "Terça, 24 de nov.", 19, MUT, 500))
    y = C2[1] + 96; ys = []
    for j, (ic, lab, val) in enumerate(C2_ROWS):
        if lab is None:
            a.add(f"h{j}", (lambda y, j: lambda t: dict(x=C2[0] + 32, y=y + 10, w=300, h=24, o=pc(t, j, 0.03)))(y, j), txt(0, 0, ic, 15, DIM, 700, "letter-spacing:.12em"))
            y += 44
        else:
            ys.append(y)
            a.add(f"r{j}", (lambda y, j: lambda t: dict(x=C2[0] + 16, y=y + 8 * (1 - pc(t, j, 0.03)), w=C2[2] - 32, h=60, o=pc(t, j, 0.03)))(y, j),
                  row_label_html(ic, lab, 21) + (f'<div style="position:absolute;right:18px;top:0;bottom:0;display:flex;align-items:center;font-size:19px;color:{MUT}">{val}</div>' if val else ""))
            y += 66
    def pill(t):
        f = pf(t); i = int(clamp(math.floor(f), 0, len(ys) - 2)); k = f - i
        return dict(x=C2[0] + 16, y=lerp(ys[i], ys[i + 1], k), w=C2[2] - 32, h=60, r=16, o=pc(t, 1))
    a.add("pill", pill, "", "background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE")
    return a

# ================================================================ Busca (CH-)
BAR0 = (176, 40, 464, 52)
FIELD = (410, -32, 1100, 128)
def build_busca():
    a = Anim("BU")
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.975, pm(t))), home_plain(False), "transform-origin:50% 30%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.55 * pm(t), bg="#000"))
    a.add("avatar", lambda t: dict(x=96 - 30 * pm(t), y=40, w=52, h=52, r=26, o=1 - seg(pm(t), 0, 0.4)), "", "background:#3a6fd8")
    a.add("clockp", lambda t: dict(x=1704 + 30 * pm(t), y=40, w=120, h=52, r=26, o=1 - seg(pm(t), 0, 0.4)),
          f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:600">18:44</div>', "background:rgba(11,11,12,.72)")
    # o corpo: desce do campo depois que ele assenta (o teclado e os resultados vivem nele)
    def corpo(t):
        m = pm(t); k = dec(seg(m, 0.35, 1))
        return dict(x=FIELD[0], y=FIELD[1] + FIELD[3] - 40, w=FIELD[2], h=lerp(40, 700, k), r=34, o=1 if m > 0.3 else 0)
    a.add("corpo", corpo, "", "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 80px rgba(0,0,0,.6)")
    a.add("sep", lambda t: dict(x=FIELD[0] + 30, y=FIELD[1] + FIELD[3] - 2, w=(FIELD[2] - 60) * dec(seg(pm(t), 0.4, 1)), h=1.5, o=0.5 if pm(t) > 0.4 else 0, bg="rgba(255,255,255,.18)"))
    def field(t):
        m = pm(t); k1, k2 = dec(seg(m, 0, 0.8)), dec(seg(m, 0.15, 1))
        return dict(x=lerp(BAR0[0], FIELD[0], k1), y=lerp(BAR0[1], FIELD[1], k2), w=lerp(BAR0[2], FIELD[2], k1), h=lerp(BAR0[3], FIELD[3], k2), r=lerp(26, 34, k2), o=1)
    a.add("field", field, "", "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.12),0 24px 60px rgba(0,0,0,.55)")
    icons = ["search", "home", "compass", "library", "tv", "cal", "user", "gear"]
    for i, n in enumerate(icons):
        if n == "search":
            def sic(t):
                k = dec(seg(pm(t), 0, 0.85))
                return dict(x=lerp(BAR0[0] + 10, FIELD[0] + 30, k), y=lerp(BAR0[1] + 4, 34, k), w=lerp(52, 44, k), h=44, o=1)
            a.add("sicon", sic, f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:{BONE}">{ico("search", 28)}</div>')
        else:
            a.add(f"ic{i}", (lambda i: lambda t: dict(x=BAR0[0] + 10 + i * 56 + 40 * pm(t), y=BAR0[1] + 4, w=52, h=44, o=1 - seg(pm(t), 0, 0.35)))(i),
                  f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:{DIM}">{ico(n, 24)}</div>')
    typed = "duna"
    def ph(t):
        return dict(x=FIELD[0] + 92, y=48, w=700, h=40, o=pc(t, 1) * (1 - seg(t, 2.1, 2.2)))
    a.add("ph", ph, txt(0, 0, "Buscar filmes, séries, pessoas e canais", 26, DIM, 500))
    for j in range(len(typed)):
        a.add(f"ty{j}", (lambda j: lambda t: dict(x=FIELD[0] + 92 + j * 17.5, y=48, w=30, h=40, o=seg(t, 2.2 + j * 0.16, 2.24 + j * 0.16) * (1 - seg(t, P_CLOSE0, P_CLOSE0 + 0.1))))(j),
              txt(0, 0, typed[j], 28, BONE, 600))
    a.add("mic", lambda t: dict(x=FIELD[0] + FIELD[2] - 130, y=34, w=44, h=44, o=pc(t, 2)), f'<div style="color:{BONE}">{ico("mic", 28)}</div>')
    a.add("phn", lambda t: dict(x=FIELD[0] + FIELD[2] - 70, y=34, w=44, h=44, o=pc(t, 3)), f'<div style="color:{BONE}">{ico("phone", 28)}</div>')
    # teclado: sobe do campo, em linhas
    keys = "abcdef ghijkl mnopqr stuvwx yz1234".split()
    for r, row in enumerate(keys):
        for c, ch in enumerate(row):
            kx, ky = FIELD[0] + 30 + c * 76, 150 + r * 76
            foc = (r == 0 and c == 3)
            a.add(f"k{r}{c}", (lambda r, c, kx, ky: lambda t: dict(x=kx, y=ky + 16 * (1 - pc(t, 3 + r * 0.7, 0.05)), w=68, h=68, r=16, o=pc(t, 3 + r * 0.7, 0.05)))(r, c, kx, ky),
                  f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:26px;font-weight:600;color:{BONE}">{ch}</div>',
                  "background:rgba(255,255,255,.06)" + (";box-shadow:inset 0 0 0 2px #F4F2EE" if foc else ""))
    # resultados: chegam em cascata quando ha texto
    for j in range(6):
        rx, ry = FIELD[0] + 520 + (j % 3) * 192, 150 + (j // 3) * 296
        def res(t, j=j, rx=rx, ry=ry):
            a_ = seg(t, 2.5 + j * 0.06, 2.75 + j * 0.06) * (1 - seg(t, P_CLOSE0 - 0.02, P_CLOSE0 + 0.08))
            return dict(x=rx, y=ry + 18 * (1 - a_), w=176, h=264, r=12, o=a_)
        a.add(f"res{j}", res, "", f"background:{FIGS[(j * 3) % len(FIGS)]}")
    return a

# ================================================================ Indicador de segurar CH+
# Ciclo: toque curto (cancelado) em 0,6-1,0 s; segurar valido 2,2 s -> completa em 2,85 s -> Central.
H_DEAD, H_FILL = 0.2, 0.45
def hold_prog(t):
    if 0.6 <= t < 1.0: return seg(t, 0.6 + H_DEAD, 0.6 + H_DEAD + H_FILL)
    if 1.0 <= t < 1.25: return seg(0.98, 0.6 + H_DEAD, 0.6 + H_DEAD + H_FILL) * (1 - dec(seg(t, 1.0, 1.22)))
    if 2.2 <= t < 2.2 + H_DEAD + H_FILL: return seg(t, 2.2 + H_DEAD, 2.2 + H_DEAD + H_FILL)
    if 2.85 <= t < 3.1: return 1.0
    return 0.0
def hold_vis(t):
    p = hold_prog(t)
    if 0.6 + H_DEAD <= t < 1.25: return min(1.0, seg(t, 0.8, 0.9)) * (1 - seg(t, 1.05, 1.25))
    if 2.2 + H_DEAD <= t < 3.0: return seg(t, 2.4, 2.5) * (1 - seg(t, 2.9, 3.0))
    return 0.0
def hm(t):   # a Central abre ao completar
    if t < 2.85: return 0.0
    if t < 5.4: return dec(seg(t, 2.85, 3.3))
    return 1 - acc(seg(t, 5.4, 5.68))
def hc(t, i=0, step=0.035, start=0.12):
    a_ = seg(t, 2.85 + start + i * step, 2.85 + start + i * step + 0.22)
    return a_ * (1 - seg(t, 5.38, 5.48))
H_MARKS = [(0.6, 1.0, "toque curto"), (2.2, 2.85, "segurar"), (2.95, 3.3, "abre"), (5.4, 5.68, "fecha")]

def build_hold(relogio):
    a = Anim("HR" if relogio else "HO")
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.985, hm(t))), home_plain(False), "transform-origin:50% 100%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.5 * hm(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1 - seg(hm(t), 0, 0.5)), topbar(clock=False))
    if relogio:
        a.add("pill", lambda t: dict(x=PILL[0], y=PILL[1], w=PILL[2], h=PILL[3], r=26, o=1 if hm(t) < 0.02 else 0),
              f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:600;color:{BONE}">18:44</div>',
              "background:rgba(11,11,12,.72);box-shadow:0 0 0 1.5px rgba(255,255,255,.08)")
        a.add("fill", lambda t: dict(x=PILL[0] + 18, y=PILL[1] + PILL[3] - 8, w=(PILL[2] - 36) * hold_prog(t), h=3, r=2, o=hold_vis(t), bg=BONE))
    else:
        # sem relogio: so o fio, onde o relogio estaria; nada de hora
        a.add("lbl", lambda t: dict(x=1824 - 160, y=44 + 6 * (1 - hold_vis(t)), w=160, h=26, o=hold_vis(t)),
              f'<div style="text-align:right;font-size:18px;font-weight:600;color:{BONE};letter-spacing:.02em">Central</div>')
        a.add("track", lambda t: dict(x=1824 - 160, y=78, w=160, h=3, r=2, o=hold_vis(t) * 0.9, bg="rgba(255,255,255,.22)"))
        a.add("fill", lambda t: dict(x=1824 - 160 * hold_prog(t), y=78, w=160 * hold_prog(t), h=3, r=2, o=hold_vis(t), bg=BONE))
    central_band(a, hm, hc, lambda t: 0 * t)
    return a
