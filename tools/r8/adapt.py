# Adaptacao das tres direcoes a cada origem: pôster, card largo, Palco, destaque (hero),
# Collection (pagina de estilo integrada) e item na borda direita.
from base import *
from menus import ACTIONS, A_PW, B_geo, C_geo, chip_html, row_label_html, poster_label

ACT_CONT = [("x", "Tirar de Continuar"), ("eye", "Marcar como assistido"), ("bookmark", "Salvar")]
ACT_HERO = [("bookmark", "Salvar"), ("eye", "Marcar como assistido"), ("users", "Recomendar")]
ESTILOS = ["Automático", "Pôster", "Paisagem", "Faixa com título", "Destaque 4:3", "Palco"]

# ---------------------------------------------------------------- cenas de origem
def sc_poster(focus, shift=None, dim=0.0):
    o = box(0, 0, SW, SH, "background:#08080A") + hero(0, h=600) + next_row() + topbar()
    o += poster_row(focus, shift=shift, dim_others=dim)
    return o, focus_rect(focus)

def sc_land(shift=None, dim=0.0, focus=1):
    o = box(0, 0, SW, SH, "background:#08080A") + hero(3, "FALLOUT", h=600) + topbar()
    y = 560
    o += txt(96, y - 46, "Continuar assistindo", 26, BONE, 700)
    R = None
    for i in range(5):
        x = 96 + i * 408; w, h = 384, 216
        dx = shift(i) if shift else 0
        if i == focus:
            s = 1.05; fw, fh = w * s, h * s; fx, fy = x + dx - (fw - w) / 2, y - (fh - h) / 2
            o += art(fx, fy, fw, fh, i + 3, 14, None, "box-shadow:0 0 0 3px #F4F2EE")
            o += box(fx + 16, fy + fh - 18, (fw - 32) * 0.62, 4, f"border-radius:2px;background:{BONE}")
            R = (fx, fy, fw, fh)
        else:
            o += art(x + dx, y, w, h, i + 3, 12, None, f"opacity:{1-dim:.2f}")
            o += box(x + dx + 16, y + h - 18, (w - 32) * (0.3 + 0.1 * i), 4, f"border-radius:2px;background:rgba(244,242,238,.7);opacity:{1-dim:.2f}")
    o += next_row(y + 216 + 120, "Populares")
    return o, R

def sc_palco():
    o = box(0, 0, SW, SH, f"background:radial-gradient(70% 60% at 75% 10%, rgba(150,70,170,.45), rgba(0,0,0,0) 60%),#08080A") + topbar()
    o += txt(150, 380, "Mais bem avaliados", 26, BONE, 700)
    R = (150, 420, 276, 419)
    o += art(*R, 7, 14, "SHŌGUN", "box-shadow:0 0 0 3px #F4F2EE")
    fx = 150 + 276 + 36
    o += txt(fx, 440, "PROGRAMA DE TV", 17, "#4fd1a5", 700, "letter-spacing:.08em")
    o += txt(fx, 470, "Shōgun", 44, BONE, 700)
    o += txt(fx, 530, "2024 · 1 temporada", 20, MUT, 500)
    o += txt(fx, 572, "Um navegador inglês naufraga no Japão feudal e é arrastado para uma disputa de poder entre senhores rivais.", 21, "#D8D5CF", 400, w=440)
    for i in range(4):
        o += art(1000 + i * 176, 520, 160, 240, i + 1, 10)
    return o, R, (fx, 430, 480, 419)

def sc_hero():
    o = box(0, 0, SW, SH, "background:#08080A") + hero(5, "O URSO", h=640) + topbar()
    o += box(90, 394, 188, 68, "border-radius:34px;box-shadow:0 0 0 3px #F4F2EE")
    o += next_row(ROW_Y + 200, "Populares")
    return o, (96, 400, 176, 56)

def sc_colecao(shift=None, dim=0.0, focus=1):
    o = box(0, 0, SW, SH, "background:#08080A") + hero(2, "STREAMING", h=600) + topbar()
    y = 600
    o += txt(96, y - 46, "Streaming Services", 26, BONE, 700)
    marcas = ["N", "prime video", "Disney+", "max", "Apple TV"]
    R = None
    for i in range(5):
        x = 96 + i * 344; w, h = 320, 180
        dx = shift(i) if shift else 0
        lab = f'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:34px;font-weight:800;color:#fff">{marcas[i]}</div>'
        if i == focus:
            s = 1.05; fw, fh = w * s, h * s; fx, fy = x + dx - (fw - w) / 2, y - (fh - h) / 2
            o += box(fx, fy, fw, fh, f"border-radius:14px;background:{FIGS[i+2]};box-shadow:0 0 0 3px #F4F2EE", lab)
            R = (fx, fy, fw, fh)
        else:
            o += box(x + dx, y, w, h, f"border-radius:12px;background:{FIGS[i+2]};opacity:{1-dim:.2f}", lab)
    o += next_row(y + 180 + 120, "Mais bem avaliados")
    return o, R

VEIL = lambda a: box(0, 0, SW, SH, f"background:rgba(0,0,0,{a})")

# ---------------------------------------------------------------- A · Desdobrar (estatico)
def A_panel(x, y, w, h, title, meta, actions, focus=0, rows_top=124):
    o = box(x, y, w, h, "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 70px rgba(0,0,0,.6)")
    return o

def A_contents(px, py, title, meta, actions, focus=0, w=A_PW, pitch=66):
    o = ""
    if title: o += txt(px + 30, py + 28, title, 30, BONE, 700)
    if meta: o += txt(px + 30, py + 70, meta, 19, MUT, 500)
    y0 = py + (124 if title else 20)
    o += box(px + 16, y0 + pitch * focus, w - 32, 56, "border-radius:16px;background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE")
    for i, (ic, lab) in enumerate(actions):
        o += box(px + 16, y0 + pitch * i, w - 32, 56, "", row_label_html(ic, lab))
    return o

def A_static(kind):
    if kind in ("poster", "borda"):
        f = 2 if kind == "poster" else 6
        R = focus_rect(f)
        side = 1 if R[0] + R[2] + A_PW <= SW - 48 else -1
        sh = (lambda i: 0 if i == f else (A_PW if i > f else -18)) if side > 0 else (lambda i: 0 if i == f else (-A_PW if i < f else 18))
        base, _ = sc_poster(f, sh, 0.55)
        cx = R[0] if side > 0 else R[0] - A_PW
        # a cena mudou: o cartaz em foco nao se move; reposiciona so as vizinhas (feito em sc_poster)
        o = base + VEIL(0.34)
        o += box(cx, R[1], R[2] + A_PW, R[3], "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 70px rgba(0,0,0,.6)")
        o += art(*R, f, 14, NAMES[f], "box-shadow:0 0 0 3px #F4F2EE")
        px = R[0] + R[2] if side > 0 else R[0] - A_PW
        o += A_contents(px, R[1], NAMES[f], "Série · 2022 · 2 temporadas", ACTIONS)
        return o, f"lado {'direito' if side > 0 else 'esquerdo'}: as vizinhas abrem espaço"
    if kind == "land":
        base, R = sc_land(lambda i: 0 if i == 1 else (A_PW if i > 1 else -18), 0.55)
        o = base + VEIL(0.34)
        o += box(R[0], R[1], R[2] + A_PW, R[3] + 40, "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 70px rgba(0,0,0,.6)")
        o += art(*R, 4, 14, None, "box-shadow:0 0 0 3px #F4F2EE")
        o += A_contents(R[0] + R[2], R[1] - 4, None, None, ACT_CONT, pitch=62)
        o += txt(R[0] + 16, R[1] + R[3] + 6, "T2 E4 · faltam 38 min", 18, MUT, 600)
        return o, "card largo: o painel cresce 40 px para baixo para caber 3 linhas"
    if kind == "palco":
        base, R, fic = sc_palco()
        o = base.replace("Um navegador inglês naufraga", "<span style='opacity:0'>Um navegador inglês naufraga")  # sinopse cede
        o = base + VEIL(0.30)
        o += art(*R, 7, 14, "SHŌGUN", "box-shadow:0 0 0 3px #F4F2EE")
        x, y = fic[0] - 16, 540
        o += box(x, y, 500, 230, "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10)")
        o += txt(fic[0], 440, "PROGRAMA DE TV", 17, "#4fd1a5", 700, "letter-spacing:.08em") + txt(fic[0], 470, "Shōgun", 44, BONE, 700)
        o += A_contents(x - 14, y - 4, None, None, ACTIONS, w=528, pitch=70)
        return o, "Palco: a sinopse da ficha se dobra e vira o painel; o nome fica"
    if kind == "hero":
        base, R = sc_hero()
        o = base + VEIL(0.30)
        o += box(96, 400, 520, 250, "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10)")
        o += A_contents(96, 384, None, None, ACT_HERO, w=520, pitch=66)
        o += txt(96, 176, "O URSO", 64, BONE, 800)
        return o, "destaque: o botão “Ver título” se desdobra no painel"
    if kind == "colecao":
        base, R = sc_colecao(lambda i: 0 if i == 1 else (470 if i > 1 else -18), 0.55)
        o = base + VEIL(0.34)
        o += box(R[0], R[1] - 130, R[2] + 470, R[3] + 260, "border-radius:16px;background:rgba(16,16,18,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10)")
        o += box(R[0], R[1], R[2], R[3], f"border-radius:14px;background:{FIGS[3]};box-shadow:0 0 0 3px #F4F2EE",
                 '<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:34px;font-weight:800;color:#fff">prime video</div>')
        px, py = R[0] + R[2] + 16, R[1] - 120
        o += txt(px + 14, py, "Estilo da fileira", 24, BONE, 700)
        for i, e in enumerate(ESTILOS):
            yy = py + 44 + i * 52
            if i == 2: o += box(px, yy, 430, 46, "border-radius:14px;background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE")
            o += box(px + 16, yy + 13, 28, 20, f"border-radius:4px;box-shadow:inset 0 0 0 2px {MUT}")
            o += txt(px + 60, yy + 10, e, 21, BONE if i == 2 else MUT, 600)
        return o, "Collection: a página de estilo é o próprio painel (prévia só ao confirmar)"

# ---------------------------------------------------------------- B · Trilho (estatico)
def B_draw(g, actions, focus=0, ghost_label=None):
    o = box(g["x"], g["y"], g["w"], g["h"], "border-radius:34px;background:rgba(14,14,16,.96);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 24px 60px rgba(0,0,0,.6)")
    cx, cw = g["chips"][focus]
    o += box(cx, g["y"] + 8, cw, g["h"] - 16, "border-radius:26px;background:rgba(244,242,238,.14);box-shadow:inset 0 0 0 2px #F4F2EE")
    for (ic, lab), (x, w) in zip(actions, g["chips"]):
        o += box(x, g["y"] + 8, w, g["h"] - 16, "", chip_html(ic, lab))
    return o

def B_static(kind):
    if kind in ("poster", "borda"):
        f = 2 if kind == "poster" else 6
        base, R = sc_poster(f, None, 0.0)
        g = B_geo(R)
        o = base + VEIL(0.34) + art(*R, f, 14, NAMES[f], "box-shadow:0 0 0 3px #F4F2EE") + B_draw(g, ACTIONS)
        return o, "centrado no cartaz; na borda, preso à margem segura"
    if kind == "land":
        base, R = sc_land()
        g = B_geo(R, ACT_CONT)
        o = base + VEIL(0.34) + art(*R, 4, 14, None, "box-shadow:0 0 0 3px #F4F2EE") + B_draw(g, ACT_CONT)
        return o, "card largo: mesma regra, o trilho cabe sob o card"
    if kind == "palco":
        base, R, fic = sc_palco()
        g = B_geo((fic[0], 430, 10, 280), ACTIONS)
        g = B_geo((fic[0] + g["w"] / 2 - 5, 430, 10, 300), ACTIONS)   # alinha a esquerda com a ficha
        o = base + VEIL(0.30) + art(*R, 7, 14, "SHŌGUN", "box-shadow:0 0 0 3px #F4F2EE") + B_draw(g, ACTIONS)
        o += txt(fic[0], 440, "PROGRAMA DE TV", 17, "#4fd1a5", 700, "letter-spacing:.08em") + txt(fic[0], 470, "Shōgun", 44, BONE, 700)
        return o, "Palco: o trilho entra sob a sinopse, alinhado à ficha"
    if kind == "hero":
        base, R = sc_hero()
        g = B_geo((96, 400, 176, 56), ACT_HERO)
        g = dict(g, x=290, y=394); xs, acc_ = [], 290 + 8
        for (_, l) in ACT_HERO:
            w = 18 + 26 + 12 + tw(l, 22, .55) + 22; xs.append((acc_, w)); acc_ += w + 6
        g["chips"] = xs
        o = base + VEIL(0.28) + txt(96, 176, "O URSO", 64, BONE, 800) + B_draw(g, ACT_HERO)
        o += box(96, 400, 176, 56, f"border-radius:28px;background:{BONE};display:flex;align-items:center;justify-content:center;gap:10px;color:{INK};font-weight:700;font-size:20px", ico("play", 22, INK) + "Ver título")
        return o, "destaque: o trilho sai do botão “Ver título”, na mesma linha"
    if kind == "colecao":
        base, R = sc_colecao()
        acts = [("rows", e) for e in ESTILOS[:5]]
        g = B_geo(R, acts)
        o = base + VEIL(0.34) + box(R[0], R[1], R[2], R[3], f"border-radius:14px;background:{FIGS[3]};box-shadow:0 0 0 3px #F4F2EE",
              '<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:34px;font-weight:800;color:#fff">prime video</div>')
        o += B_draw(g, acts, 2)
        return o, "Collection: o trilho vira o seletor de estilo (rola na horizontal)"

# ---------------------------------------------------------------- C · Folha (estatico)
def C_draw(g, title, actions, art_fig, focus=0, info=True, styles=False):
    sx, sy = g["x"], g["y"]
    o = box(sx, sy, g["w"], g["h"], "border-radius:22px;background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),-30px 0 80px rgba(0,0,0,.55)")
    o += box(sx, sy, g["w"], 338, f"border-radius:22px 22px 0 0;background:{FIGS[art_fig]}",
             '<div style="position:absolute;inset:0;background:linear-gradient(0deg,rgba(14,14,16,1) 0%,rgba(14,14,16,0) 55%)"></div>')
    o += txt(sx + 36, sy + 264, title, 40, BONE, 800, "text-transform:uppercase")
    o += txt(sx + 36, sy + 358, f'<span style="color:{BONE}">IMDb 8,7</span>&nbsp;&nbsp;·&nbsp;&nbsp;Série · 2022&nbsp;&nbsp;·&nbsp;&nbsp;16', 19, MUT, 600)
    y0 = sy + 414
    o += box(sx + 20, y0 + 68 * focus, g["w"] - 40, 58, "border-radius:16px;background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE")
    for i, (ic, lab) in enumerate(actions):
        o += box(sx + 20, y0 + 68 * i, g["w"] - 40, 58, "", row_label_html(ic, lab))
    if info:
        o += txt(sx + 36, y0 + 68 * len(actions) + 30, "Mark lidera uma equipe cujas memórias foram separadas entre o trabalho e a vida pessoal.", 20, MUT, 400, "line-height:1.45", w=g["w"] - 72)
    return o

def C_static(kind):
    if kind in ("poster", "borda"):
        f = 2 if kind == "poster" else 6
        base, R = sc_poster(f, None, 0.0)
        g = C_geo(R)
        o = base + VEIL(0.5) + art(*R, f, 14, NAMES[f], "box-shadow:0 0 0 3px #F4F2EE") + C_draw(g, NAMES[f], ACTIONS, f)
        return o, f"folha à {'direita' if g['side'] > 0 else 'esquerda'}: sempre do lado oposto ao item"
    if kind == "land":
        base, R = sc_land()
        g = C_geo(R)
        o = base + VEIL(0.5) + art(*R, 4, 14, None, "box-shadow:0 0 0 3px #F4F2EE") + C_draw(g, "FALLOUT", ACT_CONT, 4)
        return o, "card largo: a imagem larga é a do próprio card"
    if kind == "palco":
        base, R, fic = sc_palco()
        g = C_geo(R)
        o = base + VEIL(0.45) + art(*R, 7, 14, "SHŌGUN", "box-shadow:0 0 0 3px #F4F2EE") + C_draw(g, "Shōgun", ACTIONS, 7, info=False)
        return o, "Palco: a ficha continua legível ao lado do cartaz"
    if kind == "hero":
        base, R = sc_hero()
        g = C_geo((96, 400, 176, 56))
        o = base + VEIL(0.45) + C_draw(g, "O Urso", ACT_HERO, 5)
        return o, "destaque: mesma folha; a arte do topo é a do destaque"
    if kind == "colecao":
        base, R = sc_colecao()
        g = C_geo(R)
        acts = [("rows", e) for e in ESTILOS]
        o = base + VEIL(0.5) + C_draw(g, "prime video", acts, 3, focus=2, info=False)
        return o, "Collection: a folha mostra os estilos (lista completa)"

KINDS = [("poster", "Pôster vertical"), ("land", "Card largo (Continuar)"), ("palco", "Palco"),
         ("hero", "Destaque (hero)"), ("colecao", "Collection"), ("borda", "Item na borda direita")]
STATIC = {"A": A_static, "B": B_static, "C": C_static}
