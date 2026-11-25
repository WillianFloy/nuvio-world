# Gera as pranchas da secao 8 (rodada 24/11, noite), as mesmas aprovadas pelo dono.
# Uso: python3 tools/r8/pranchas.py /tmp/r8-pranchas   (o canvas do projeto nao e tocado)
import json, os
from base import *
import menus, adapt, panels

import sys
OUT = sys.argv[1] if len(sys.argv) > 1 else "/tmp/r8-pranchas"
os.makedirs(OUT, exist_ok=True)
BOARDS = []

def write(name, title, body, styles="", h=SH):
    open(f"{OUT}/{name}.dc.html", "w").write(page(title, body, styles, h=h))
    BOARDS.append((name, title, h))

def bullets(x, y, items, w=520, size=19, gap=14):
    o, yy = "", y
    for it in items:
        lines = max(1, int(tw(it, size, 0.52) / w) + 1)
        o += box(x, yy + size * 0.55, 6, 6, f"border-radius:3px;background:{BONE}")
        o += txt(x + 20, yy, it, size, "#D8D5CF", 400, "line-height:1.4", w=w - 20)
        yy += lines * size * 1.4 + gap
    return o, yy

# ================================================================ 0 · diagnostico
IMG = {"v46": "/_blob/7aa0e9043e521e04ae4ab67e5d1c4c13", "v130": "/_blob/3bf630724c770eeedcb85e0b08cd95aa", "v162": "/_blob/db89dda8966b3842e0b642b1e6b2166b",
       "v370": "/_blob/c79b438aeff5208daed18e7153b4d920", "v446": "/_blob/c32b3911de505ca7664b364b23f044e2", "v475": "/_blob/62c440c8bf762576cc0b4e8d25541d8f",
       "v512": "/_blob/75f2b05a2190330b1523a9174d36768f"}
def diag():
    cells = [("v46", "0:04 · Destaque", "Segurar OK no destaque ainda abre o menu ANTIGO (ilha + cartão de informações). O mesmo gesto tem dois desenhos."),
             ("v130", "0:13 · Pôster", "As fichas caem em cima do pôster vizinho (o vão da fileira é ~24 px). O véu de 22 % quase não se vê."),
             ("v162", "0:16 · Abrindo", "A caixa do rótulo cresce antes do texto (retângulo preto vazio) e o texto aparece cortado com reticências."),
             ("v370", "0:37 · Collection", "Página de estilo antiga, translúcida, sobre os cards; a fileira muda de estilo ao vivo atrás dela (saltos)."),
             ("v446", "0:44 · Ranking", "Fichas sobre o número “2” e o pôster vizinho; o rótulo aberto cobre a arte."),
             ("v475", "0:47 · Entrando no Palco", "Quadro quase preto; o cartaz do Palco aparece como uma faixa com “Mais bem avaliados” dentro. Causa não confirmada."),
             ("v512", "0:51 · Palco", "Fichas dentro do cartaz cobrindo a logo; legenda escrita sobre a arte, perto do nome do título.")]
    b = box(0, 0, SW, SH, "background:#0A0A0B") + head("8.0 · Diagnóstico · vídeo VID_20261124_182912 (TCL)",
        "Quadros do vídeo com o que aparece em cada um. Comprovado = visto no vídeo e explicado pelo código; o resto está marcado como hipótese no documento.")
    for i, (k, lab, cap) in enumerate(cells):
        x = 96 + (i % 4) * 444; y = 236 + (i // 4) * 410
        b += txt(x, y - 34, lab, 19, MUT, 700, "letter-spacing:.04em")
        b += f'<img src="{IMG[k]}" alt="{lab}" style="position:absolute;left:{x}px;top:{y}px;width:420px;height:236px;object-fit:cover;border-radius:8px;box-shadow:0 0 0 1.5px rgba(255,255,255,.12)">'
        b += txt(x, y + 248, cap, 17, "#D8D5CF", 400, "line-height:1.4", w=420)
    x, y = 96 + 3 * 444, 236 + 410
    b += box(x, y, 420, 236, "border-radius:8px;background:#141416;box-shadow:0 0 0 1.5px rgba(255,255,255,.12)")
    b += txt(x + 24, y + 22, "Log da TCL (18:30)", 19, BONE, 700)
    rows = [("Home parada", "38–40 fps"), ("Menu aberto", "32 fps, 100 % atrasado"), ("Véu de tela cheia", "+1,0 tela de pintura"), ("Salvos/Busca parados", "60 fps")]
    for j, (a_, v) in enumerate(rows):
        b += txt(x + 24, y + 62 + j * 40, f'{a_}: <span style="color:{BONE};font-weight:600">{v}</span>', 17, MUT, 500)
    b += txt(x, y + 248, "A 30 fps uma transição de 300 ms tem 9 quadros: por isso a animação parece “seca”.", 17, "#D8D5CF", 400, "line-height:1.4", w=420)
    write("R8-0-diagnostico", "8.0 · Diagnóstico do vídeo", b)

# ================================================================ 1 · fundacao
def fundacao():
    b = box(0, 0, SW, 1120, "background:#0A0A0B") + head("8.1 · Fundação · uma linguagem para o app inteiro",
        "O que todas as telas compartilham: superfície, tipografia, foco, ícones e raios. Cada tela tem a sua composição; a identidade vem destes elementos repetidos.")
    # cores
    b += txt(96, 220, "SUPERFÍCIES E TEXTO", 16, DIM, 700, "letter-spacing:.12em")
    sw = [("#0B0B0C", "Tinta", "superfícies (97 %)"), ("#151517", "Grafite 1", "fundo de linha"), ("#1E1E21", "Grafite 2", "campo"),
          ("#2A2A2E", "Grafite 3", "separador"), ("#F4F2EE", "Osso", "texto e foco"), ("#B4B1AB", "Cinza", "apoio"), (ACC, "Acento", "só progresso e “ao vivo”")]
    for i, (c, n, d) in enumerate(sw):
        x = 96 + i * 150
        b += box(x, 256, 132, 92, f"border-radius:14px;background:{c};box-shadow:0 0 0 1.5px rgba(255,255,255,.12)")
        b += txt(x, 358, n, 18, BONE, 700) + txt(x, 382, d, 15, MUT, 500, w=140)
    # tipografia
    b += txt(1200, 220, "TIPOGRAFIA · MONTSERRAT (A FONTE DA TV)", 16, DIM, 700, "letter-spacing:.12em")
    ty = [("Título 44 / 800", 44, 800), ("Seção 26 / 700", 26, 700), ("Ação 22 / 600", 22, 600), ("Apoio 19 / 500", 19, 500)]
    yy = 252
    for s, sz, wg in ty:
        b += txt(1200, yy, s, sz, BONE if sz > 20 else MUT, wg); yy += sz * 1.3 + 12
    b += txt(1200, yy + 4, "LEGENDA 15 / 700 ESPAÇADA", 15, DIM, 700, "letter-spacing:.12em")
    # material
    b += txt(96, 450, "MATERIAL", 16, DIM, 700, "letter-spacing:.12em")
    b += box(96, 486, 520, 300, f"background:{FIGS[2]};border-radius:14px")
    b += box(136, 526, 440, 220, "border-radius:22px;background:rgba(14,14,16,.97);box-shadow:0 0 0 1.5px rgba(255,255,255,.10),0 30px 80px rgba(0,0,0,.6)")
    b += txt(168, 556, "Tinta 97 %", 22, BONE, 700) + txt(168, 590, "fio 1,5 px branco 10 %\nsombra longa, sem vidro".replace("\n", "<br>"), 18, MUT, 500)
    b += txt(96, 800, "Sem vidro, sem gradiente decorativo. A profundidade vem da sombra e do fundo que recua.", 17, MUT, 400, w=520)
    # foco
    b += txt(700, 450, "FOCO · UMA REGRA SÓ", 16, DIM, 700, "letter-spacing:.12em")
    b += box(700, 486, 440, 60, "border-radius:16px;background:rgba(244,242,238,.13);box-shadow:inset 0 0 0 2px #F4F2EE", adapt.row_label_html("eye", "Ação em foco"))
    b += box(700, 556, 440, 60, "", adapt.row_label_html("bookmark", "Ação sem foco", 23, MUT))
    b += box(700, 640, 300, 60, "border-radius:30px;background:rgba(14,14,16,.96);box-shadow:0 0 0 1.5px rgba(255,255,255,.10)")
    b += box(708, 648, 150, 44, "border-radius:22px;background:rgba(244,242,238,.14);box-shadow:inset 0 0 0 2px #F4F2EE", menus.chip_html("check", "Chip").replace("font-size:22px", "font-size:19px"))
    b += art(1030, 620, 110, 165, 1, 14, None, "box-shadow:0 0 0 3px #F4F2EE")
    b += txt(700, 800, "Ação: véu osso 13 % + contorno 2 px. Conteúdo: contorno 3 px + escala 1,06. Pressionado: 0,96. O acento nunca é o foco (funciona com qualquer cor escolhida).", 17, MUT, 400, w=440)
    # icones e raios
    b += txt(1200, 486, "ÍCONES · TRAÇO 2 PX, PONTAS REDONDAS", 16, DIM, 700, "letter-spacing:.12em")
    for i, n in enumerate(["bookmark", "eye", "rows", "users", "search", "mic", "film", "wave", "moon", "badge"]):
        b += box(1200 + (i % 5) * 76, 520 + (i // 5) * 76, 60, 60, f"border-radius:14px;background:#151517;display:flex;align-items:center;justify-content:center;color:{BONE}", ico(n, 28))
    b += txt(1200, 686, "RAIOS", 16, DIM, 700, "letter-spacing:.12em")
    for i, (r, lab) in enumerate([(34, "trilho"), (22, "folha"), (16, "painel/linha"), (12, "cartaz")]):
        b += box(1200 + i * 150, 720, 120, 70, f"border-radius:{r}px;background:#1E1E21;box-shadow:0 0 0 1.5px rgba(255,255,255,.12)")
        b += txt(1200 + i * 150, 798, f"{r} · {lab}", 16, MUT, 600)
    # um gesto, um componente
    b += txt(96, 900, "UM GESTO, UM COMPONENTE", 16, DIM, 700, "letter-spacing:.12em")
    rows = [("Segurar OK (qualquer item)", "Trilho ou Folha (8.4–8.9)"), ("CH+ toque", "Salvos · Estante (8.11)"), ("CH+ segurado", "Central · Faixa (8.13)"),
            ("CH−", "Busca · campo no topo (8.15)"), ("Relógio, ilhas", "pílula Tinta (já existe)"), ("Ajustes, listas", "linhas com o mesmo foco")]
    for i, (g, c) in enumerate(rows):
        x = 96 + (i % 3) * 580; y = 940 + (i // 3) * 70
        b += box(x, y, 556, 58, "border-radius:14px;background:#141416")
        b += txt(x + 20, y + 17, g, 19, BONE, 700) + txt(x + 300, y + 18, c, 18, MUT, 500, w=250)
    write("R8-1-fundacao", "8.1 · Fundação visual", b, h=1120)

# ================================================================ 2 · sistema de movimento
def curve_svg(x, y, p, label, sub):
    s = 200
    x1, y1, x2, y2 = p
    path = f"M0 {s} C {x1*s:.0f} {s - y1*s:.0f} {x2*s:.0f} {s - y2*s:.0f} {s} 0"
    o = box(x, y, s + 40, s + 40, "border-radius:14px;background:#141416")
    o += (f'<svg style="position:absolute;left:{x+20}px;top:{y+20}px" width="{s}" height="{s}" viewBox="0 0 {s} {s}" aria-hidden="true">'
          f'<path d="M0 {s} L{s} 0" stroke="rgba(255,255,255,.12)" stroke-width="2" fill="none"/>'
          f'<path d="{path}" stroke="#F4F2EE" stroke-width="4" fill="none" stroke-linecap="round"/></svg>')
    o += txt(x, y + s + 56, label, 20, BONE, 700) + txt(x, y + s + 86, sub, 16, MUT, 500, w=s + 40)
    return o

def movimento():
    b = box(0, 0, SW, SH, "background:#0A0A0B") + head("8.2 · Sistema de movimento · mesma coreografia em todo lugar",
        "Toda camada que abre nasce do que a chamou e volta para lá pelo mesmo caminho. Quatro curvas e cinco camadas cobrem o menu, os painéis do CH e as ilhas.")
    b += curve_svg(96, 220, (.2, .8, .2, 1), "Entrada · 380–480 ms", "desacelera: rápido no início, pousa devagar")
    b += curve_svg(376, 220, (.4, 0, 1, 1), "Saída · 200–260 ms", "acelera: o mesmo caminho, mais curto")
    b += curve_svg(656, 220, (.3, 1.3, .5, 1), "Foco · 220 ms", "mola com ~4 % de repique, nunca salta")
    b += curve_svg(936, 220, (.4, 0, .2, 1), "Troca · 160 ms", "cruzada com 12 px de deslocamento")
    # camadas
    X, W = 96, 1100
    b += txt(X, 600, "COREOGRAFIA DE ABERTURA (0–500 MS)", 16, DIM, 700, "letter-spacing:.12em")
    lay = [("Fundo", 0, 480, "congela no 1º quadro; recua 1,5–3 % e escurece 34–55 %"),
           ("Origem", 0, 500, "o item/pílula que abriu fica na tela, por cima do véu"),
           ("Superfície", 20, 420, "cresce da origem: primeiro a largura, depois a altura"),
           ("Conteúdo", 140, 480, "cascata de 35–50 ms, da origem para fora; só entra com a superfície ≥ 85 %"),
           ("Foco", 200, 420, "o realce aparece no item inicial e depois só desliza")]
    for i, (n, t0, t1, d) in enumerate(lay):
        y = 640 + i * 62
        b += txt(X, y + 10, n, 19, BONE, 700)
        b += box(X + 150, y + 14, W - 150, 4, "border-radius:2px;background:rgba(255,255,255,.08)")
        b += box(X + 150 + (W - 150) * t0 / 500, y + 6, (W - 150) * (t1 - t0) / 500, 20, "border-radius:10px;background:rgba(244,242,238,.35)")
        b += txt(X + 150, y + 32, d, 15, MUT, 500, w=W - 150)
    for s in range(0, 501, 100):
        b += txt(X + 150 + (W - 150) * s / 500 - 12, 950, f"{s} ms", 14, DIM, 500)
    rules = ["Nada aparece sem vir de algum lugar: o item, a pílula do relógio, a barra do topo.",
             "Fechar é o mesmo caminho ao contrário, ~40 % mais curto; o conteúdo sai antes da superfície.",
             "O conteúdo nunca aparece cortado: rótulos medidos no tamanho final, entram depois da forma.",
             "Quem abriu continua vivo; o resto da tela vira uma cópia parada (FBO) que recua.",
             "60 fps é pré-requisito: hoje o menu roda a 32 fps porque a Home inteira é redesenhada atrás.",
             "Reduzir animações: só opacidade, 120 ms (regra que já existe)."]
    o, _ = bullets(1290, 224, rules, 534, 19, 16)
    b += txt(1290, 186, "REGRAS", 16, DIM, 700, "letter-spacing:.12em") + o
    write("R8-2-movimento", "8.2 · Sistema de movimento", b)

# ================================================================ quadro de movimento (generico)
def live_board(name, title, sub, anim, marks, notes, strip, cycle=CYCLE, crop_r=None):
    inner, css = anim.live()
    n = len(strip); fw = (1728 - (n - 1) * 18) / n
    fh = fw * (crop_r[3] / crop_r[2] if crop_r else SH / SW)
    H = int(236 + 1110 * SH / SW + 26 + 110 + fh + 60)
    b = box(0, 0, SW, H, "background:#0A0A0B") + head(title, sub)
    b += live_frame(inner, 96, 236, 1110, "PRÉVIA AO VIVO · LOOP DE 7 S")
    yb = 236 + 1110 * SH / SW + 26
    b += timeline_bar(96, yb, 1110, marks, cycle)
    o, _ = bullets(1260, 236, notes, 564, 19, 14)
    b += txt(1260, 198, "COMO SE MOVE", 16, DIM, 700, "letter-spacing:.12em") + o
    y = yb + 110
    for i, (t, lab) in enumerate(strip):
        x = 96 + i * (fw + 18)
        if crop_r:
            b += crop(anim.static(t), crop_r[0], crop_r[1], crop_r[2], crop_r[3], fw / crop_r[2], x, y, lab)
        else:
            b += frame(anim.static(t), x, y, fw, lab)
    write(name, title, b, css, h=H)

def adapt_board(name, title, sub, k):
    b = box(0, 0, SW, SH, "background:#0A0A0B") + head(title, sub)
    for i, (kind, lab) in enumerate(adapt.KINDS):
        scene, note = adapt.STATIC[k](kind)
        x = 96 + (i % 3) * 584; y = 236 + (i // 3) * 412
        b += frame(scene, x, y, 560, lab.upper(), note)
    write(name, title, b)

MENU_TXT = {
 "A": ("Desdobrar · o cartaz vira o painel",
       "O próprio cartaz se desdobra para o lado e as vizinhas abrem espaço; nada fica por cima de outro título.",
       ["Segurar: a barra de 4 px corre dentro da base do cartaz (já existe).",
        "Abrir: a superfície nasce atrás do cartaz e se estende 470 px; as vizinhas da direita deslizam junto, as da esquerda recuam 18 px e escurecem.",
        "O cartaz não muda: é o item real. Ao fechar, é ele que fica (nunca outra arte).",
        "Conteúdo: título, depois meta, depois as ações em cascata de 50 ms.",
        "Foco: o realce desliza com mola; pressionar encolhe 4 %; pendente = linha no acento; feito = ícone de confirmado e rótulo novo.",
        "Fechar: o conteúdo sai em 80 ms, a superfície se recolhe em 260 ms e as vizinhas voltam."]),
 "B": ("Trilho · uma pílula de ações sob o item",
       "Uma pílula escura nasce da base do item e se abre na horizontal. Mesma forma da barra do topo, do relógio e da Busca.",
       ["Segurar: a barra de 4 px na base do cartaz enche; ao completar, ela vira o trilho.",
        "Abrir: a pílula nasce estreita no centro da base (altura primeiro), depois se abre para os lados até o tamanho das ações.",
        "O cartaz sobe 3 % por cima do véu de 34 %; a cópia parada da Home recua 1 %.",
        "Chips entram em cascata de 40 ms, do centro para fora.",
        "Foco elástico: a borda da frente vai primeiro e a de trás alcança 70 ms depois.",
        "OK: o chip afunda 4 %; pendente = linha no acento; feito = “Assistido” com ícone de confirmado.",
        "Fechar: os chips somem, a pílula volta para a base do cartaz e encolhe."]),
 "C": ("Folha · um painel de título ao lado",
       "Uma folha no lado oposto ao item. A arte do próprio título voa do cartaz para o topo da folha. Volta o espaço para informações.",
       ["Segurar: igual às outras (barra na base do cartaz).",
        "Abrir: uma cópia da arte sai do cartaz e voa até o topo da folha, trocando de pôster para imagem larga no caminho.",
        "A folha entra 70 px de lado com opacidade; a Home recua 2,5 % e escurece 50 %.",
        "Conteúdo: título sobre a arte, notas, ações, sinopse, em cascata.",
        "Foco e estados iguais aos do A; a folha cabe 4 ações e uma linha de informações.",
        "Fechar: a arte volta voando para o cartaz de origem, a folha sai pelo mesmo lado."]),
}
def menus_boards():
    strip = [(1.0, "segurando"), (1.42, "abrindo 30 %"), (1.6, "abrindo 70 %"), (2.6, "foco mudando"), (4.4, "OK → pendente"), (5.4, "fechando")]
    for k in "ABC":
        t, s, notes = MENU_TXT[k]
        live_board(f"R8-M{k}-1-movimento", f"8.{ {'A':4,'B':6,'C':8}[k] } · Menu {k} · {t}", s, menus.BUILDERS[k](), menus.TL_MARKS, notes, strip, crop_r=(300, 380, 1100, 700))
        adapt_board(f"R8-M{k}-2-adaptacao", f"8.{ {'A':5,'B':7,'C':9}[k] } · Menu {k} · em cada origem",
                    "Mesma regra no pôster, card largo, Palco, destaque, Collection e item na borda. A geometria sai do retângulo real do item.", k)

def comparativo():
    b = box(0, 0, SW, SH, "background:#0A0A0B") + head("8.10 · Menu: as três direções lado a lado",
        "Nenhuma usa formas cortadas. As três usam a mesma superfície, o mesmo foco e o mesmo relógio de animação.")
    cols = [("A · Desdobrar", "R8-MA-1-movimento"), ("B · Trilho", "R8-MB-1-movimento"), ("C · Folha", "R8-MC-1-movimento")]
    for i, k in enumerate("ABC"):
        b += frame(menus.BUILDERS[k]().static(2.2), 96 + i * 584, 230, 560, cols[i][0].upper())
    rows = [("Continuidade", "máxima: o cartaz vira o painel", "alta: sai da base do item", "média-alta: a arte voa até a folha"),
            ("O que cobre", "nada: as vizinhas abrem espaço", "parte da fileira de baixo (escurecida)", "um terço da tela, no lado oposto"),
            ("Rótulos longos (29 idiomas)", "bom (470 px)", "limitado: trilho largo com 4+ ações", "ótimo (600 px)"),
            ("Informações do título", "nome e meta", "não", "sim: notas, sinopse, onde parou"),
            ("Destaque, Palco, Collection", "sim, com variação por origem", "sim", "sim, idêntico"),
            ("Custo na Mali-G52*", "médio: a fileira continua viva", "baixo", "médio: folha + arte"),
            ("Mudança no código", "alta: deslocar vizinhas na Home", "média", "média"),
            ("Risco principal", "conflito com foco fixo/rolagem", "legibilidade sobre a fileira de baixo", "parecer “outra tela”")]
    y = 580
    for j, r in enumerate(rows):
        yy = y + j * 50
        if j % 2 == 0: b += box(96, yy - 8, 1728, 46, "border-radius:10px;background:#121214")
        b += txt(112, yy + 4, r[0], 18, BONE, 700)
        for i in range(3):
            b += txt(430 + i * 465, yy + 4, r[i + 1], 18, "#D8D5CF", 500, w=450)
    b += txt(96, y + 8 * 50 + 14, "* com a Home congelada atrás (pré-requisito das três). Recomendação: B como menu padrão; C se as informações do título forem essenciais. A é a mais cinematográfica, mas mexe na rolagem da fileira.", 18, MUT, 500, w=1728)
    write("R8-M-comparativo", "8.10 · Menu: comparativo", b)

# ================================================================ painéis
def paineis():
    strip = [(0.95, "abrindo"), (1.4, "aberto"), (2.33, "foco mudando"), (5.5, "fechando")]
    live_board("R8-S1-estante", "8.11 · CH+ (Salvos) · S1 Estante",
               "A Home sobe e escurece; o terço de cima vira a arte do salvo em foco e a estante sobe da base da tela. A barra do topo fica, com Salvos aceso.",
               panels.build_S1(), panels.P_MARKS,
               ["Abrir: a cópia parada da Home sobe 60 px, encolhe 4 % e apaga 65 %; a estante sobe da base com borda em degradê (sem linha dura).",
                "O topo é a arte do item em foco, com nome, onde parou e progresso. Troca cruzada a cada foco (160 ms).",
                "Abas em texto (Continuar 6 · Minha lista 12 · Assistidos · Categorias) com o mesmo foco das ações.",
                "Cards largos com progresso integrado; o anel de foco desliza entre eles.",
                "Barra lateral (configuração da TCL): o ícone Salvos acende igual; a estante começa à direita dela.",
                "Fechar: a estante desce, a arte apaga e a Home volta para o lugar."],
               strip)
    live_board("R8-S2-coluna", "8.12 · CH+ (Salvos) · S2 Coluna editorial",
               "Folha à direita, abaixo da barra do topo. A linha em foco se abre num cartão grande com as ações; as outras descem.",
               panels.build_S2(), panels.P_MARKS,
               ["Abrir: a Home é empurrada 30 px para a esquerda e recua; a folha entra 60 px da direita.",
                "Cabeçalho: “Salvos”, a contagem e as abas.",
                "Linha em foco: a miniatura cresce para 568×320 e o nome desce para baixo dela; Retomar, Detalhes, Remover aparecem por último.",
                "Mudar de foco: a linha anterior fecha enquanto a próxima abre (acordeão na mola).",
                "Mais perto do Salvos de hoje (lista), mas com outra composição; risco de parecer “o mesmo painel”."],
               strip)
    live_board("R8-C1-faixa", "8.13 · CH+ segurado (Central) · C1 Faixa",
               "A pílula do relógio se estica para a esquerda e vira uma faixa no topo. A barra do topo sobe e some; a Home desce um pouco e escurece.",
               panels.build_C1(), panels.P_MARKS,
               ["Abrir: a largura vem primeiro (da pílula para a esquerda), depois a altura.",
                "A hora sai da pílula e vira o cabeçalho da faixa (mesmo texto, sem trocar).",
                "Ladrilhos agrupados (Imagem · Som · App), valor sempre visível; entram da direita para a esquerda, a partir do relógio.",
                "Foco: anel com véu e o ladrilho sobe 6 px; mola entre ladrilhos, inclusive entre grupos.",
                "A Home desce 24 px “empurrada” pela faixa.",
                "Fechar: os ladrilhos somem, a faixa encolhe de volta para a pílula."],
               strip)
    live_board("R8-C2-coluna", "8.14 · CH+ segurado (Central) · C2 Coluna",
               "A pílula do relógio desce e vira uma coluna à direita, com linhas agrupadas e valores à direita.",
               panels.build_C2(), panels.P_MARKS,
               ["Abrir: a pílula se alarga para a esquerda e depois desce até 900 px.",
                "Linhas de 60 px com ícone, nome e valor; grupos com legenda.",
                "Mesma linguagem da Folha (C) do menu e do S2.",
                "Mais simples de implementar, mais parecida com o layout atual."],
               strip)
    live_board("R8-B-busca", "8.15 · CH− (Busca) · a barra vira o campo",
               "A lupa da barra do topo se estica e vira o campo colado no topo; o resto da barra sai para os lados. Teclado e resultados chegam em cascata.",
               panels.build_busca(), [(0.8, 1.3, "abrir"), (2.2, 2.9, "digitando: resultados chegam"), (5.4, 5.68, "fechar")],
               ["Abrir: os ícones da barra deslizam 40 px e apagam; a pílula da barra cresce até 1100 px e sobe até colar no topo.",
                "A lupa não some: viaja até a ponta esquerda do campo.",
                "Avatar e relógio saem para fora da tela pelos lados.",
                "O corpo desce do campo depois que ele assenta; o teclado sobe dentro dele, linha a linha (50 ms).",
                "Resultados chegam em cascata (60 ms) quando há texto; trocar o texto troca os resultados com 12 px.",
                "Fechar: tudo volta para a barra pelo mesmo caminho."],
               [(0.95, "abrindo"), (1.4, "aberto"), (3.1, "digitando"), (5.5, "fechando")])
    hs = [(0.92, "toque curto"), (1.12, "soltou: recolhe"), (2.6, "segurando"), (3.05, "Central abrindo")]
    live_board("R8-H-segurar-sem-relogio", "8.16 · Segurar CH+ com o relógio desligado",
               "Sem relógio, nada de hora: só um fio fino no canto onde o relógio ficaria, com “Central”. Soltou antes, ele recolhe. Completou, a Central nasce dali.",
               panels.build_hold(False), panels.H_MARKS,
               ["Os primeiros 200 ms não desenham nada (um toque não pisca).",
                "O fio enche em 450 ms; o rótulo “Central” sobe 6 px junto.",
                "Soltou antes: o fio esvazia em 220 ms e some (a ação é cancelada).",
                "Completou: o fio some e a faixa da Central (C1) nasce do mesmo canto.",
                "Hoje: 300 ms sem desenho + 300 ms de barra (600 ms). Proposta: 200 + 450 (650 ms), para a barra ser lida. Decisão sua.",
                "Com o relógio ligado (8.17), o comportamento de hoje fica: a linha enche na base da pílula."],
               hs, crop_r=(1100, 0, 820, 470))
    live_board("R8-H-segurar-com-relogio", "8.17 · Segurar CH+ com o relógio ligado (mantido)",
               "O comportamento atual continua: a linha enche dentro da pílula. Só a abertura da Central muda (C1).",
               panels.build_hold(True), panels.H_MARKS,
               ["Mesma linha de 3 px no osso, dentro da pílula.",
                "Mesmos tempos da 8.16, para os dois casos serem iguais.",
                "Ao completar, a pílula é a origem da faixa."],
               hs, crop_r=(1100, 0, 820, 470))

diag(); fundacao(); movimento(); menus_boards(); comparativo(); paineis()

print("ok", len(BOARDS), "pranchas em", OUT)
