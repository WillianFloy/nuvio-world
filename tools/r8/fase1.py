# FASE 1 (plano 25/11): pranchas animadas do que as pranchas aprovadas da secao 8
# nao mostram. Mesma linguagem (R8-1 fundacao, R8-2 movimento), mesmas curvas
# (dec/acc/mola de base.py), mesmos materiais. Cada peca e uma funcao do tempo.
import math
from base import *
from menus import row_label_html
import panels
from panels import SAVED, home_plain, C2, C2_ROWS, PILL

IC.update({
 "plus": '<path d="M12 5v14M5 12h14"/>',
 "bell": '<path d="M6 16V11a6 6 0 0 1 12 0v5l1.5 2h-15z"/><path d="M10 20a2 2 0 0 0 4 0"/>',
 "pulse": '<path d="M3 12h4l2-6 4 12 2-6h6"/>',
 "chevl": '<path d="M15 6l-6 6 6 6"/>',
 "chevr": '<path d="M9 6l6 6-6 6"/>',
 "palette": '<path d="M12 3a9 9 0 1 0 0 18c1 0 1.6-.8 1.3-1.7-.4-1.1.3-2.3 1.5-2.3H17a4 4 0 0 0 4-4c0-5.5-4-10-9-10z"/><circle cx="7.5" cy="11" r="1.2"/><circle cx="10.5" cy="7.5" r="1.2"/><circle cx="15" cy="7.5" r="1.2"/>',
 "gauge": '<path d="M4 15a8 8 0 1 1 16 0"/><path d="M12 15l4-5"/>',
 "trash": '<path d="M4 7h16M9 7V4h6v3M6 7l1 13h10l1-13"/>',
 "move": '<path d="M12 4v16M8 8l4-4 4 4M8 16l4 4 4-4"/>',
 "sub": '<rect x="3" y="6" width="18" height="13" rx="2"/><path d="M7 14h4M13 14h4M7 11h10"/>',
 "sun": '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4 12H2M22 12h-2M5 5l1.5 1.5M17.5 17.5L19 19M5 19l1.5-1.5M17.5 6.5L19 5"/>',
 "qr": '<rect x="4" y="4" width="6" height="6" rx="1"/><rect x="14" y="4" width="6" height="6" rx="1"/><rect x="4" y="14" width="6" height="6" rx="1"/><path d="M14 14h2v2h-2zM18 18h2v2h-2zM14 18h2M18 14h2"/>',
 "clock": '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
})

MAT = "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 80px rgba(0,0,0,.6)"
FOC = "background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE"
RING3 = "box-shadow:inset 0 0 0 3px #F4F2EE"

# ---------------------------------------------------------------- tempo
def oc(t, o0, c0, d_in=0.5, d_out=0.28):
    """abertura: 0 -> 1 (dec) a partir de o0; fecha (acc) a partir de c0."""
    if t < o0: return 0.0
    if t < c0: return dec(seg(t, o0, o0 + d_in))
    return 1 - acc(seg(t, c0, c0 + d_out))
def casc(t, i, o0, c0, step=0.045, start=0.14, d=0.22):
    a_ = seg(t, o0 + start + i * step, o0 + start + i * step + d)
    return a_ * (1 - seg(t, c0 - 0.02, c0 + 0.09))
def track(t, keys, d=0.27):
    """keys: [(t0, (v...)), ...]: cada mudanca chega na mola (270 ms)."""
    v = list(keys[0][1])
    for tk, val in keys[1:]:
        if t < tk: break
        p = mola(seg(t, tk, tk + d))
        v = [lerp(a_, b_, p) for a_, b_ in zip(v, val)]
    return v
def xf(t, keys, k):
    """troca de conteudo (160 ms, 12 px): peso e deslocamento do conteudo k.
    keys: [(t0, indice)], o primeiro com t0 = 0."""
    cur, prev, tk = keys[0][1], None, 0.0
    for tt, idx in keys[1:]:
        if t >= tt: prev, cur, tk = cur, idx, tt
    p = seg(t, tk, tk + 0.16) if prev is not None else 1.0
    if k == cur: return seg(p, 0.25, 1.0), 12 * (1 - dec(p))
    if k == prev: return 1 - seg(p, 0.0, 0.55), -12 * dec(p)
    return 0.0, 0.0

def center(inner, size=20, color=BONE, weight=700, gap=10):
    return (f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;gap:{gap}px;'
            f'font-size:{size}px;font-weight:{weight};color:{color};white-space:nowrap">{inner}</div>')
def avatar(letter, color, sz=40):
    return (f'<div style="width:{sz}px;height:{sz}px;border-radius:{sz/2}px;background:{color};display:flex;align-items:center;'
            f'justify-content:center;font-size:{sz*0.45:.0f}px;font-weight:700;color:#fff;flex:none">{letter}</div>')

# ================================================================ S1 · dados das abas reais
TABS = [("bookmark", "Salvos", "12"), ("pulse", "Atividade", ""), ("users", "Amigos", "3"), ("bell", "Avisos", "2"), ("cal", "Agenda", "4")]
# Conteudo de cada aba: (rotulo da estante, [(nome, apoio, progresso, fig, extra)], rotulo da segunda estante, banner)
ABAS = {
 0: ("Continuar", [(n, s, p, f, None) for n, s, p, f in SAVED], "Não começados",
     [(n.upper(), s, p, f) for n, s, p, f in SAVED]),
 1: ("Hoje", [("Um Sonho de Liberdade", "Gustavo te mandou · há 15 min", 0, 6, ("G", "#5a8a5f")),
              ("Breaking Bad", "Marina te mandou · 15:40", 0, 3, ("M", "#c0583a")),
              ("Whiplash", "Gustavo te mandou · há 2 dias", 0, 1, ("G", "#5a8a5f")),
              ("Stranger Things", "Carolina te mandou · há 4 dias", 0, 7, ("C", "#3d8a5a"))], "23/11",
     [("UM SONHO DE LIBERDADE", "Gustavo te mandou · há 15 min", 0, 6)]),
 2: ("Seus amigos", [("Marina", "Agora · O Urso · T3 E4", 0, 5, ("M", "#c0583a")),
                     ("Pedro", "Terminou e gostou · Project Hail Mary", 0, 2, ("P", "#3a6fd8")),
                     ("Gustavo", "Ontem · Severance", 0, 6, ("G", "#5a8a5f"))], "Recomendações",
     [("O URSO", "Marina está vendo agora · T3 E4", 0, 5)]),
 3: ("Avisos", [("Nova temporada", "Severance · T3 chega dia 8", 0, 2, ("bell", None)),
                ("Fonte indisponível", "Shōgun · use outra fonte", 0, 7, ("bell", None))], "",
     [("SEVERANCE", "Nova temporada chega dia 8", 0, 2)]),
 4: ("Esta semana", [("Severance", "Qui 8 · T3 E4 · Apple TV+", 0, 2, None), ("Fallout", "Qui 8 · T1 E6 · Prime", 0, 3, None),
                     ("O Urso", "Dom 11 · T2 E9 · Disney+", 0, 5, None), ("Andor", "Ter 13 · T2 E3 · Disney+", 0, 4, None)], "Mais tarde",
     [("SEVERANCE", "Qui 8 · T3 E4 · Apple TV+", 0, 2)]),
}
TAB_Y, CARD_Y, CW, CH, CSTEP = 548, 632, 384, 216, 408

def tab_geo(i, edit=False):
    """x, w da aba i na fileira de abas da Estante."""
    x = 96
    for j, (ic, nm, n) in enumerate(TABS):
        w = tw(nm, 24, 0.58) + (36 if n else 0) + 44
        if j == i: return x, w
        x += w + 12
    return x, 56   # o lapis (editar abas) depois da ultima aba

CHIPS = [("ORDENAR", "Mais antigos"), ("AGRUPAR", "Progresso"), ("", "Categoria")]
# As pilulas a direita das abas mudam com a aba (o que hoje e a barra de cada aba).
TAB_CHIPS = {0: CHIPS, 1: [], 2: [("QUEM VÊ", "Ninguém"), ("", "Adicionar pessoas")], 3: [("", "Limpar avisos")],
             4: [("", "Agenda completa")]}
def chip_geo(i, lst=None):
    lst = CHIPS if lst is None else lst
    ws = [tw(v, 19, 0.6) + 48 if c else tw(v, 19, 0.6) + 80 for c, v in lst]
    x = 1824 - sum(ws) - 12 * (len(ws) - 1)
    for j in range(i): x += ws[j] + 12
    return x, ws[i]

def tab_html(i, on):
    ic, nm, n = TABS[i]
    return center(f'{nm}' + (f'<span style="font-size:18px;color:{MUT}">{n}</span>' if n else ""), 24, BONE if on else MUT, 700)
def chip_html(i, val=None, lst=None):
    lst = CHIPS if lst is None else lst
    cap, v = lst[i]
    v = val or v
    if not cap:
        icn = "plus" if v in ("Categoria", "Adicionar pessoas") else ("cal" if "Agenda" in v else "x")
        return center(ico(icn, 20) + v, 19, BONE, 600, 8)
    return (f'<div style="position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;line-height:1.1">'
            f'<div style="font-size:12px;font-weight:700;letter-spacing:.12em;color:{DIM}">{cap}</div>'
            f'<div style="font-size:18px;font-weight:600;color:{BONE}">{v}</div></div>')

def card_inner(it, k):
    nm, sub, pr, fig, ex = it
    o = ""
    if pr: o += box(16, 198, 352 * pr, 4, f"border-radius:2px;background:{BONE}")
    if ex and ex[1]:
        o += box(14, 160, 40, 40, "", avatar(ex[0], ex[1], 40))
    if ex and ex[0] == "bell":
        o += box(0, 0, CW, CH, "background:linear-gradient(0deg,rgba(8,8,10,.85),rgba(8,8,10,.2))",
                 box(22, 22, 40, 40, f"color:{BONE}", ico("bell", 34)) + txt(22, 140, nm, 26, BONE, 800, w=340))
    return o

def add_estante_aba(a, k, pm_, pc_, wfn, focus_fn, persons=False):
    """Cartoes e legendas da aba k (peso de troca wfn(t) -> (o, dx))."""
    lab, itens, lab2, ban = ABAS[k]
    for i, it in enumerate(itens):
        nm, sub, pr, fig, ex = it
        bx = 96 + i * CSTEP
        pessoa = k == 2
        def card(t, i=i, bx=bx):
            o, dx = wfn(t); f = focus_fn(t); w = max(0.0, 1 - abs(f - i)) if f is not None else 0
            return dict(x=bx + dx, y=lerp(1080, CARD_Y, pm_(t)) + 24 * (1 - pc_(t, i + 2)), w=CW, h=CH, o=pc_(t, i + 2) * o, s=1 + 0.06 * w, r=12)
        if pessoa:
            inner = center(avatar(ex[0], ex[1], 104), 20)
            a.add(f"c{k}_{i}", card, inner, f"background:{FIGS[fig]};transform-origin:50% 50%")
        else:
            a.add(f"c{k}_{i}", card, card_inner(it, k), f"background:{FIGS[fig]};transform-origin:50% 50%;overflow:hidden")
        def cap(t, i=i, bx=bx):
            o, dx = wfn(t); f = focus_fn(t); w = max(0.0, 1 - abs(f - i)) if f is not None else 0
            return dict(x=bx + dx, y=lerp(1080, CARD_Y, pm_(t)) + 236, w=CW, h=60, o=pc_(t, i + 2) * o * (0.55 + 0.45 * w))
        a.add(f"cap{k}_{i}", cap, txt(0, 0, nm, 22, BONE, 700) + txt(0, 30, sub, 18, MUT, 500, w=CW))
    if lab2:
        a.add(f"lab2_{k}", lambda t: dict(x=96 + wfn(t)[1], y=lerp(1080, 960, pm_(t)), w=600, h=40, o=pc_(t, 8) * wfn(t)[0]), txt(0, 0, lab2, 24, BONE, 700))

def add_banner(a, k, j, pm_, wfn, cyc_o=None):
    nm, sub, pr, fig = ABAS[k][3][j]
    def hb(t):
        o, dx = wfn(t)
        return dict(x=0, y=0, w=SW, h=640, o=pm_(t) * o, s=1.0 + 0.02 * (1 - o))
    a.add(f"hb{k}_{j}", hb, '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(8,8,10,.9),rgba(8,8,10,.2) 60%),linear-gradient(0deg,#08080A,rgba(8,8,10,0) 50%)"></div>',
          f"background:{FIGS[fig]};transform-origin:60% 40%")
    def ht(t):
        o, dx = wfn(t)
        return dict(x=96 + dx, y=200, w=1100, h=220, o=o * pm_(t))
    a.add(f"ht{k}_{j}", ht, txt(0, 0, nm, 60, BONE, 800) + txt(0, 84, sub, 24, MUT, 600)
          + (box(0, 132, 360, 5, "border-radius:3px;background:rgba(255,255,255,.18)") + box(0, 132, 360 * pr, 5, f"border-radius:3px;background:{BONE}") if pr else ""))

# ================================================================ 10.1 · S1 com as abas reais
F1_CYC = 9.0
def build_S1_abas():
    a = Anim("FA", cycle=F1_CYC)
    O0, C0 = 0.6, 8.2
    pm_ = lambda t: oc(t, O0, C0)
    pc_ = lambda t, i=0: casc(t, i, O0, C0)
    TK = [(0, 0), (3.3, 1), (4.1, 2), (4.9, 4)]
    a.add("bg", lambda t: dict(x=0, y=-60 * pm_(t), w=SW, h=SH, s=lerp(1, 0.96, pm_(t)), o=1 - 0.65 * pm_(t)), home_plain(False), "transform-origin:50% 0%")
    # banner: o item em foco (Salvos) ou o primeiro da aba mostrada
    def ban_w(k, j):
        def w(t):
            o, dx = xf(t, TK, k)
            if k == 0:
                f = s1_focus(t)
                ff = f if f is not None else 0
                o *= max(0.0, 1 - abs(ff - j))
            return o, dx
        return w
    # base escura do banner: na troca cruzada a Home nunca aparece por baixo
    a.add("banbase", lambda t: dict(x=0, y=0, w=SW, h=640, o=pm_(t), bg="#08080A"))
    for j in range(2): add_banner(a, 0, j, pm_, ban_w(0, j))
    for k in (1, 2, 4): add_banner(a, k, 0, pm_, ban_w(k, 0))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("panel", lambda t: dict(x=0, y=lerp(1080, 500, pm_(t)), w=SW, h=580, o=1 if pm_(t) > 0.01 else 0), "",
          "background:linear-gradient(180deg,rgba(8,8,10,0) 0%,rgba(8,8,10,.94) 90px,#08080A 140px)")
    # abas
    for i in range(len(TABS)):
        x, w = tab_geo(i)
        a.add(f"tab{i}", (lambda x, w, i: lambda t: dict(x=x, y=lerp(1080, TAB_Y, pm_(t)) + 14 * (1 - pc_(t, i * 0.5)), w=w, h=52, o=pc_(t, i * 0.5)))(x, w, i),
              tab_html(i, False))
        # aba ativa: o texto acende (sem anel quando o foco esta nos cartoes)
        a.add(f"tabon{i}", (lambda x, w, i: lambda t: dict(x=x, y=lerp(1080, TAB_Y, pm_(t)) + 14 * (1 - pc_(t, i * 0.5)), w=w, h=52,
                                                          o=pc_(t, i * 0.5) * xf(t, TK, i)[0]))(x, w, i), tab_html(i, True))
    x, w = tab_geo(len(TABS))
    a.add("tabedit", lambda t: dict(x=x, y=lerp(1080, TAB_Y, pm_(t)), w=52, h=52, o=pc_(t, 3)), center(ico("edit", 24), 20, MUT))
    for k, lst in TAB_CHIPS.items():
        for i in range(len(lst)):
            cx, cw = chip_geo(i, lst)
            a.add(f"chip{k}_{i}", (lambda cx, cw, i, k: lambda t: dict(x=cx + xf(t, TK, k)[1], y=lerp(1080, TAB_Y, pm_(t)), w=cw, h=52, r=26,
                                                                       o=pc_(t, 4 + i * 0.5) * xf(t, TK, k)[0]))(cx, cw, i, k),
                  chip_html(i, lst=lst), "background:rgba(255,255,255,.07)")
    for k in (0, 1, 2, 4):
        add_estante_aba(a, k, pm_, pc_, (lambda k: lambda t: xf(t, TK, k))(k), (lambda k: lambda t: s1_focus(t) if k == 0 else (0 if k == 4 and t > 5.7 else None))(k))
    # o anel de foco: cartao -> aba -> abas -> cartao
    tabs_r = [tab_geo(i) for i in range(len(TABS))]
    def ring_keys():
        c = lambda i: (96 + i * CSTEP - 12, CARD_Y - 7, CSTEP, 230, 16)
        tb = lambda i: (tabs_r[i][0] - 4, TAB_Y - 4, tabs_r[i][1] + 8, 60, 30)
        return [(0, c(0)), (1.7, c(1)), (2.5, tb(0)), (3.3, tb(1)), (4.1, tb(2)), (4.9, tb(4)), (5.7, c(0))]
    RK = ring_keys()
    def ring(t):
        x, y, w, h, r = track(t, RK)
        y += lerp(1080, CARD_Y, pm_(t)) - CARD_Y
        return dict(x=x, y=y, w=w, h=h, r=r, o=pc_(t, 3))
    a.add("ring", ring, "", RING3)
    return a

def s1_focus(t):
    """foco na estante dos Salvos: 0 -> 1 em 1,7 s; sobe para as abas em 2,5 s."""
    if t >= 2.5: return 1.0
    return mola(seg(t, 1.7, 1.97))

F1A_MARKS = [(0.6, 1.1, "abrir"), (1.7, 2.0, "foco"), (2.5, 2.8, "sobe às abas"), (3.3, 5.2, "troca de aba"), (5.7, 6.0, "desce"), (8.2, 8.48, "fechar")]

# ================================================================ 10.2 · S1 organizar (Agrupar e Nova categoria)
GRUPOS = ["Continuar e não começados", "Tipo: filmes, séries, canais", "Minhas categorias", "Sem agrupar"]
TIPO_FILMES = [("Duna: Parte Dois", "Faltam 1 h 12", 0.45, 0), ("Moana", "Filme · 2016", 0, 1), ("Project Hail Mary", "Filme · 2026", 0, 6),
               ("A Origem", "Filme · 2010", 0, 4), ("Whiplash", "Filme · 2014", 0, 3)]
def sheet_anim(a, pre, anchor, sheet, t_open, t_close, d_in=0.44, d_out=0.26):
    """uma folha pequena que nasce do retangulo `anchor` (largura primeiro, depois altura)."""
    def m(t):
        if t < t_open: return 0.0
        if t < t_close: return dec(seg(t, t_open, t_open + d_in))
        return 1 - acc(seg(t, t_close, t_close + d_out))
    def r(t):
        k = m(t); k1, k2 = dec(seg(k, 0, 0.55)), dec(seg(k, 0.15, 1))
        return dict(x=lerp(anchor[0], sheet[0], k1), y=lerp(anchor[1], sheet[1], k2), w=lerp(anchor[2], sheet[2], k1),
                    h=lerp(anchor[3], sheet[3], k2), r=lerp(26, 16, k2), o=1 if k > 0.005 else 0)
    a.add(pre + "sheet", r, "", MAT)
    return m

def build_S1_org():
    a = Anim("FO", cycle=F1_CYC)
    O0, C0 = 0.5, 8.2
    pm_ = lambda t: oc(t, O0, C0)
    pc_ = lambda t, i=0: casc(t, i, O0, C0)
    a.add("bg", lambda t: dict(x=0, y=-60 * pm_(t), w=SW, h=SH, s=lerp(1, 0.96, pm_(t)), o=1 - 0.65 * pm_(t)), home_plain(False), "transform-origin:50% 0%")
    a.add("banbase", lambda t: dict(x=0, y=0, w=SW, h=640, o=pm_(t), bg="#08080A"))
    GK = [(0, 0), (3.55, 1)]   # o agrupamento troca em 3,55 s
    add_banner(a, 0, 0, pm_, lambda t: (1.0, 0.0))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("panel", lambda t: dict(x=0, y=lerp(1080, 500, pm_(t)), w=SW, h=580, o=1 if pm_(t) > 0.01 else 0), "",
          "background:linear-gradient(180deg,rgba(8,8,10,0) 0%,rgba(8,8,10,.94) 90px,#08080A 140px)")
    for i in range(len(TABS)):
        x, w = tab_geo(i)
        a.add(f"tab{i}", (lambda x, w, i: lambda t: dict(x=x, y=lerp(1080, TAB_Y, pm_(t)), w=w, h=52, o=pc_(t, i * 0.5)))(x, w, i), tab_html(i, i == 0))
    x, w = tab_geo(len(TABS))
    a.add("tabedit", lambda t: dict(x=x, y=lerp(1080, TAB_Y, pm_(t)), w=52, h=52, o=pc_(t, 3)), center(ico("edit", 24), 20, MUT))
    for i in range(len(CHIPS)):
        cx, cw = chip_geo(i)
        if i == 1:
            for g, val in ((0, "Progresso"), (1, "Tipo")):
                a.add(f"chip1_{g}", (lambda cx, cw, g: lambda t: dict(x=cx, y=lerp(1080, TAB_Y, pm_(t)) + xf(t, GK, g)[1] * 0.5, w=cw, h=52, r=26,
                                                                     o=pc_(t, 4.5) * xf(t, GK, g)[0]))(cx, cw, g), chip_html(1, val), "")
            a.add("chip1bg", (lambda cx, cw: lambda t: dict(x=cx, y=lerp(1080, TAB_Y, pm_(t)), w=cw, h=52, r=26, o=pc_(t, 4.5)))(cx, cw), "", "background:rgba(255,255,255,.07)")
        else:
            a.add(f"chip{i}", (lambda cx, cw, i: lambda t: dict(x=cx, y=lerp(1080, TAB_Y, pm_(t)), w=cw, h=52, r=26, o=pc_(t, 4 + i * 0.5)))(cx, cw, i),
                  chip_html(i), "background:rgba(255,255,255,.07)")
    # estantes: Progresso (Continuar) -> Tipo (Filmes)
    def shelf(g, itens, lab2):
        for i, (nm, sub, pr, fig) in enumerate(itens):
            bx = 96 + i * CSTEP
            a.add(f"g{g}c{i}", (lambda bx, i, g: lambda t: dict(x=bx + xf(t, GK, g)[1], y=lerp(1080, CARD_Y, pm_(t)) + 24 * (1 - pc_(t, i + 2)), w=CW, h=CH,
                                                              o=pc_(t, i + 2) * xf(t, GK, g)[0] * (0.45 if 1.6 < t < 3.6 else 1.0), r=12))(bx, i, g),
                  box(16, 198, 352 * pr, 4, f"border-radius:2px;background:{BONE}") if pr else "", f"background:{FIGS[fig]}")
            a.add(f"g{g}t{i}", (lambda bx, i, g: lambda t: dict(x=bx + xf(t, GK, g)[1], y=lerp(1080, CARD_Y, pm_(t)) + 236, w=CW, h=60,
                                                              o=pc_(t, i + 2) * xf(t, GK, g)[0] * (0.45 if 1.6 < t < 3.6 else 0.8)))(bx, i, g),
                  txt(0, 0, nm, 22, BONE, 700) + txt(0, 30, sub, 18, MUT, 500))
        a.add(f"g{g}l", (lambda g: lambda t: dict(x=96 + xf(t, GK, g)[1], y=lerp(1080, 960, pm_(t)), w=600, h=40, o=pc_(t, 8) * xf(t, GK, g)[0]))(g), txt(0, 0, lab2, 24, BONE, 700))
    shelf(0, SAVED, "Não começados")
    shelf(1, TIPO_FILMES, "Séries")
    # o rotulo da primeira estante aparece no lugar das abas? Nao: a aba ativa diz "Salvos"; o grupo aparece na legenda da pilula.
    # folha de Agrupar: nasce da pilula
    cx, cw = chip_geo(1)
    anc = (cx, TAB_Y, cw, 52)
    SH_ = (cx + cw - 520, TAB_Y + 66, 520, 56 + 4 * 68 + 20)
    m = sheet_anim(a, "ag", anc, SH_, 1.6, 3.3)
    a.add("agh", lambda t: dict(x=SH_[0] + 24, y=SH_[1] + 18, w=400, h=24, o=seg(m(t), 0.6, 1)), txt(0, 0, "AGRUPAR POR", 15, DIM, 700, "letter-spacing:.12em"))
    for j, g in enumerate(GRUPOS):
        yy = SH_[1] + 56 + j * 68
        a.add(f"ago{j}", (lambda yy, j: lambda t: dict(x=SH_[0] + 12, y=yy + 8 * (1 - seg(m(t), 0.55 + j * 0.08, 0.85 + j * 0.08)), w=SH_[2] - 24, h=58,
                                                      o=seg(m(t), 0.55 + j * 0.08, 0.85 + j * 0.08)))(yy, j), row_label_html("rows", g, 21))
    for j in (0, 1):
        yy = SH_[1] + 56 + j * 68
        a.add(f"agc{j}", (lambda yy, j: lambda t: dict(x=SH_[0] + SH_[2] - 56, y=yy + 15, w=28, h=28,
                                                      o=seg(m(t), 0.8, 1) * (xf(t, [(0, 0), (3.0, 1)], j)[0])))(yy, j), f'<div style="color:{BONE}">{ico("check", 28)}</div>')
    # anel: pilula Agrupar -> opcao 1 -> opcao 2 -> (fecha) pilula -> pilula Categoria -> folha do teclado
    c2x, c2w = chip_geo(2)
    RK = [(0, (cx - 4, TAB_Y - 4, cw + 8, 60, 30)), (1.75, (SH_[0] + 12, SH_[1] + 56, SH_[2] - 24, 58, 16)),
          (2.45, (SH_[0] + 12, SH_[1] + 56 + 68, SH_[2] - 24, 58, 16)), (3.3, (cx - 4, TAB_Y - 4, cw + 8, 60, 30)),
          (4.6, (c2x - 4, TAB_Y - 4, c2w + 8, 60, 30)), (5.3, (1824 - 640 + 24 + 1 * 68, TAB_Y + 66 + 76 + 1 * 68, 60, 60, 14)), (5.75, (1824 - 640 + 24 + 8 * 68, TAB_Y + 66 + 76 + 0 * 68, 60, 60, 14)), (6.2, (1824 - 640 + 24 + 3 * 68, TAB_Y + 66 + 76 + 0 * 68, 60, 60, 14)), (6.65, (1824 - 640 + 24 + 0 * 68, TAB_Y + 66 + 76 + 2 * 68, 60, 60, 14)),
          (7.1, (1824 - 640 + 24, TAB_Y + 66 + 76 + 3 * 68, 592, 56, 28)), (7.55, (c2x - 4, TAB_Y - 4, c2w + 8, 60, 30))]
    def ring(t):
        x, y, w, h, r = track(t, RK)
        y += lerp(1080, CARD_Y, pm_(t)) - CARD_Y
        st = 1.0
        return dict(x=x, y=y, w=w, h=h, r=r, o=pc_(t, 3) * st)
    # Nova categoria: folha com o teclado (o mesmo da Busca) e o campo do nome
    anc2 = (c2x, TAB_Y, c2w, 52)
    SK = (1824 - 640, TAB_Y + 66, 640, 76 + 4 * 68 + 20)
    m2 = sheet_anim(a, "nc", anc2, SK, 5.0, 7.45)
    a.add("ncf", lambda t: dict(x=SK[0] + 24, y=SK[1] + 18, w=SK[2] - 48, h=44, r=12, o=seg(m2(t), 0.5, 1)), "", "background:rgba(255,255,255,.07)")
    nome = "Kids"; TT = [5.75, 6.2, 6.65, 7.1]
    a.add("ncph", lambda t: dict(x=SK[0] + 40, y=SK[1] + 27, w=400, h=30, o=seg(m2(t), 0.6, 1) * (1 - seg(t, TT[0], TT[0] + 0.05))), txt(0, 0, "Nome da categoria", 20, DIM, 500))
    for j in range(len(nome)):
        fim = TT[j + 1] if j + 1 < len(nome) else 99
        a.add(f"ncl{j}", (lambda j, fim: lambda t: dict(x=SK[0] + 40, y=SK[1] + 26, w=400, h=30,
                                                       o=seg(t, TT[j], TT[j] + 0.05) * (1 - seg(t, fim, fim + 0.05)) * seg(m2(t), 0.5, 1)))(j, fim),
              txt(0, 0, nome[:j + 1] + '<span style="display:inline-block;width:2px;height:22px;margin-left:3px;background:#F4F2EE;vertical-align:-3px"></span>', 21, BONE, 600))
    for rr, row in enumerate(["abcdefghi", "jklmnopqr", "stuvwxyz⌫", ]):
        for c, ch in enumerate(row):
            kx, ky = SK[0] + 24 + c * 68, SK[1] + 76 + rr * 68
            a.add(f"nck{rr}{c}", (lambda kx, ky, rr: lambda t: dict(x=kx, y=ky + 10 * (1 - seg(m2(t), 0.55 + rr * 0.1, 0.9 + rr * 0.1)), w=60, h=60, r=14,
                                                                  o=seg(m2(t), 0.55 + rr * 0.1, 0.9 + rr * 0.1)))(kx, ky, rr),
                  center(ch, 22, BONE, 600), "background:rgba(255,255,255,.06)")
    a.add("ncok", lambda t: dict(x=SK[0] + 24, y=SK[1] + 76 + 3 * 68, w=SK[2] - 48, h=56, r=28, o=seg(m2(t), 0.85, 1)),
          center(ico("check", 22, INK) + "Criar categoria", 20, INK, 700), f"background:{BONE}")
    a.add("ring", ring, "", RING3)
    # a nova pilula de categoria aparece depois de criar
    a.add("nccat", lambda t: dict(x=96, y=lerp(1080, 960, pm_(t)) - 2, w=300, h=40, o=0), "")
    return a
F1O_MARKS = [(0.5, 1.0, "abrir"), (1.6, 2.05, "Agrupar"), (2.45, 3.0, "Tipo"), (3.3, 3.75, "regrupa"), (5.0, 7.45, "Nova categoria"), (8.2, 8.48, "fechar")]

# ---------------------------------------------------------------- partes comuns da Estante
def estante_base(a, pm_, pc_, bar=True):
    a.add("bg", lambda t: dict(x=0, y=-60 * pm_(t), w=SW, h=SH, s=lerp(1, 0.96, pm_(t)), o=1 - 0.65 * pm_(t)), home_plain(False), "transform-origin:50% 0%")
    a.add("banbase", lambda t: dict(x=0, y=0, w=SW, h=640, o=pm_(t), bg="#08080A"))
    add_banner(a, 0, 0, pm_, lambda t: (1.0, 0.0))
    if bar: a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("panel", lambda t: dict(x=0, y=lerp(1080, 500, pm_(t)), w=SW, h=580, o=1 if pm_(t) > 0.01 else 0), "",
          "background:linear-gradient(180deg,rgba(8,8,10,0) 0%,rgba(8,8,10,.94) 90px,#08080A 140px)")
def estante_cartoes(a, pm_, pc_, dim=lambda t: 1.0, dy=lambda t: 0.0):
    for i, (nm, sub, pr, fig) in enumerate(SAVED):
        bx = 96 + i * CSTEP
        a.add(f"sc{i}", (lambda bx, i: lambda t: dict(x=bx, y=lerp(1080, CARD_Y, pm_(t)) + dy(t) + 24 * (1 - pc_(t, i + 2)), w=CW, h=CH, r=12, o=pc_(t, i + 2) * dim(t)))(bx, i),
              box(16, 198, 352 * pr, 4, f"border-radius:2px;background:{BONE}"), f"background:{FIGS[fig]}")
        a.add(f"st{i}", (lambda bx, i: lambda t: dict(x=bx, y=lerp(1080, CARD_Y, pm_(t)) + dy(t) + 236, w=CW, h=60, o=pc_(t, i + 2) * 0.8 * dim(t)))(bx, i),
              txt(0, 0, nm, 22, BONE, 700) + txt(0, 30, sub, 18, MUT, 500))
    a.add("slab2", lambda t: dict(x=96, y=lerp(1080, 960, pm_(t)) + dy(t), w=600, h=40, o=pc_(t, 8) * dim(t)), txt(0, 0, "Não começados", 24, BONE, 700))

# ================================================================ 10.3 · S1 editar abas
def build_S1_editar():
    a = Anim("FE", cycle=F1_CYC)
    O0, C0 = 0.5, 8.2
    pm_ = lambda t: oc(t, O0, C0); pc_ = lambda t, i=0: casc(t, i, O0, C0)
    EDIT = lambda t: dec(seg(t, 1.85, 2.15)) * (1 - dec(seg(t, 6.2, 6.5)))
    estante_base(a, pm_, pc_)
    estante_cartoes(a, pm_, pc_, lambda t: 1 - 0.6 * EDIT(t), lambda t: 76 * EDIT(t))
    ORD = [(0, [0, 1, 2, 3, 4]), (3.3, [0, 1, 2, 4, 3]), (3.9, [0, 1, 4, 2, 3])]
    HID_T, GONE_T = 5.0, 6.2
    W = [tab_geo(i)[1] for i in range(5)]
    def xs(order, gone):
        x, out = 96, {}
        for i in order:
            out[i] = x
            if not (gone and i == 3): x += W[i] + 12
        return out, x
    keys = {i: [] for i in range(6)}
    stages = [(0, ORD[0][1], False), (3.3, ORD[1][1], False), (3.9, ORD[2][1], False), (GONE_T, ORD[2][1], True)]
    for tk, order, gone in stages:
        pos, end = xs(order, gone)
        for i in range(5): keys[i].append((tk, (pos[i],)))
        keys[5].append((tk, (end,)))
    ty = lambda t: lerp(1080, TAB_Y, pm_(t))
    for i in range(5):
        def tabr(t, i=i):
            x = track(t, keys[i])[0]
            w = W[i] * (1 - dec(seg(t, GONE_T, GONE_T + 0.3)) if i == 3 else 1)
            hid = seg(t, HID_T, HID_T + 0.2) if i == 3 else 0
            return dict(x=x, y=ty(t), w=w, h=52, r=26, o=pc_(t, i * 0.5) * (1 - 0.6 * hid) * (1 - seg(t, GONE_T, GONE_T + 0.2) if i == 3 else 1))
        a.add(f"etb{i}", tabr, "", "background:rgba(255,255,255,.07)")   # fundo de edicao (so aparece em edicao)
        a.add(f"tab{i}", tabr, tab_html(i, i == 0))
    # o fundo de edicao acende so durante a edicao
    for i in range(5):
        a.els[[e[0] for e in a.els].index(f"etb{i}")] = (lambda e, i: (e[0], (lambda f: lambda t: dict(f(t), o=f(t)["o"] * EDIT(t)))(e[1]), e[2], e[3]))(a.els[[e[0] for e in a.els].index(f"etb{i}")], i)
    a.add("eyeoff3", lambda t: dict(x=track(t, keys[3])[0] + W[3] + 6, y=ty(t) + 14, w=24, h=24, o=seg(t, HID_T, HID_T + 0.2) * (1 - seg(t, GONE_T, GONE_T + 0.15))),
          f'<div style="color:{BONE}">{ico("eyeoff", 22)}</div>')
    a.add("pen", lambda t: dict(x=track(t, keys[5])[0], y=ty(t), w=52, h=52, o=pc_(t, 3) * (1 - EDIT(t))), center(ico("edit", 24), 20, MUT))
    for i in range(len(CHIPS)):
        cx, cw = chip_geo(i)
        a.add(f"chip{i}", (lambda cx, cw, i: lambda t: dict(x=cx, y=ty(t), w=cw, h=52, r=26, o=pc_(t, 4 + i * 0.5) * (1 - EDIT(t))))(cx, cw, i),
              chip_html(i), "background:rgba(255,255,255,.07)")
    DONE = (1824 - 190, TAB_Y, 190, 52)
    a.add("done", lambda t: dict(x=DONE[0] + 20 * (1 - EDIT(t)), y=ty(t), w=DONE[2], h=52, r=26, o=EDIT(t)),
          center(ico("check", 22, INK) + "Concluir", 20, INK, 700, 8), f"background:{BONE}")
    a.add("hint", lambda t: dict(x=96, y=ty(t) + 70, w=1000, h=30, o=EDIT(t) * (1 - seg(t, 2.5, 2.7))),
          txt(0, 0, "Escolha uma aba para mover ou esconder", 19, MUT, 500))
    # acoes da aba em foco: nascem embaixo dela (mover para a esquerda, para a direita, esconder)
    ACTS = ["chevl", "chevr", "eyeoff"]
    def foco_aba(t):
        return 3 if t >= 4.4 else 4
    def acts_x(t):
        return track(t, [(0, (track(t, keys[4])[0],)), (4.4, (track(t, keys[3])[0],))])[0]
    ACT = lambda t: dec(seg(t, 2.4, 2.65)) * (1 - seg(t, 5.6, 5.8))
    for j, n in enumerate(ACTS):
        a.add(f"act{j}", (lambda j, n: lambda t: dict(x=acts_x(t) + j * 60, y=ty(t) + 66 + 10 * (1 - ACT(t)), w=52, h=52, r=26,
                                                      o=ACT(t) * seg(ACT(t), 0.2 + j * 0.2, 0.6 + j * 0.2) ** 0.5))(j, n),
              center(ico(n, 24), 20, BONE), "background:rgba(255,255,255,.09)")
    def ring(t):
        tab = lambda i: (track(t, keys[i])[0] - 4, TAB_Y - 4, W[i] + 8, 60, 30)
        act = lambda j: (acts_x(t) + j * 60 - 4, TAB_Y + 62, 60, 60, 30)
        kk = [(0, (96 - 12, CARD_Y - 7, CSTEP, 230, 16)), (1.3, tab(0)), (1.6, (track(t, keys[5])[0] - 4, TAB_Y - 4, 60, 60, 30)),
              (2.2, tab(4)), (2.8, act(0)), (4.4, tab(3)), (4.75, act(2)), (5.6, (DONE[0] - 4, TAB_Y - 4, DONE[2] + 8, 60, 30)),
              (6.6, (96 - 12, CARD_Y - 7, CSTEP, 230, 16))]
        x, y, w, h, r = track(t, kk)
        y += ty(t) - TAB_Y
        return dict(x=x, y=y, w=w, h=h, r=r, o=pc_(t, 3))
    a.add("ring", ring, "", RING3)
    # aperto: o anel encolhe 4% num toque
    for tp in (3.25, 3.85, 4.95, 6.15):
        pass
    return a
F1E_MARKS = [(0.5, 1.0, "abrir"), (1.6, 2.15, "editar abas"), (2.8, 4.2, "mover Agenda"), (4.75, 5.2, "esconder"), (5.75, 6.5, "concluir"), (8.2, 8.48, "fechar")]

# ================================================================ 10.4 · S1 vazia (sem salvos; aba sem conteudo)
def build_S1_vazia():
    a = Anim("FV", cycle=F1_CYC)
    O0, C0 = 0.5, 8.2
    pm_ = lambda t: oc(t, O0, C0); pc_ = lambda t, i=0: casc(t, i, O0, C0)
    TK = [(0, 0), (3.0, 1), (3.4, 2), (4.2, 3)]
    a.add("bg", lambda t: dict(x=0, y=-60 * pm_(t), w=SW, h=SH, s=lerp(1, 0.96, pm_(t)), o=1 - 0.65 * pm_(t)), home_plain(False), "transform-origin:50% 0%")
    a.add("veilv", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.86 * pm_(t), bg="#08080A"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    a.add("panel", lambda t: dict(x=0, y=lerp(1080, 500, pm_(t)), w=SW, h=580, o=1 if pm_(t) > 0.01 else 0), "",
          "background:linear-gradient(180deg,rgba(8,8,10,0) 0%,rgba(8,8,10,.94) 90px,#08080A 140px)")
    ty = lambda t: lerp(1080, TAB_Y, pm_(t))
    for i in range(len(TABS)):
        x, w = tab_geo(i)
        a.add(f"tab{i}", (lambda x, w, i: lambda t: dict(x=x, y=ty(t), w=w, h=52, o=pc_(t, i * 0.5)))(x, w, i),
              center(TABS[i][1], 24, MUT, 700))
        a.add(f"tabon{i}", (lambda x, w, i: lambda t: dict(x=x, y=ty(t), w=w, h=52, o=pc_(t, i * 0.5) * xf(t, TK, i)[0]))(x, w, i),
              center(TABS[i][1], 24, BONE, 700))
    VZ = {0: ("bookmark", "Nada salvo ainda", "Segure OK em qualquer título para salvar aqui.", "Explorar"),
          3: ("bell", "Nenhum aviso", "Novas temporadas e avisos das fontes aparecem aqui.", "")}
    for k, (ic, h, sub, btn) in VZ.items():
        w_ = lambda t, k=k: xf(t, TK, k)
        a.add(f"vi{k}", (lambda k: lambda t: dict(x=96 + xf(t, TK, k)[1], y=ty(t) + 104 + 16 * (1 - pc_(t, 3)), w=72, h=72, r=36,
                                                  o=pc_(t, 3) * xf(t, TK, k)[0]))(k), center(ico(ic, 34), 20, BONE), "background:rgba(255,255,255,.08)")
        a.add(f"vh{k}", (lambda k: lambda t: dict(x=196 + xf(t, TK, k)[1], y=ty(t) + 104 + 16 * (1 - pc_(t, 4)), w=1000, h=90,
                                                  o=pc_(t, 4) * xf(t, TK, k)[0]))(k), txt(0, 0, h, 34, BONE, 800) + txt(0, 46, sub, 21, MUT, 500))
        if btn:
            a.add(f"vb{k}", (lambda k: lambda t: dict(x=196 + xf(t, TK, k)[1], y=ty(t) + 224 + 16 * (1 - pc_(t, 5)), w=190, h=56, r=28,
                                                      o=pc_(t, 5) * xf(t, TK, k)[0]))(k), center(ico("compass", 22, INK) + btn, 20, INK, 700, 8), f"background:{BONE}")
    tabs_r = [tab_geo(i) for i in range(5)]
    tb = lambda i: (tabs_r[i][0] - 4, TAB_Y - 4, tabs_r[i][1] + 8, 60, 30)
    RK = [(0, (196 - 4, TAB_Y + 220, 198, 64, 32)), (2.6, tb(0)), (3.0, tb(1)), (3.4, tb(2)), (4.2, tb(3))]
    a.add("ring", lambda t: (lambda v: dict(x=v[0], y=v[1] + ty(t) - TAB_Y, w=v[2], h=v[3], r=v[4], o=pc_(t, 5)))(track(t, RK)), "", RING3)
    # as abas Atividade e Amigos passam rapido (sem conteudo carregado: pontos de espera, nao cartoes vazios)
    for k in (1, 2):
        a.add(f"ld{k}", (lambda k: lambda t: dict(x=96, y=ty(t) + 130, w=200, h=20, o=pc_(t, 3) * xf(t, TK, k)[0]))(k),
              '<div style="display:flex;gap:10px">' + "".join('<div style="width:12px;height:12px;border-radius:6px;background:rgba(244,242,238,.35)"></div>' for _ in range(3)) + '</div>')
    return a
F1V_MARKS = [(0.5, 1.0, "abrir vazia"), (2.6, 4.5, "troca de aba"), (8.2, 8.48, "fechar")]

# ================================================================ 10.5 · S2 (folha) com abas reais e Organizar
S2X, S2W = panels.S2_X, panels.S2_W
def s2_geo(relogio):
    return (112, 920) if relogio else (36, 996)
def build_S2_abas(relogio=True):
    a = Anim("F2" if relogio else "F3", cycle=F1_CYC)
    O0, C0 = 0.5, 8.2
    pm_ = lambda t: oc(t, O0, C0); pc_ = lambda t, i=0: casc(t, i, O0, C0)
    SY, SHH = s2_geo(relogio)
    a.add("bg", lambda t: dict(x=-30 * pm_(t), y=0, w=SW, h=SH, s=lerp(1, 0.985, pm_(t))), home_plain(False), "transform-origin:0% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.5 * pm_(t), bg="#000"))
    if relogio:
        a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library"))
    else:
        a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1), topbar(active="library", clock=False))
    a.add("sheet", lambda t: dict(x=S2X + 60 * (1 - pm_(t)), y=SY, w=S2W, h=SHH, r=22, o=seg(pm_(t), 0, 0.4)), "",
          "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),-30px 0 80px rgba(0,0,0,.55)")
    X0 = S2X + 36
    TK = [(0, 0), (2.4, 1), (3.0, 0)]
    HEAD = {0: ("Salvos", "12 títulos · 5 para retomar"), 1: ("Atividade", "4 novidades hoje")}
    for k, (h, sub) in HEAD.items():
        a.add(f"h{k}", (lambda k: lambda t: dict(x=X0 + xf(t, TK, k)[1], y=SY + 28 + 8 * (1 - pc_(t, 0)), w=560, h=90, o=pc_(t, 0) * xf(t, TK, k)[0]))(k),
              txt(0, 0, h, 40, BONE, 800) + txt(0, 54, sub, 19, MUT, 500))
    # abas: icones redondos (cabem 6 na folha); o nome so na aba ativa
    TI = [(ic, nm) for ic, nm, n in TABS] + [("edit", "")]
    TY = SY + 134
    def tabx(i, t):
        x = X0
        for j in range(i):
            on = xf(t, TK, j)[0] if j < 2 else 0
            x += 52 + 10 + (tw(TI[j][1], 19, .6) + 16) * on
        return x
    for i, (ic, nm) in enumerate(TI):
        def tb(t, i=i):
            on = xf(t, TK, i)[0] if i < 2 else 0
            return dict(x=tabx(i, t), y=TY, w=52 + (tw(nm, 19, .6) + 16) * on, h=52, r=26, o=pc_(t, 1 + i * 0.4))
        a.add(f"tb{i}", tb, f'<div style="position:absolute;left:14px;top:14px;color:{BONE}">{ico(ic, 24)}</div>'
              + (f'<div style="position:absolute;left:50px;top:0;bottom:0;display:flex;align-items:center;font-size:19px;font-weight:700;color:{BONE};white-space:nowrap;overflow:hidden">{nm}</div>' if nm else ""),
              "background:rgba(255,255,255,.07);overflow:hidden")
    # Organizar: pilulas (so em Salvos)
    OY = TY + 68
    ORD = (lambda: (X0, OY, tw("Mais antigos", 18, .6) + 120, 44))()
    a.add("ord", lambda t: dict(x=ORD[0], y=OY, w=ORD[2], h=44, r=22, o=pc_(t, 2) * xf(t, TK, 0)[0]),
          center(f'<span style="font-size:13px;letter-spacing:.12em;color:{DIM}">ORDENAR</span>' + "Mais antigos", 18, BONE, 600, 8), "background:rgba(255,255,255,.07)")
    a.add("ord2", lambda t: dict(x=ORD[0], y=OY, w=ORD[2], h=44, r=22, o=0), "")
    a.add("agr", lambda t: dict(x=ORD[0] + ORD[2] + 10, y=OY, w=190, h=44, r=22, o=pc_(t, 2.5) * xf(t, TK, 0)[0]),
          center(f'<span style="font-size:13px;letter-spacing:.12em;color:{DIM}">AGRUPAR</span>' + "Progresso", 18, BONE, 600, 8), "background:rgba(255,255,255,.07)")
    a.add("cat", lambda t: dict(x=ORD[0] + ORD[2] + 210, y=OY, w=52, h=44, r=22, o=pc_(t, 3) * xf(t, TK, 0)[0]), center(ico("plus", 22), 18), "background:rgba(255,255,255,.07)")
    # opcoes de Ordenar: abrem embaixo da pilula e empurram a lista
    OPTS = ["Mais antigos", "Mais recentes", "A a Z", "Ano"]
    OPEN = lambda t: dec(seg(t, 4.1, 4.45)) * (1 - dec(seg(t, 5.55, 5.85)))
    OPH = len(OPTS) * 56 + 16
    a.add("osh", lambda t: dict(x=X0, y=OY + 52, w=S2W - 72, h=OPH * OPEN(t), r=16, o=1 if OPEN(t) > 0.01 else 0), "", "background:rgba(255,255,255,.05)")
    for j, op in enumerate(OPTS):
        a.add(f"op{j}", (lambda j: lambda t: dict(x=X0 + 8, y=OY + 60 + j * 56, w=S2W - 88, h=52, o=seg(OPEN(t), 0.35 + j * 0.12, 0.75 + j * 0.12)))(j),
              txt(18, 13, op, 20, BONE, 600) + (f'<div style="position:absolute;right:18px;top:12px;color:{BONE}">{ico("check", 24)}</div>' if j == 0 else ""))
    # lista: linhas de 118 (a em foco expande, como na prancha aprovada); reordena em 5,4 s
    L0 = OY + 64
    ROW = 118
    order0 = [0, 1, 2, 3]; order1 = [3, 1, 0, 2]
    ITS = SAVED[:4]
    ACT = {1: [("Um Sonho de Liberdade", "Gustavo te mandou · há 15 min", 0, 6), ("Breaking Bad", "Marina te mandou · 15:40", 0, 3),
               ("Whiplash", "Gustavo te mandou · há 2 dias", 0, 1), ("Stranger Things", "Carolina te mandou", 0, 7)]}
    EXP = 470
    EF = lambda t: (1 - mola(seg(t, 1.8, 2.07))) if t < 3.6 else mola(seg(t, 6.2, 6.47))   # o item em foco expande (prancha S2)
    def order_at(t): return order0 if t < 5.4 else order1
    for k, lst in ((0, ITS), (1, ACT[1])):
        for i, (nm, sub, pr, fig) in enumerate(lst):
            def geo(t, i=i, k=k):
                if k == 1:
                    return L0 - 60 + i * (ROW + 10), 0.0
                pos = track(t, [(0, (order0.index(i),)), (5.4, (order1.index(i),))])[0]
                first = order_at(t)[0]
                e = EF(t)
                w = e if first == i else 0.0
                y = L0 + pos * (ROW + 10) + (EXP - ROW) * e * (1 if pos > 0.5 and first != i else 0)
                return y + OPH * OPEN(t) + 6 * (1 - pc_(t, 4 + i)), w
            def vis(y): return 1 - seg(y, SY + SHH - 150, SY + SHH - 100)
            def th(t, i=i, k=k, geo=geo):
                y, w = geo(t)
                return dict(x=X0 + xf(t, TK, k)[1], y=y, w=lerp(176, 568, w), h=lerp(99, 320, w), r=lerp(10, 14, w), o=pc_(t, 4 + i) * xf(t, TK, k)[0] * vis(y))
            a.add(f"th{k}{i}", th, (f'<div style="position:absolute;left:10px;bottom:8px;width:{88*pr:.0f}%;height:3px;border-radius:2px;background:{BONE}"></div>') if pr else "", f"background:{FIGS[fig]}")
            def lb(t, i=i, k=k, geo=geo):
                y, w = geo(t)
                return dict(x=lerp(X0 + 196, X0, w) + xf(t, TK, k)[1], y=lerp(y + 18, y + 334, w), w=520, h=70, o=pc_(t, 4 + i) * xf(t, TK, k)[0] * vis(y))
            a.add(f"lb{k}{i}", lb, txt(0, 0, nm, 23, BONE, 700) + txt(0, 32, sub, 18, MUT, 500))
            if k == 0:
                def ac(t, i=i, geo=geo):
                    y, w = geo(t)
                    return dict(x=X0, y=y + 404, w=568, h=52, o=seg(w, 0.6, 1) * vis(y + 300))
                a.add(f"ac{i}", ac, box(0, 0, 150, 52, f"border-radius:26px;background:{BONE}", center(ico("play", 20, INK) + "Retomar", 19, INK, 700, 8))
                      + box(162, 0, 150, 52, "border-radius:26px;box-shadow:inset 0 0 0 1.5px rgba(255,255,255,.25)", center("Detalhes", 19, BONE, 600))
                      + box(324, 0, 150, 52, "border-radius:26px;box-shadow:inset 0 0 0 1.5px rgba(255,255,255,.25)", center("Remover", 19, BONE, 600)))
    # recorte no pe da folha (a lista nao sai da folha)
    a.add("foot", lambda t: dict(x=S2X + 60 * (1 - pm_(t)), y=SY + SHH - 60, w=S2W, h=60, o=seg(pm_(t), 0, 0.4)), "",
          "background:linear-gradient(180deg,rgba(14,14,16,0),rgba(14,14,16,1));border-radius:0 0 22px 22px")
    tab = lambda i, t: (tabx(i, t) - 4, TY - 4, (52 + (tw(TI[i][1], 19, .6) + 16) * (xf(t, TK, i)[0] if i < 2 else 0)) + 8, 60, 30)
    def ring(t):
        rowk = lambda j: (X0 - 8, L0 - 8, S2W - 56, EXP + 16, 16)
        kk = [(0, rowk(0)), (1.8, tab(0, t)), (2.4, tab(1, t)), (3.0, tab(0, t)), (3.6, (ORD[0] - 4, OY - 4, ORD[2] + 8, 52, 26)),
              (4.6, (X0 + 4, OY + 60 + 56 - 2, S2W - 80, 56, 14)), (5.55, (ORD[0] - 4, OY - 4, ORD[2] + 8, 52, 26)), (6.2, rowk(0))]
        x, y, w, h, r = track(t, kk)
        return dict(x=x, y=y, w=w, h=h, r=r, o=pc_(t, 3))
    a.add("ring", ring, "", RING3)
    a.add("ordv", lambda t: dict(x=ORD[0], y=OY, w=ORD[2], h=44, r=22, o=pc_(t, 2) * seg(t, 5.5, 5.6) * xf(t, TK, 0)[0]),
          center(f'<span style="font-size:13px;letter-spacing:.12em;color:{DIM}">ORDENAR</span>' + "Mais recentes", 18, BONE, 600, 8), "background:#1c1c1f")
    return a
F2_MARKS = [(0.5, 1.0, "abrir"), (1.8, 3.2, "abas"), (3.6, 4.45, "Ordenar"), (4.6, 5.85, "escolhe · reordena"), (8.2, 8.48, "fechar")]

# ================================================================ C2 · partes comuns
def c2_rows(a, pre, X, Y, W, rows, o_fn, dx_fn=lambda t: 0.0):
    """linhas da Coluna (cabecalhos de grupo e linhas de 60 px); devolve os y das linhas."""
    y = Y; ys = []
    for j, (ic, lab, val) in enumerate(rows):
        if lab is None:
            a.add(f"{pre}h{j}", (lambda y, j: lambda t: dict(x=X + 16 + dx_fn(t), y=y + 10, w=300, h=24, o=o_fn(t, j)))(y, j), txt(0, 0, ic, 15, DIM, 700, "letter-spacing:.12em"))
            y += 44
        else:
            ys.append(y)
            a.add(f"{pre}r{j}", (lambda y, j: lambda t: dict(x=X + dx_fn(t), y=y + 8 * (1 - o_fn(t, j)), w=W, h=60, o=o_fn(t, j)))(y, j),
                  row_label_html(ic, lab, 21) + (f'<div style="position:absolute;right:18px;top:0;bottom:0;display:flex;align-items:center;font-size:19px;color:{MUT}">{val}</div>' if val else ""))
            y += 66
    return ys

def c2_card(a, m_fn, origem, alvo=C2):
    def card(t):
        m = m_fn(t); k1, k2 = dec(seg(m, 0, 0.55)), dec(seg(m, 0.15, 1))
        return dict(x=lerp(origem[0], alvo[0], k1), y=lerp(origem[1], alvo[1], k2), w=lerp(origem[2], alvo[2], k1), h=lerp(origem[3], alvo[3], k2), r=26, o=1 if m > 0.003 else 0)
    a.add("card", card, "", MAT)

# ================================================================ 10.6 · C2 editar atalhos e cor de destaque
EDIT_ROWS = [("film", "Qualidade", 1), ("hdr", "Dolby Vision", 1), ("spark", "HDR", 0), ("wave", "Dolby Atmos", 1), ("source", "Fonte de áudio", 1),
             ("sub", "Legendas", 0), ("play", "Trailer auto", 1), ("moon", "Descanso", 1), ("badge", "Selos em cor", 1), ("palette", "Cor de destaque", 0)]
SWATCH = ["#F4F2EE", "#FF4D6D", "#FF7A45", "#FFB020", "#E6D84A", "#9BD34A", "#3FCB7E", "#2BC4B6", "#36B3E8", "#3A7BFF", "#6A5CFF",
          "#9B5CFF", "#D45CFF", "#FF5CC8", "#C9A27A", "#8C8A85", "#5E7A8C", "#B4B1AB"]
def build_C2_editar():
    a = Anim("FC", cycle=F1_CYC)
    O0, C0 = 0.5, 8.3
    pm_ = lambda t: oc(t, O0, C0, 0.45)
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.985, pm_(t))), home_plain(False), "transform-origin:0% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.45 * pm_(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1 - 0.6 * pm_(t)), topbar(clock=False))
    c2_card(a, pm_, (PILL[0], PILL[1], PILL[2], PILL[3] + 8))
    PG = [(0, 0), (1.9, 1), (6.1, 0)]          # pagina: 0 Central, 1 Editar atalhos
    TRAY = lambda t: dec(seg(t, 4.1, 4.45)) * (1 - dec(seg(t, 5.55, 5.85)))
    cas = lambda t, j, k: casc(t, j, O0, C0, 0.03) * xf(t, PG, k)[0]
    a.add("clock", lambda t: dict(x=C2[0] + 32 + xf(t, PG, 0)[1], y=C2[1] + 26, w=300, h=60, o=seg(pm_(t), 0.3, 1) * xf(t, PG, 0)[0]), f'<div style="font-size:30px;font-weight:600;color:{BONE}">18:44</div>')
    a.add("date", lambda t: dict(x=C2[0] + 140 + xf(t, PG, 0)[1], y=C2[1] + 34, w=320, h=40, o=cas(t, 0, 0)), txt(0, 0, "Terça, 24 de nov.", 19, MUT, 500))
    ys0 = c2_rows(a, "m", C2[0] + 16, C2[1] + 96, C2[2] - 32, C2_ROWS, lambda t, j: cas(t, j, 0), lambda t: xf(t, PG, 0)[1])
    # pagina Editar atalhos
    a.add("eh", lambda t: dict(x=C2[0] + 32 + xf(t, PG, 1)[1], y=C2[1] + 28, w=460, h=60, o=xf(t, PG, 1)[0]),
          f'<div style="display:flex;align-items:center;gap:12px;color:{BONE}">{ico("chevl", 26)}<span style="font-size:28px;font-weight:800">Editar atalhos</span></div>')
    for n, tk in ((7, 0), (8, 3.25)):
        a.add(f"ecount{n}", (lambda n, tk: lambda t: dict(x=C2[0] + C2[2] - 110 + xf(t, PG, 1)[1], y=C2[1] + 38, w=80, h=30,
                                                       o=xf(t, PG, 1)[0] * (seg(t, tk, tk + 0.12) if tk else 1) * (1 - (seg(t, 3.25, 3.37) if not tk else 0))))(n, tk),
              f'<div style="text-align:right;font-size:19px;color:{MUT}">{n}/25</div>')
    EY = C2[1] + 100
    for j, (ic, lab, on) in enumerate(EDIT_ROWS):
        yy = EY + j * 66
        a.add(f"er{j}", (lambda yy, j: lambda t: dict(x=C2[0] + 16 + xf(t, PG, 1)[1], y=yy + 8 * (1 - seg(t, 1.95 + j * 0.03, 2.2 + j * 0.03)), w=C2[2] - 32, h=60,
                                                     o=xf(t, PG, 1)[0] * seg(t, 1.95 + j * 0.03, 2.2 + j * 0.03)))(yy, j), row_label_html(ic, lab, 21))
        if lab == "Legendas":
            for st, (icn, tk0, tk1) in enumerate([("plus", 0, 3.25), ("check", 3.25, 99)]):
                a.add(f"et{j}_{st}", (lambda yy, j, tk0, tk1: lambda t: dict(x=C2[0] + C2[2] - 72 + xf(t, PG, 1)[1], y=yy + 12, w=36, h=36, r=18,
                                                                       o=xf(t, PG, 1)[0] * seg(t, 1.95 + j * 0.03, 2.2 + j * 0.03) * seg(t, tk0, tk0 + 0.12 if tk0 else 0.01) * (1 - seg(t, tk1, tk1 + 0.12)),
                                                                       s=1 + 0.18 * (1 - mola(seg(t, tk0, tk0 + 0.27))) * (tk0 > 0)))(yy, j, tk0, tk1),
                      center(ico(icn, 22, INK if icn == "check" else BONE), 18), f"background:{BONE}" if icn == "check" else "background:rgba(255,255,255,.12)")
        else:
            a.add(f"et{j}", (lambda yy, j: lambda t: dict(x=C2[0] + C2[2] - 72 + xf(t, PG, 1)[1], y=yy + 12, w=36, h=36, r=18,
                                                         o=xf(t, PG, 1)[0] * seg(t, 1.95 + j * 0.03, 2.2 + j * 0.03)))(yy, j),
                  center(ico("check" if on else ("chevr" if lab == "Cor de destaque" else "plus"), 22, INK if on else BONE), 18),
                  f"background:{BONE}" if on else "background:rgba(255,255,255,.12)")
    # bandeja de cor: abre sob "Cor de destaque" (a linha 9) e empurra nada: e a ultima
    ty_ = EY + 9 * 66 + 66
    a.add("tray", lambda t: dict(x=C2[0] + 16, y=ty_, w=C2[2] - 32, h=(3 * 64 + 24) * TRAY(t), r=16, o=1 if TRAY(t) > 0.01 else 0), "", "background:rgba(255,255,255,.05)")
    for j, c in enumerate(SWATCH):
        cx, cy = C2[0] + 36 + (j % 6) * 76, ty_ + 14 + (j // 6) * 64
        a.add(f"sw{j}", (lambda cx, cy, j: lambda t: dict(x=cx, y=cy, w=52, h=52, r=26, o=seg(TRAY(t), 0.3 + (j // 6) * 0.15 + (j % 6) * 0.03, 0.7 + (j // 6) * 0.15 + (j % 6) * 0.03)))(cx, cy, j),
              "", f"background:{c}")
    # a cor escolhida tinge o acento da propria Central (o "check" de Legendas e o indicador)
    a.add("acc", lambda t: dict(x=C2[0] + C2[2] - 72, y=EY + 5 * 66 + 12, w=36, h=36, r=18, o=xf(t, PG, 1)[0] * seg(t, 5.3, 5.5)), center(ico("check", 22, INK), 18), "background:#36B3E8")
    # a lista rola para cima enquanto a bandeja esta aberta (a Coluna nao cresce)
    SC = lambda t: -150 * TRAY(t)
    def rolar(fn):
        def g(t):
            d = dict(fn(t)); d["y"] = d["y"] + SC(t)
            d["o"] = d.get("o", 1) * (1 - seg(C2[1] + 96 - d["y"], 0, 30))
            return d
        return g
    a.els = [(e, rolar(f) if e.startswith(("er", "et", "tray", "sw", "acc")) else f, i, st) for e, f, i, st in a.els]
    def ring(t):
        row0 = lambda yy: (C2[0] + 16, yy, C2[2] - 32, 60, 16)
        sw = lambda j: (C2[0] + 36 + (j % 6) * 76 - 6, ty_ + 14 + (j // 6) * 64 - 6, 64, 64, 32)
        kk = [(0, row0(ys0[0])), (1.2, row0(ys0[-1])), (2.0, row0(EY)), (2.7, row0(EY + 5 * 66)), (3.7, row0(EY + 9 * 66)),
              (4.5, sw(0)), (4.8, sw(6)), (5.1, sw(8)), (5.55, row0(EY + 9 * 66)), (6.0, row0(EY)), (6.3, row0(ys0[-1]))]
        x, y, w, h, r = track(t, kk)
        if 2.0 <= t < 6.0: y += SC(t)
        return dict(x=x, y=y, w=w, h=h, r=r, o=casc(t, 1, O0, C0))
    a.add("ring", ring, "", FOC)
    return a
F1C_MARKS = [(0.5, 0.95, "abrir"), (1.2, 2.2, "Editar atalhos"), (3.25, 3.5, "liga Legendas"), (4.1, 5.85, "cor de destaque"), (6.1, 6.3, "volta"), (8.3, 8.58, "fechar")]

# ================================================================ 10.7 · C2 no player (Tocando agora e Estatisticas)
PLAY_ROWS = [("AGORA", None, None), ("wave", "Áudio", "Inglês 5.1"), ("sub", "Legendas", "Português"), ("film", "Qualidade", "4K DV"),
             ("gauge", "Velocidade", "1x"), ("APP", None, None), ("sun", "Brilho da legenda", "80%"), ("pulse", "Estatísticas", "")]
STATS = [("Resolução", "3840 × 2160"), ("Vídeo", "HEVC Main10 · DV 8.1"), ("Áudio", "E-AC-3 JOC · 5.1"), ("Taxa", "18,2 Mbps"),
         ("Conexões", "4 · 31 Mbps"), ("Buffer", "32 s"), ("Quadros perdidos", "0"), ("Fonte", "Torrentio · RD")]
def player_bg(a, m_fn):
    a.add("video", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.985, m_fn(t))), "", f"background:{FIGS[2]};transform-origin:0% 50%")
    a.add("vveil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.15 + 0.35 * m_fn(t), bg="#000"))
    a.add("leg", lambda t: dict(x=0, y=920, w=SW - 620 * m_fn(t), h=60, o=1 - 0.4 * m_fn(t)),
          f'<div style="text-align:center;font-size:34px;font-weight:600;color:#fff;text-shadow:0 2px 6px #000">— Você está aqui há quanto tempo?</div>')
def build_C2_player():
    a = Anim("FP", cycle=F1_CYC)
    O0, C0 = 0.5, 8.3
    pm_ = lambda t: oc(t, O0, C0, 0.45)
    player_bg(a, pm_)
    # no player nao ha barra: a Coluna nasce do canto do relogio mesmo sem ele
    c2_card(a, pm_, (PILL[0], PILL[1], PILL[2], PILL[3] + 8))
    PG = [(0, 0), (3.6, 1), (6.4, 0)]
    cas = lambda t, j, k: casc(t, j, O0, C0, 0.03) * xf(t, PG, k)[0]
    X = C2[0]
    # cabecalho Tocando agora: miniatura, nome, barra
    a.add("np", lambda t: dict(x=X + 24 + xf(t, PG, 0)[1], y=C2[1] + 24, w=C2[2] - 48, h=150, o=cas(t, 0, 0)),
          box(0, 0, 140, 79, f"border-radius:10px;background:{FIGS[2]}") + txt(160, 0, "TOCANDO AGORA", 14, DIM, 700, "letter-spacing:.12em")
          + txt(160, 22, "Severance", 26, BONE, 800) + txt(160, 56, "T2 E4 · Mulher do Woe", 18, MUT, 500)
          + box(0, 104, C2[2] - 48, 4, "border-radius:2px;background:rgba(255,255,255,.15)") + box(0, 104, (C2[2] - 48) * 0.38, 4, f"border-radius:2px;background:{BONE}")
          + txt(0, 118, "21:04", 17, MUT, 500) + f'<div style="position:absolute;right:0;top:118px;font-size:17px;color:{MUT}">−38:12</div>')
    ys = c2_rows(a, "p", X + 16, C2[1] + 190, C2[2] - 32, PLAY_ROWS, lambda t, j: cas(t, j + 1, 0), lambda t: xf(t, PG, 0)[1])
    a.add("sh", lambda t: dict(x=X + 32 + xf(t, PG, 1)[1], y=C2[1] + 28, w=460, h=60, o=xf(t, PG, 1)[0]),
          f'<div style="display:flex;align-items:center;gap:12px;color:{BONE}">{ico("chevl", 26)}<span style="font-size:28px;font-weight:800">Estatísticas</span></div>')
    for j, (k_, v) in enumerate(STATS):
        a.add(f"st{j}", (lambda j: lambda t: dict(x=X + 32 + xf(t, PG, 1)[1], y=C2[1] + 104 + j * 58 + 8 * (1 - seg(t, 3.65 + j * 0.03, 3.9 + j * 0.03)), w=C2[2] - 64, h=50,
                                                 o=xf(t, PG, 1)[0] * seg(t, 3.65 + j * 0.03, 3.9 + j * 0.03)))(j),
              txt(0, 12, k_, 20, MUT, 500) + f'<div style="position:absolute;right:0;top:12px;font-size:20px;font-weight:600;color:{BONE}">{v}</div>'
              + box(0, 49, C2[2] - 64, 1, "background:rgba(255,255,255,.07)"))
    # o grafico da taxa (ultimos 30 s): barras que andam
    for b in range(30):
        a.add(f"gb{b}", (lambda b: lambda t: dict(x=X + 32 + b * 15.2, y=C2[1] + 104 + 8 * 58 + 40 + (60 - 60 * (0.45 + 0.4 * abs(math.sin(b * 0.7 + t * 2.0)))),
                                                  w=10, h=60 * (0.45 + 0.4 * abs(math.sin(b * 0.7 + t * 2.0))), r=2, o=xf(t, PG, 1)[0] * seg(t, 3.9, 4.2) * 0.8, bg=BONE))(b))
    row0 = lambda yy: (X + 16, yy, C2[2] - 32, 60, 16)
    RK = [(0, row0(ys[0])), (1.4, row0(ys[1])), (2.1, row0(ys[2])), (2.9, row0(ys[-1])), (3.6, (X + 20, C2[1] + 28, 200, 56, 16)), (6.4, row0(ys[-1]))]
    a.add("ring", lambda t: (lambda v: dict(x=v[0], y=v[1], w=v[2], h=v[3], r=v[4], o=casc(t, 1, O0, C0)))(track(t, RK)), "", FOC)
    return a
F1P_MARKS = [(0.5, 0.95, "abrir no player"), (1.4, 2.9, "foco"), (3.6, 4.2, "Estatísticas"), (6.4, 6.6, "volta"), (8.3, 8.58, "fechar")]

# ================================================================ 10.8 · C2 nascendo do segurar CH+ (com e sem relogio)
from panels import hold_prog, hold_vis, hm, hc, H_MARKS
def build_hold_C2(relogio):
    a = Anim("FHR" if relogio else "FHO")
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.985, hm(t))), home_plain(False), "transform-origin:0% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.45 * hm(t), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=0, w=SW, h=140, o=1 - 0.6 * hm(t)), topbar(clock=False))
    if relogio:
        a.add("pill", lambda t: dict(x=PILL[0], y=PILL[1], w=PILL[2], h=PILL[3], r=26, o=1 if hm(t) < 0.02 else 0),
              f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:24px;font-weight:600;color:{BONE}">18:44</div>',
              "background:rgba(11,11,12,.72);box-shadow:0 0 0 1.5px rgba(255,255,255,.08)")
        a.add("fill", lambda t: dict(x=PILL[0] + 18, y=PILL[1] + PILL[3] - 8, w=(PILL[2] - 36) * hold_prog(t), h=3, r=2, o=hold_vis(t), bg=BONE))
        orig = (PILL[0], PILL[1], PILL[2], PILL[3] + 8)
    else:
        a.add("lbl", lambda t: dict(x=1824 - 160, y=44 + 6 * (1 - hold_vis(t)), w=160, h=26, o=hold_vis(t)),
              f'<div style="text-align:right;font-size:18px;font-weight:600;color:{BONE};letter-spacing:.02em">Central</div>')
        a.add("track", lambda t: dict(x=1824 - 160, y=78, w=160, h=3, r=2, o=hold_vis(t) * 0.9, bg="rgba(255,255,255,.22)"))
        a.add("fill", lambda t: dict(x=1824 - 160 * hold_prog(t), y=78, w=160 * hold_prog(t), h=3, r=2, o=hold_vis(t), bg=BONE))
        orig = (1824 - 160, 72, 160, 14)    # sem relogio, a Coluna nasce do fio cheio
    c2_card(a, hm, orig)
    if relogio:
        def clock(t):
            k = dec(seg(hm(t), 0.1, 1))
            return dict(x=lerp(PILL[0] + 26, C2[0] + 32, k), y=lerp(PILL[1] + 10, C2[1] + 26, k), w=300, h=60, o=1 if hm(t) > 0.02 else 0)
    else:
        def clock(t):
            return dict(x=C2[0] + 32, y=C2[1] + 26 + 8 * (1 - hc(t, 0)), w=300, h=60, o=hc(t, 0))
    a.add("clock", clock, f'<div style="font-size:30px;font-weight:600;color:{BONE}">18:44</div>')
    a.add("date", lambda t: dict(x=C2[0] + 140, y=C2[1] + 34, w=320, h=40, o=hc(t, 0)), txt(0, 0, "Terça, 24 de nov.", 19, MUT, 500))
    ys = c2_rows(a, "h", C2[0] + 16, C2[1] + 96, C2[2] - 32, C2_ROWS, lambda t, j: hc(t, j, 0.03))
    a.add("pillf", lambda t: dict(x=C2[0] + 16, y=ys[0], w=C2[2] - 32, h=60, r=16, o=hc(t, 1)), "", FOC)
    return a

# ================================================================ 10.9 · Busca (CH-) abrindo do meio
CP = (728, 40, 464, 52)         # a pilula nasce no meio, em cima (linha da barra), descendo do topo
CP_Y0 = -64                     # comeca acima da tela
FLD = panels.FIELD
BODY_H = 900
# geometria da Busca: G muda com o modo flutuante (10.13/10.14): o campo nao encosta no topo
G = dict(flut=False, fld=FLD, icy=34, fty=48, veil=0.6, bdy=0)
def busca_geo(flut):
    if flut: G.update(flut=True, fld=(410, 40, 1100, 72), icy=54, fty=58, veil=0.8, bdy=68, body=(410, 124, 1100, 840))
    else: G.update(flut=False, fld=FLD, icy=34, fty=48, veil=0.6, bdy=0)
def busca_m(t, O0=0.8, C0=7.6):
    """progresso LINEAR da abertura (0,65 s); cada fase aplica a propria curva. Fecha voltando ao meio (0,3 s)."""
    if t < O0: return 0.0
    if t < C0: return seg(t, O0, O0 + 0.65)
    return 1 - seg(t, C0, C0 + 0.3)
def busca_base(a, m, typed_fn=None):
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.975, dec(m(t)))), home_plain(False), "transform-origin:50% 50%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=G["veil"] * dec(m(t)), bg="#000"))
    a.add("bar", lambda t: dict(x=0, y=-10 * dec(m(t)), w=SW, h=140, o=1 - seg(m(t), 0, 0.25)), topbar())
    FL = G["fld"]; flut = G["flut"]; BD = G.get("body")
    def corpo(t):
        k = dec(seg(m(t), 0.7, 1))
        if flut:   # cartao proprio, 12 px abaixo do campo: desdobra de cima para baixo
            return dict(x=BD[0], y=BD[1] - 10 * (1 - k), w=BD[2], h=lerp(60, BD[3], k), r=lerp(36, 28, k), o=seg(m(t), 0.7, 0.8))
        return dict(x=FLD[0], y=FLD[1] + FLD[3] - 40, w=FLD[2], h=lerp(40, BODY_H, k), r=34, o=1 if m(t) > 0.7 else 0)
    a.add("corpo", corpo, "", MAT)
    a.add("sep", lambda t: dict(x=FLD[0] + 30, y=FLD[1] + FLD[3] - 2, w=(FLD[2] - 60) * dec(seg(m(t), 0.75, 1)), h=1.5, o=(0.5 if m(t) > 0.75 else 0) * (0 if flut else 1), bg="rgba(255,255,255,.18)"))
    def fase(mm):
        ap = dec(seg(mm, 0, 0.3))                        # desliza de cima para a linha da barra
        k1, k2 = dec(seg(mm, 0.25, 0.85)), dec(seg(mm, 0.35, 0.95))
        return ap, k1, k2, lerp(CP_Y0, CP[1], ap)
    def field(t):
        mm = m(t); ap, k1, k2, yb = fase(mm)
        return dict(x=lerp(CP[0], FL[0], k1), y=lerp(yb, FL[1], k2), w=lerp(CP[2], FL[2], k1), h=lerp(CP[3], FL[3], k2), r=lerp(26, FL[3] / 2 if flut else 34, k2),
                    o=seg(mm, 0, 0.12) if mm > 0.001 else 0)
    a.add("field", field, "", "background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.12),0 24px 60px rgba(0,0,0,.55)")
    def lupa(t):
        mm = m(t); ap, k1, k2, yb = fase(mm)
        return dict(x=lerp(CP[0] + 14, FL[0] + 30, k1), y=lerp(yb + 4, G["icy"], k2), w=44, h=44, o=seg(mm, 0, 0.12) if mm > 0.001 else 0, s=lerp(0.9, 1.1, k2))
    a.add("lupa", lupa, center(ico("search", 26), 20, BONE))
    # o texto de ajuda viaja junto e cresce de 20 para 26 px
    def ph(t):
        mm = m(t); ap, k1, k2, yb = fase(mm)
        hide = 1 - seg(t, typed_fn[0], typed_fn[0] + 0.08) if typed_fn else 1
        return dict(x=lerp(CP[0] + 62, FL[0] + 92, k1), y=lerp(yb + 13, G["fty"], k2), w=700, h=40, o=seg(mm, 0.05, 0.2) * hide if mm > 0.001 else 0, s=lerp(0.77, 1, k2))
    a.add("ph", ph, txt(0, 0, "Buscar filmes, séries, pessoas e canais", 26, DIM, 500), "transform-origin:0% 0%")
    ca = lambda t, i: casc(t, i, 0.8 + 0.45, 7.6, 0.05)
    a.add("mic", lambda t: dict(x=FLD[0] + FLD[2] - 130, y=G["icy"], w=44, h=44, o=ca(t, 2)), center(ico("mic", 28), 20, BONE))
    a.add("phn", lambda t: dict(x=FLD[0] + FLD[2] - 70, y=G["icy"], w=44, h=44, o=ca(t, 3)), center(ico("qr", 28), 20, BONE))
    return ca
KEYROWS = "abcdef ghijkl mnopqr stuvwx yz1234 567890".split()
def teclado(a, ca, o_fn=lambda t: 1.0, dx=lambda t: 0.0):
    for r, row in enumerate(KEYROWS):
        for c, ch in enumerate(row):
            kx, ky = FLD[0] + 30 + c * 76, 150 + r * 76
            a.add(f"k{r}{c}", (lambda r, kx, ky: lambda t: dict(x=kx + dx(t), y=ky + 16 * (1 - ca(t, 4 + r * 0.7)), w=68, h=68, r=16, o=ca(t, 4 + r * 0.7) * o_fn(t)))(r, kx, ky),
                  center(ch, 26, BONE, 600), "background:rgba(255,255,255,.06)")
    for j, (lab, w) in enumerate([("espaço", 220), ("apagar", 112), ("limpar", 112)]):
        x = FLD[0] + 30 + [0, 228, 348][j]
        a.add(f"kx{j}", (lambda x, w: lambda t: dict(x=x + dx(t), y=150 + 6 * 76 + 16 * (1 - ca(t, 8.2)), w=w, h=60, r=16, o=ca(t, 8.2) * o_fn(t)))(x, w),
              center(lab, 19, MUT, 600), "background:rgba(255,255,255,.06)")
def key_rect(ch):
    for r, row in enumerate(KEYROWS):
        if ch in row: return (FLD[0] + 30 + row.index(ch) * 76, 150 + r * 76, 68, 68, 16)
RX = FLD[0] + 520
def resultados(a, t0, t1):
    """melhor resultado, titulos, pessoas e canais: chegam em cascata, secao a secao."""
    ap = lambda t, i: seg(t, t0 + i * 0.06, t0 + 0.25 + i * 0.06) * (1 - seg(t, t1, t1 + 0.1))
    def put(eid, i, x, y, w, h, inner="", style="", r=0):
        a.add(eid, (lambda i: lambda t: dict(x=x, y=y + 14 * (1 - ap(t, i)), w=w, h=h, r=r, o=ap(t, i)))(i), inner, style)
    put("rs0", 0, RX, 150, 300, 20, txt(0, 0, "MELHOR RESULTADO", 14, DIM, 700, "letter-spacing:.12em"))
    put("rb", 1, RX, 178, 560, 200, '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(8,8,10,.85),rgba(8,8,10,0) 70%)"></div>'
        + txt(24, 92, "Duna: Parte Dois", 30, BONE, 800) + txt(24, 134, "Filme · 2024 · 2 h 46", 18, MUT, 500), f"background:{FIGS[0]};overflow:hidden", 14)
    put("rs1", 2, RX, 400, 300, 20, txt(0, 0, "TÍTULOS", 14, DIM, 700, "letter-spacing:.12em"))
    for j, (nm, fig) in enumerate([("Duna", 4), ("Duna (1984)", 6), ("Duna: Profecia", 2), ("Dunkirk", 5)]):
        put(f"rt{j}", 3 + j, RX + j * 144, 428, 128, 192, "", f"background:{FIGS[fig]}", 10)
        put(f"rtl{j}", 3 + j, RX + j * 144, 626, 128, 24, txt(0, 0, nm, 16, MUT, 600, w=128))
    put("rs2", 7, RX, 668, 300, 20, txt(0, 0, "PESSOAS", 14, DIM, 700, "letter-spacing:.12em"))
    for j, (nm, c) in enumerate([("Denis Villeneuve", "#7a5a3a"), ("Zendaya", "#5a3a6a"), ("Austin Butler", "#3a5a6a")]):
        put(f"rp{j}", 8 + j, RX + j * 190, 696, 64, 64, center(nm[0], 26, "#fff", 700), f"background:{c}", 32)
        put(f"rpl{j}", 8 + j, RX + j * 190 + 74, 712, 120, 40, txt(0, 0, nm, 16, BONE, 600, w=120))
    put("rs3", 11, RX, 784, 300, 20, txt(0, 0, "CANAIS", 14, DIM, 700, "letter-spacing:.12em"))
    for j, nm in enumerate(["Dune TV 24h", "Sci-Fi Clássicos"]):
        put(f"rc{j}", 12 + j, RX + j * 250, 812, 236, 52, center(ico("tv", 20) + nm, 17, BONE, 600, 8), "background:rgba(255,255,255,.07)", 26)

def build_busca_meio(flut=False):
    busca_geo(flut)
    a = Anim("FB", cycle=F1_CYC)
    typed, TT = "duna", [2.4, 2.75, 3.1, 3.45]
    m = lambda t: busca_m(t)
    ca = busca_base(a, m, TT)
    teclado(a, ca)
    for j in range(len(typed)):
        fim = TT[j + 1] if j + 1 < len(typed) else 7.6
        a.add(f"ty{j}", (lambda j, fim: lambda t: dict(x=FLD[0] + 92, y=G["fty"], w=600, h=40, o=seg(t, TT[j], TT[j] + 0.04) * (1 - seg(t, fim, fim + 0.04))))(j, fim),
              txt(0, 0, typed[:j + 1] + '<span style="display:inline-block;width:2px;height:28px;margin-left:3px;background:#F4F2EE;vertical-align:-4px"></span>', 28, BONE, 600))
    resultados(a, 3.6, 7.6)
    RK = [(0, key_rect("a")), (2.15, key_rect("d")), (2.6, key_rect("u")), (2.95, key_rect("n")), (3.3, key_rect("a")),
          (4.6, (RX - 6, 172, 572, 212, 18)), (5.5, (RX - 6, 422, 140, 204, 14))]
    a.add("ring", lambda t: (lambda v: dict(x=v[0], y=v[1], w=v[2], h=v[3], r=v[4], o=ca(t, 4)))(track(t, RK)), "", RING3)
    return a
F1B_MARKS = [(0.8, 1.45, "desce e abre"), (1.5, 1.95, "teclado"), (2.4, 3.5, "digita"), (3.6, 4.4, "resultados"), (7.6, 7.9, "fechar")]

# ================================================================ 10.10 · Busca: historico, voz, celular e sem resultados
def qr_html(n=25, px=8, seed=7):
    import random
    rnd = random.Random(seed); cells = ""
    for y in range(n):
        for x in range(n):
            fin = any(x0 <= x < x0 + 7 and y0 <= y < y0 + 7 for x0, y0 in ((0, 0), (n - 7, 0), (0, n - 7)))
            if fin:
                xx, yy = min(x, abs(x - (n - 7)) if x >= n - 7 else x), 0
                lx = x if x < 7 else x - (n - 7); ly = y if y < 7 else y - (n - 7)
                on = lx in (0, 6) or ly in (0, 6) or (2 <= lx <= 4 and 2 <= ly <= 4)
            else:
                on = rnd.random() < 0.48
            if on: cells += f'<div style="position:absolute;left:{x*px}px;top:{y*px}px;width:{px}px;height:{px}px;background:#0B0B0C"></div>'
    return f'<div style="position:absolute;left:16px;top:16px;width:{n*px}px;height:{n*px}px">{cells}</div>'

def build_busca_modos(flut=False):
    busca_geo(flut)
    a = Anim("FM", cycle=F1_CYC)
    O0, C0 = 0.3, 8.4
    m = lambda t: busca_m(t, O0, C0)
    FT = [(0, 0), (3.25, 1), (6.3, 2), (6.5, 3), (6.7, 4)]       # texto do campo: vazio, "o urso", x, xq, xqz
    ca = busca_base(a, m, (3.25, 0))
    # area esquerda: 0 teclado, 1 voz, 2 celular
    LM = [(0, 0), (2.1, 1), (4.6, 2), (6.05, 0)]
    teclado(a, lambda t, i: casc(t, i, O0 + 0.45, C0, 0.05), lambda t: xf(t, LM, 0)[0], lambda t: xf(t, LM, 0)[1])
    for k, txt_ in ((1, "o urso"), (2, "x"), (3, "xq"), (4, "xqz")):
        a.add(f"ft{k}", (lambda k: lambda t: dict(x=FLD[0] + 92, y=G["fty"], w=600, h=40, o=xf(t, FT, k)[0] if t < C0 else 0))(k),
              txt(0, 0, txt_ + '<span style="display:inline-block;width:2px;height:28px;margin-left:3px;background:#F4F2EE;vertical-align:-4px"></span>', 28, BONE, 600))
    LX, LY, LW, LH = FLD[0] + 30, 150, 460, 520
    # voz
    vo = lambda t: xf(t, LM, 1)
    a.add("vcard", lambda t: dict(x=LX + vo(t)[1], y=LY, w=LW, h=LH, r=20, o=vo(t)[0]), "", "background:rgba(255,255,255,.05)")
    a.add("vring", lambda t: dict(x=LX + LW / 2 - 70 + vo(t)[1], y=LY + 60, w=140, h=140, r=70, o=vo(t)[0] * (0.25 + 0.2 * math.sin(t * 6)), s=1 + 0.08 * math.sin(t * 6)), "", f"background:{ACC}")
    a.add("vmic", lambda t: dict(x=LX + LW / 2 - 50 + vo(t)[1], y=LY + 80, w=100, h=100, r=50, o=vo(t)[0]), center(ico("mic", 44, INK), 20), f"background:{BONE}")
    a.add("vtx", lambda t: dict(x=LX + 30 + vo(t)[1], y=LY + 230, w=LW - 60, h=80, o=vo(t)[0] * (1 - seg(t, 3.25, 3.35))),
          f'<div style="text-align:center;font-size:28px;font-weight:800;color:{BONE}">Ouvindo…</div><div style="text-align:center;font-size:19px;color:{MUT};margin-top:6px">Fale agora</div>')
    a.add("vtx2", lambda t: dict(x=LX + 30 + vo(t)[1], y=LY + 230, w=LW - 60, h=80, o=vo(t)[0] * seg(t, 3.25, 3.35)),
          f'<div style="text-align:center;font-size:28px;font-weight:800;color:{BONE}">“o urso”</div><div style="text-align:center;font-size:19px;color:{MUT};margin-top:6px">Entendido</div>')
    a.add("vsug", lambda t: dict(x=LX + 30 + vo(t)[1], y=LY + 340, w=LW - 60, h=150, o=vo(t)[0]),
          txt(0, 0, "TENTE DIZER", 14, DIM, 700, "letter-spacing:.12em")
          + '<div style="position:absolute;top:30px;display:flex;flex-wrap:wrap;gap:10px">' + "".join(
              f'<div style="padding:10px 18px;border-radius:22px;background:rgba(255,255,255,.07);font-size:18px;font-weight:600;color:{BONE}">{s}</div>'
              for s in ("Filmes de ficção", "Séries com Zendaya", "Canal de notícias")) + '</div>')
    # celular (QR)
    qo = lambda t: xf(t, LM, 2)
    a.add("qcard", lambda t: dict(x=LX + qo(t)[1], y=LY, w=LW, h=LH, r=20, o=qo(t)[0]), "", "background:rgba(255,255,255,.05)")
    a.add("qr", lambda t: dict(x=LX + LW / 2 - 116 + qo(t)[1], y=LY + 36, w=232, h=232, r=16, o=qo(t)[0], s=lerp(0.94, 1, dec(seg(t, 4.6, 4.9)))), qr_html(), "background:#F4F2EE")
    a.add("qtx", lambda t: dict(x=LX + 30 + qo(t)[1], y=LY + 300, w=LW - 60, h=160, o=qo(t)[0]),
          f'<div style="text-align:center;font-size:26px;font-weight:800;color:{BONE}">Digite pelo celular</div>'
          f'<div style="text-align:center;font-size:19px;color:{MUT};margin-top:8px">Aponte a câmera para o código</div>'
          f'<div style="text-align:center;font-size:19px;color:{BONE};margin-top:18px;letter-spacing:.06em">código <b>7K2Q</b></div>')
    # direita: historico -> resultados de "o urso" -> nada encontrado
    RT = [(0, 0), (3.45, 1), (6.75, 2)]
    ro = lambda t, k: xf(t, RT, k)
    a.add("hh", lambda t: dict(x=RX + ro(t, 0)[1], y=150, w=500, h=20, o=ro(t, 0)[0] * seg(m(t), 0.8, 1)), txt(0, 0, "RECENTES", 14, DIM, 700, "letter-spacing:.12em"))
    for j, term in enumerate(["severance", "duna", "o urso", "andor"]):
        a.add(f"hr{j}", (lambda j: lambda t: dict(x=RX + ro(t, 0)[1], y=180 + j * 60 + 10 * (1 - casc(t, 5 + j, O0 + 0.45, C0)), w=540, h=52, r=14,
                                                   o=ro(t, 0)[0] * casc(t, 5 + j, O0 + 0.45, C0)))(j),
              f'<div style="position:absolute;inset:0;display:flex;align-items:center;gap:14px;padding-left:16px;font-size:21px;font-weight:600;color:{BONE}">{ico("clock", 22)}{term}</div>'
              f'<div style="position:absolute;right:16px;top:14px;color:{DIM}">{ico("x", 22)}</div>', "background:rgba(255,255,255,.04)")
    a.add("eh", lambda t: dict(x=RX + ro(t, 0)[1], y=444, w=500, h=20, o=ro(t, 0)[0] * casc(t, 9, O0 + 0.45, C0)), txt(0, 0, "EM ALTA", 14, DIM, 700, "letter-spacing:.12em"))
    for j, (nm, fig) in enumerate([("Severance", 2), ("O Urso", 5), ("Fallout", 3), ("Andor", 4)]):
        a.add(f"ea{j}", (lambda j, fig: lambda t: dict(x=RX + j * 144 + ro(t, 0)[1], y=472, w=128, h=192, r=10, o=ro(t, 0)[0] * casc(t, 10 + j, O0 + 0.45, C0)))(j, fig), "", f"background:{FIGS[fig]}")
    a.add("uh", lambda t: dict(x=RX + ro(t, 1)[1], y=150, w=500, h=20, o=ro(t, 1)[0]), txt(0, 0, "MELHOR RESULTADO", 14, DIM, 700, "letter-spacing:.12em"))
    a.add("ub", lambda t: dict(x=RX + ro(t, 1)[1], y=178, w=560, h=200, r=14, o=ro(t, 1)[0]),
          '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(8,8,10,.85),rgba(8,8,10,0) 70%)"></div>' + txt(24, 92, "O Urso", 30, BONE, 800) + txt(24, 134, "Série · 3 temporadas", 18, MUT, 500),
          f"background:{FIGS[5]};overflow:hidden")
    for j, fig in enumerate([5, 1, 7, 3]):
        a.add(f"ut{j}", (lambda j, fig: lambda t: dict(x=RX + j * 144 + ro(t, 1)[1], y=428 + 14 * (1 - seg(t, 3.5 + j * 0.06, 3.75 + j * 0.06)), w=128, h=192, r=10,
                                                       o=ro(t, 1)[0] * seg(t, 3.5 + j * 0.06, 3.75 + j * 0.06)))(j, fig), "", f"background:{FIGS[fig]}")
    a.add("nf", lambda t: dict(x=RX + ro(t, 2)[1], y=250, w=560, h=200, o=ro(t, 2)[0]),
          box(0, 0, 72, 72, "border-radius:36px;background:rgba(255,255,255,.08)", center(ico("search", 32), 20, MUT))
          + txt(0, 96, "Nada encontrado para “xqz”", 30, BONE, 800) + txt(0, 140, "Confira a ortografia ou busque por pessoa ou canal.", 19, MUT, 500, w=560))
    RK = [(0, key_rect("a")), (1.7, (FLD[0] + FLD[2] - 136, G["icy"] - 6 - G["bdy"], 56, 56, 28)), (4.2, (FLD[0] + FLD[2] - 76, G["icy"] - 6 - G["bdy"], 56, 56, 28)),
          (6.05, key_rect("x")), (6.42, key_rect("q")), (6.62, key_rect("z"))]
    a.add("ring", lambda t: (lambda v: dict(x=v[0], y=v[1], w=v[2], h=v[3], r=v[4], o=casc(t, 4, O0 + 0.45, C0)))(track(t, RK)), "", RING3)
    return a
F1M_MARKS = [(0.3, 0.95, "abrir · recentes"), (2.1, 3.5, "voz"), (4.6, 6.0, "celular"), (6.3, 6.9, "nada encontrado"), (8.4, 8.7, "fechar")]


FIXOS = ("bg", "veil", "bar", "corpo", "sep", "field", "lupa", "ph", "mic", "phn")
def deslocar_corpo(a):
    dy = G["bdy"]
    if not dy: return a
    def mv(fn):
        def g(t):
            d = dict(fn(t)); d["y"] = d["y"] + dy; return d
        return g
    a.els = [(e, f if (e in FIXOS or e.startswith(("ty", "ft"))) else mv(f), i, st) for e, f, i, st in a.els]
    return a
def build_busca_flut(): return deslocar_corpo(build_busca_meio(True))
def build_busca_modos_flut(): return deslocar_corpo(build_busca_modos(True))
