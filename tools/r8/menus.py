# As tres direcoes do menu de segurar OK: A Desdobrar, B Trilho, C Folha.
from base import *

# ---------------- relogio comum das demonstracoes (7 s, em loop)
T_HOLD0, T_OPEN0, T_OPEN1, T_CLOSE0, T_CLOSE1 = 0.6, 1.3, 1.74, 5.3, 5.56
def hold(t): return seg(t, T_HOLD0, T_OPEN0) if t < T_OPEN0 + 0.12 else 0.0
def menu(t):
    if t < T_OPEN0: return 0.0
    if t < T_CLOSE0: return dec(seg(t, T_OPEN0, T_OPEN1))
    return 1 - acc(seg(t, T_CLOSE0, T_CLOSE1))
def fpos(t):
    return mola(seg(t, 2.45, 2.72)) + mola(seg(t, 3.1, 3.37)) - mola(seg(t, 3.7, 3.97))
def press(t): return math.sin(math.pi * seg(t, 4.1, 4.24))
def pend(t): return seg(t, 4.24, 4.74) if t < 4.76 else 0.0
def done(t): return seg(t, 4.74, 4.9) * (1 - seg(t, T_CLOSE0, T_CLOSE0 + 0.08)) if t >= 4.74 else 0.0
def content(t, i=0, step=0.05, start=0.12):
    """Opacidade do conteudo: entra em cascata depois da superficie, sai antes dela."""
    a = seg(t, T_OPEN0 + start + i * step, T_OPEN0 + start + i * step + 0.2)
    return a * (1 - seg(t, T_CLOSE0 - 0.02, T_CLOSE0 + 0.08))

# Foco de linha dos menus (dono, 26/11): raio 10, veu 10 %, contorno de 1,5 px a 50 %.
FOCO_LINHA = "background:rgba(244,242,238,.10);box-shadow:inset 0 0 0 1.5px rgba(244,242,238,.5)"
ACTIONS = [("bookmark", "Salvar"), ("eye", "Marcar como assistido"), ("rows", "Estilo da fileira")]
TL_MARKS = [(0.6, 1.3, "segurar"), (1.3, 1.74, "abrir"), (2.45, 3.97, "foco"), (4.1, 4.9, "OK → pendente → feito"), (5.3, 5.56, "fechar")]
FOCUS = 2
F = focus_rect(FOCUS)

def home_sem_fileira(fig=0):
    o = box(0, 0, SW, SH, "background:#08080A") + hero(fig, h=600) + next_row() + topbar()
    o += txt(96, ROW_Y - 46, "Populares", 26, BONE, 700)
    return o

def poster_inner(i, label=True):
    return ""  # a arte vem do estilo (background) do proprio elemento

def poster_style(i, ring=True):
    sh = "box-shadow:0 0 0 3px #F4F2EE,0 18px 40px rgba(0,0,0,.55);" if ring else ""
    return f"background:{FIGS[i % len(FIGS)]};overflow:hidden;{sh}transform-origin:50% 50%"

def poster_label(i, w):
    return (f'<div style="position:absolute;left:8%;right:8%;bottom:8%;font-size:{max(14,w*0.1):.0f}px;font-weight:800;line-height:1.02;'
            f'color:rgba(255,255,255,.92);text-transform:uppercase">{NAMES[i % len(NAMES)]}</div>')

def row_label_html(icon, label, size=23, color=BONE):
    return (f'<div style="position:absolute;inset:0;display:flex;align-items:center;gap:14px;padding:0 18px;color:{color};'
            f'font-size:{size}px;font-weight:600;white-space:nowrap">{ico(icon, 26)}{label}</div>')

def add_background(a, recede=0.985, veil=0.38, fig=0, sem_fileira=True):
    bg = home_sem_fileira(fig) if sem_fileira else home(FOCUS, fig, skip_focus=True)
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, recede, menu(t))), bg, "transform-origin:50% 60%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=veil * menu(t), bg="#000"))

def add_hold(a, rect):
    x, y, w, h = rect
    a.add("hold", lambda t: dict(x=x + 14, y=y + h - 12, w=(w - 28) * hold(t), h=4, r=2, o=1 if hold(t) > 0 else 0, bg=BONE))

# ================================================================ A · Desdobrar
A_PW = 470
def A_geo(rect=F, side=1):
    x, y, w, h = rect
    if side > 0: return dict(cx=x, cy=y, cw=w + A_PW, ch=h, px=x + w, side=1)
    return dict(cx=x - A_PW, cy=y, cw=w + A_PW, ch=h, px=x - A_PW, side=-1)

def build_A():
    a = Anim("A")
    a.add("bg", lambda t: dict(x=0, y=0, w=SW, h=SH, s=lerp(1, 0.99, menu(t))), home_sem_fileira(0), "transform-origin:50% 60%")
    a.add("veil", lambda t: dict(x=0, y=0, w=SW, h=SH, o=0.34 * menu(t), bg="#000"))
    g = A_geo()
    # as vizinhas: as da direita abrem espaco, todas escurecem
    for i in range(8):
        if i == FOCUS: continue
        x, y, w, h = poster_rect(i)
        dx = (lambda i: (lambda t: (A_PW + 0) * menu(t) if i > FOCUS else -18 * menu(t)))(i)
        a.add(f"n{i}", (lambda x, y, w, h, dx: lambda t: dict(x=x + dx(t), y=y, w=w, h=h, r=12, o=1 - 0.55 * menu(t)))(x, y, w, h, dx),
              poster_label(i, w), poster_style(i, False))
    # a superficie: nasce atras do cartaz e se desdobra para a direita
    a.add("surf", lambda t: dict(x=g["cx"], y=g["cy"], w=F[2] + A_PW * menu(t), h=g["ch"], r=16, o=1 if menu(t) > 0.01 else 0),
          "", f"background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 70px rgba(0,0,0,.6)")
    # o cartaz (o proprio item, nunca outra arte)
    a.add("poster", lambda t: dict(x=F[0], y=F[1], w=F[2], h=F[3], r=14, s=1 + 0.02 * menu(t) - 0.02 * menu(t)),
          poster_label(FOCUS, F[2]), poster_style(FOCUS, True))
    add_hold(a, F)
    px = g["px"]
    a.add("tit", lambda t: dict(x=px + 30, y=F[1] + 28 + 10 * (1 - content(t, 0)), w=A_PW - 60, h=40, o=content(t, 0)),
          f'<div style="font-size:30px;font-weight:700;color:{BONE};white-space:nowrap">{NAMES[FOCUS]}</div>')
    a.add("meta", lambda t: dict(x=px + 30, y=F[1] + 70 + 10 * (1 - content(t, 1)), w=A_PW - 60, h=30, o=content(t, 1)),
          f'<div style="font-size:19px;font-weight:500;color:{MUT};white-space:nowrap">Série · 2022 · 2 temporadas</div>')
    y0, pitch = F[1] + 124, 66
    a.add("pill", lambda t: dict(x=px + 16, y=y0 + pitch * fpos(t), w=A_PW - 32, h=56, r=10,
                                 o=content(t, 1), s=1 - 0.04 * press(t)),
          f'<div style="position:absolute;left:18px;right:18px;bottom:6px;height:3px;border-radius:2px;background:rgba(255,255,255,.12)"></div>',
          FOCO_LINHA)
    a.add("pend", lambda t: dict(x=px + 34, y=y0 + pitch * fpos(t) + 47, w=(A_PW - 68) * pend(t), h=3, r=2, o=1 if pend(t) > 0 else 0, bg=ACC))
    for i, (ic, lab) in enumerate(ACTIONS):
        if i == 1:
            a.add(f"r{i}", (lambda i: lambda t: dict(x=px + 16, y=y0 + pitch * i + 8 * (1 - content(t, i + 2)), w=A_PW - 32, h=56,
                                                        o=content(t, i + 2) * (1 - done(t))))(i), row_label_html(ic, lab))
            a.add("r1b", lambda t: dict(x=px + 16, y=y0 + pitch, w=A_PW - 32, h=56, o=done(t)), row_label_html("check", "Marcado como assistido"))
        else:
            a.add(f"r{i}", (lambda i: lambda t: dict(x=px + 16, y=y0 + pitch * i + 8 * (1 - content(t, i + 2)), w=A_PW - 32, h=56,
                                                        o=content(t, i + 2)))(i), row_label_html(ic, lab))
    return a

# ================================================================ B · Trilho
B_H, B_PAD, B_GAP = 68, 8, 6
def chip_w(lab, size=22): return 18 + 26 + 12 + tw(lab, size, 0.55) + 22
def B_geo(rect=F, actions=ACTIONS, above=False, x_min=96, x_max=1824):
    x, y, w, h = rect
    ws = [chip_w(l) for _, l in actions]
    W = sum(ws) + B_GAP * (len(ws) - 1) + 2 * B_PAD
    cx = x + w / 2
    rx = clamp(cx - W / 2, x_min, x_max - W)
    ry = (y - 18 - B_H) if above else (y + h + 18)
    xs, acc_ = [], rx + B_PAD
    for cw in ws: xs.append((acc_, cw)); acc_ += cw + B_GAP
    return dict(x=rx, y=ry, w=W, h=B_H, cx=cx, chips=xs, above=above)

def chip_html(ic, lab, color=BONE):
    return (f'<div style="position:absolute;inset:0;display:flex;align-items:center;gap:12px;padding:0 22px 0 18px;color:{color};'
            f'font-size:22px;font-weight:600;white-space:nowrap">{ico(ic, 26)}{lab}</div>')

def build_B():
    a = Anim("B")
    add_background(a, recede=0.99, veil=0.34, sem_fileira=False)
    g = B_geo()
    # o cartaz sobe um pouco, por cima do veu
    a.add("poster", lambda t: dict(x=F[0], y=F[1], w=F[2], h=F[3], r=14, s=1 + 0.03 * menu(t)), poster_label(FOCUS, F[2]), poster_style(FOCUS, True))
    add_hold(a, F)
    bx0 = g["cx"]
    def rail(t):
        m = menu(t)
        k1, k2 = dec(seg(m, 0, 0.45)), dec(seg(m, 0.2, 1))
        w = lerp(44, g["w"], k2); h = lerp(6, B_H, k1)
        x = lerp(bx0 - 22, g["x"], k2)
        y = lerp(F[1] + F[3] - 3, g["y"], k1)
        return dict(x=x, y=y, w=w, h=h, r=h / 2, o=1 if m > 0.01 else 0)
    a.add("rail", rail, "", "background:rgba(14,14,16,.96);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 24px 60px rgba(0,0,0,.6)")
    L = [cx for cx, cw in g["chips"]]; R = [cx + cw for cx, cw in g["chips"]]
    def interp(arr, p):
        i = int(clamp(math.floor(p), 0, len(arr) - 2)); f = p - i
        return lerp(arr[i], arr[i + 1], f)
    def pill(t):
        lead, trail = fpos(t), fpos(max(0, t - 0.07))
        l = interp(L, min(lead, trail)); r = interp(R, max(lead, trail))
        pr = press(t)
        return dict(x=l + 3 * pr, y=g["y"] + B_PAD + 2 * pr, w=r - l - 6 * pr, h=B_H - 2 * B_PAD - 4 * pr, r=(B_H - 2 * B_PAD) / 2, o=content(t, 1))
    a.add("pill", pill, "", "background:rgba(244,242,238,.14);box-shadow:inset 0 0 0 2px #F4F2EE")
    def pendline(t):
        l = interp(L, fpos(t)); r = interp(R, fpos(t))
        return dict(x=l + 22, y=g["y"] + B_H - B_PAD - 9, w=(r - l - 44) * pend(t), h=3, r=2, o=1 if pend(t) > 0 else 0, bg=ACC)
    a.add("pend", pendline)
    for i, ((ic, lab), (cx, cw)) in enumerate(zip(ACTIONS, g["chips"])):
        fn = (lambda i, cx, cw: lambda t: dict(x=cx, y=g["y"] + B_PAD + 6 * (1 - content(t, i + 1, 0.04, 0.2)), w=cw, h=B_H - 2 * B_PAD,
                                               o=content(t, i + 1, 0.04, 0.2) * ((1 - done(t)) if i == 1 else 1)))(i, cx, cw)
        a.add(f"c{i}", fn, chip_html(ic, lab))
        if i == 1:
            a.add("c1b", (lambda cx, cw: lambda t: dict(x=cx, y=g["y"] + B_PAD, w=cw, h=B_H - 2 * B_PAD, o=done(t)))(cx, cw), chip_html("check", "Assistido"))
    # o titulo do item acima do trilho? nao: o nome ja esta no cartaz; so a legenda do foco
    return a

# ================================================================ C · Folha
# A FOLHA E UMA GAVETA (dono, 26/11, depois do teste na TCL): o painel inteiro
# entra pela DIREITA, de fora da tela ate encostar na borda, e sai do mesmo
# jeito. Sempre a direita, qualquer que seja o cartaz. Nada voa do cartaz: a
# arte mora no topo do painel e anda com ele. Encostada na borda (sem margem,
# sem canto, sem contorno), com um degrade escurecendo a Home ate a borda dela,
# para o cartao que fica sob a beirada sumir no escuro em vez de aparecer
# cortado. O foco das acoes e mais leve e menos redondo (raio 10, veu 10 %,
# contorno de 1,5 px a 50 %).
C_W = 640
C_P = 48            # margem interna: titulo, meta, icones das acoes e sinopse
C_ARTE_H = 360
C_ESCURO = 280      # degrade da Home ate a borda da gaveta
C_Y0, C_PASSO, C_LH = 436, 64, 56
def C_geo(rect=F):
    return dict(x=SW - C_W, y=0, w=C_W, h=SH)

def build_C():
    a = Anim("C")
    add_background(a, recede=0.975, veil=0.5, sem_fileira=False)
    g = C_geo()
    a.add("poster", lambda t: dict(x=F[0], y=F[1], w=F[2], h=F[3], r=14), poster_label(FOCUS, F[2]), poster_style(FOCUS, True))
    add_hold(a, F)
    def px(t): return g["x"] + C_W * (1 - menu(t))
    vis = lambda t: 1 if menu(t) > 0.001 else 0
    a.add("escuro", lambda t: dict(x=px(t) - C_ESCURO, y=0, w=C_ESCURO, h=SH, o=0.9 * menu(t)),
          "", "background:linear-gradient(90deg,rgba(0,0,0,0) 0%,rgba(0,0,0,1) 100%)")
    a.add("sheet", lambda t: dict(x=px(t), y=0, w=C_W, h=SH, o=vis(t)), "",
          "background:rgba(14,14,16,.97);box-shadow:inset 1px 0 0 rgba(255,255,255,.08)")
    a.add("arte", lambda t: dict(x=px(t), y=0, w=C_W, h=C_ARTE_H, o=vis(t)),
          '<div style="position:absolute;inset:0;background:linear-gradient(0deg,rgba(14,14,16,1) 0%,rgba(14,14,16,0) 55%)"></div>',
          f"background:{FIGS[FOCUS]};overflow:hidden;background-size:cover")
    a.add("tit", lambda t: dict(x=px(t) + C_P, y=C_ARTE_H - 64, w=C_W - 2 * C_P, h=50, o=vis(t)),
          f'<div style="font-size:40px;font-weight:800;color:{BONE};white-space:nowrap;text-transform:uppercase">{NAMES[FOCUS]}</div>')
    a.add("meta", lambda t: dict(x=px(t) + C_P, y=C_ARTE_H + 8, w=C_W - 2 * C_P, h=30, o=vis(t)),
          f'<div style="display:flex;gap:18px;font-size:19px;font-weight:600;color:{MUT};white-space:nowrap"><span style="color:{BONE}">IMDb 8,7</span><span>Série · 2022</span><span>2 temporadas</span><span>16</span></div>')
    LX = C_P - 16       # a linha comeca 16 px antes do texto: o icone alinha com o titulo
    a.add("pill", lambda t: dict(x=px(t) + LX, y=C_Y0 + C_PASSO * fpos(t), w=C_W - 2 * LX, h=C_LH, r=10, o=vis(t), s=1 - 0.03 * press(t)),
          "", "background:rgba(244,242,238,.10);box-shadow:inset 0 0 0 1.5px rgba(244,242,238,.5)")
    a.add("pend", lambda t: dict(x=px(t) + C_P, y=C_Y0 + C_PASSO * fpos(t) + C_LH - 8, w=(C_W - 2 * C_P) * pend(t), h=3, r=2, o=1 if pend(t) > 0 else 0, bg=ACC))
    def linha(ic, lab):
        return (f'<div style="position:absolute;inset:0;display:flex;align-items:center;gap:14px;padding:0 16px;color:{BONE};'
                f'font-size:23px;font-weight:600;white-space:nowrap">{ico(ic, 26)}{lab}</div>')
    for i, (ic, lab) in enumerate(ACTIONS + [("users", "Recomendar a amigos")]):
        fn = (lambda i: lambda t: dict(x=px(t) + LX, y=C_Y0 + C_PASSO * i, w=C_W - 2 * LX, h=C_LH,
                                        o=vis(t) * ((1 - done(t)) if i == 1 else 1)))(i)
        a.add(f"r{i}", fn, linha(ic, lab))
    a.add("r1b", lambda t: dict(x=px(t) + LX, y=C_Y0 + C_PASSO, w=C_W - 2 * LX, h=C_LH, o=vis(t) * done(t)), linha("check", "Marcado como assistido"))
    a.add("sin", lambda t: dict(x=px(t) + C_P, y=C_Y0 + C_PASSO * 4 + 28, w=C_W - 2 * C_P, h=200, o=vis(t)),
          f'<div style="font-size:20px;line-height:1.45;color:{MUT}">Mark lidera uma equipe cujas memórias foram separadas cirurgicamente entre o trabalho e a vida pessoal. Faltam 38 min do T2 E4.</div>')
    return a

BUILDERS = {"A": build_A, "B": build_B, "C": build_C}
NAMES_ALT = {"A": "Desdobrar", "B": "Trilho", "C": "Folha"}
