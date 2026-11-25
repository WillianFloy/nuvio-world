# FASE 1 (plano 25/11): secao 10 do canvas, os complementos das pranchas da secao 8.
import json, os, sys
from base import *
import fase1 as F

OUT = sys.argv[1] if len(sys.argv) > 1 else "/home/claude/dc3/project"
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
def live_board(name, title, sub, anim, marks, notes, strip):
    inner, css = anim.live()
    n = len(strip); fw = (1728 - (n - 1) * 18) / n; fh = fw * SH / SW
    H = int(236 + 1110 * SH / SW + 26 + 110 + fh + 60)
    b = box(0, 0, SW, H, "background:#0A0A0B") + head(title, sub)
    b += live_frame(inner, 96, 236, 1110, f"PRÉVIA AO VIVO · LOOP DE {anim.cycle:g} S".replace(".", ","))
    yb = 236 + 1110 * SH / SW + 26
    b += timeline_bar(96, yb, 1110, marks, anim.cycle)
    o, _ = bullets(1260, 236, notes, 564, 19, 14)
    b += txt(1260, 198, "COMO SE MOVE", 16, DIM, 700, "letter-spacing:.12em") + o
    y = yb + 110
    for i, (t, lab) in enumerate(strip):
        b += frame(anim.static(t), 96 + i * (fw + 18), y, fw, lab)
    write(name, title, b, css, h=H)

live_board("R10-S1-abas", "10.1 · S1 Estante · as abas reais",
  "Salvos, Atividade, Amigos, Avisos e Agenda, como no app. A aba ativa acende; o conteúdo troca no lugar, sem a estante sair.",
  F.build_S1_abas(), F.F1A_MARKS,
  ["Abrir e fechar: exatamente os tempos da 8.11 (a estante sobe da base, a Home sobe 60 px e escurece).",
   "Para cima no primeiro cartão: o anel sobe para a aba ativa (mola de 270 ms). Esquerda/direita trocam de aba na hora.",
   "Troca de aba: o conteúdo antigo sai 12 px para a esquerda e o novo entra 12 px pela direita, em 160 ms (cartões, legendas, banner e pílulas juntos).",
   "As pílulas da direita são as de cada aba: Organizar em Salvos, Quem vê/Adicionar em Amigos, Limpar em Avisos, Agenda completa em Agenda.",
   "Atividade mostra quem mandou (avatar no cartão); Amigos mostra a pessoa; o banner é sempre do item em foco ou do primeiro da aba.",
   "O lápis no fim das abas abre a edição das abas (10.3)."],
  [(1.4, "aberta · Salvos"), (2.7, "anel nas abas"), (3.45, "Atividade"), (4.4, "Amigos"), (6.0, "Agenda · foco desce")])
live_board("R10-S1-organizar", "10.2 · S1 Estante · Organizar (Agrupar e Nova categoria)",
  "As opções nascem da própria pílula: a folha cresce primeiro na largura, depois na altura. Ao escolher, a estante se regrupa no lugar.",
  F.build_S1_org(), F.F1O_MARKS,
  ["OK na pílula: uma folha do material das pranchas nasce do retângulo da pílula (largura 0–55 %, altura 15–100 %, 440 ms).",
   "As opções entram em cascata; a marca de escolhida fica à direita; o anel percorre as linhas com mola.",
   "Escolher “Tipo”: a marca troca, a folha volta para a pílula em 260 ms e a pílula mostra o novo valor.",
   "Regrupar: as estantes antigas saem 12 px e as novas entram (160 ms). A estante não fecha.",
   "“+ Categoria”: a folha traz o campo do nome e o mesmo teclado da Busca; “Criar categoria” no fim.",
   "Ordenar abre do mesmo jeito (mesma folha, opções diferentes)."],
  [(2.0, "Agrupar aberto"), (2.7, "escolhendo"), (3.8, "regrupado"), (6.0, "Nova categoria"), (7.2, "Criar")])
live_board("R10-S1-editar", "10.3 · S1 Estante · editar as abas",
  "O lápis põe as abas em modo de edição: cada aba vira uma pílula, as ações nascem embaixo da aba em foco e as outras abas abrem espaço com mola.",
  F.build_S1_editar(), F.F1E_MARKS,
  ["Entrar: as abas ganham o fundo de pílula em 300 ms; as pílulas de Organizar dão lugar a “Concluir”; a estante desce 76 px e escurece.",
   "OK numa aba: três ações nascem embaixo dela (mover à esquerda, mover à direita, esconder), em cascata.",
   "Mover: a aba troca de lugar com a vizinha; as duas deslizam com mola (270 ms). As ações seguem a aba.",
   "Esconder: a aba fica a 40 % com o ícone de olho riscado. Só sai da fileira ao concluir.",
   "Concluir: a aba escondida encolhe até zero, as outras fecham o vão, a estante volta e o foco desce para o primeiro cartão."],
  [(2.3, "edição"), (3.0, "ações da Agenda"), (3.6, "movida"), (5.3, "Avisos escondido"), (6.8, "concluído")])
live_board("R10-S1-vazia", "10.4 · S1 Estante · sem conteúdo",
  "Quando não há salvos (ou a aba está vazia) a estante abre igual, mas no lugar dos cartões vem uma mensagem curta com uma ação.",
  F.build_S1_vazia(), F.F1V_MARKS,
  ["Sem item em foco, não há banner: o terço de cima fica escuro (a Home a 14 %).",
   "Salvos vazio: “Nada salvo ainda” e o botão Explorar (foco inicial nele).",
   "Abas que ainda carregam mostram três pontos, nunca cartões vazios.",
   "Avisos vazio: “Nenhum aviso”, sem botão. A troca de aba é a mesma da 10.1."],
  [(1.6, "Salvos vazio"), (3.2, "carregando"), (4.6, "Avisos vazio"), (8.3, "fechando")])
live_board("R10-S2-abas", "10.5 · S2 Folha · abas e Organizar (relógio ligado)",
  "A folha da 8.12 com as abas reais como ícones redondos (só a ativa mostra o nome) e o Organizar dentro dela.",
  F.build_S2_abas(True), F.F2_MARKS,
  ["Com o relógio ligado a folha começa em y 112, abaixo da barra e do relógio (decisão 2).",
   "Abas: 6 ícones de 52 px; a ativa se alarga com o nome (mola). A troca de conteúdo é a da 10.1.",
   "O item em foco expande como na 8.12 (miniatura 568 × 320 e as ações); ao subir para as abas ele recolhe.",
   "Ordenar: as opções abrem embaixo da pílula e empurram a lista; ao escolher, a lista se reordena com mola e a pílula mostra o novo valor.",
   "A lista nunca passa da folha: as linhas somem nos últimos 50 px."],
  [(1.5, "aberta"), (2.7, "Atividade"), (4.7, "Ordenar"), (7.0, "reordenada")])
live_board("R10-S2-sem-relogio", "10.6 · S2 Folha · relógio desligado",
  "Mesma folha, ocupando a altura toda (y 36 a 1032), porque não há relógio para respeitar.",
  F.build_S2_abas(False), F.F2_MARKS,
  ["Única diferença para a 10.5: a folha começa em y 36 e tem 996 px de altura.",
   "Os tempos, a troca de aba e o Organizar são idênticos."],
  [(1.5, "aberta"), (4.7, "Ordenar")])
live_board("R10-C2-editar", "10.7 · C2 Coluna · Editar atalhos e cor de destaque",
  "As páginas internas da Central trocam dentro da própria Coluna, sem abrir outra tela.",
  F.build_C2_editar(), F.F1C_MARKS,
  ["Abrir: a Coluna da 8.15 (nasce da pílula do relógio).",
   "Editar atalhos: a página atual sai 12 px e a nova entra (160 ms); o título ganha a seta de voltar e o contador 7/25.",
   "Ligar um atalho: o círculo vira marca cheia com um pulo de mola; o contador passa a 8/25.",
   "Cor de destaque: a bandeja de 18 cores abre embaixo da linha e a lista rola 150 px para caber; o anel anda pelas cores.",
   "A cor escolhida tinge na hora o acento (a marca da linha em foco). Voltar desfaz a troca de página."],
  [(1.0, "Central"), (2.5, "Editar atalhos"), (3.4, "Legendas ligado"), (5.2, "cor de destaque")])
live_board("R10-C2-player", "10.8 · C2 Coluna · no player",
  "No player a Coluna traz o que está tocando e os ajustes do vídeo; Estatísticas é uma página dela.",
  F.build_C2_player(), F.F1P_MARKS,
  ["Abre igual (mesma superfície e tempos); não há barra no player, a origem continua sendo o canto do relógio.",
   "Topo: miniatura, título, episódio, a barra de progresso e os tempos.",
   "Linhas: Áudio, Legendas, Qualidade, Velocidade e, em App, Brilho da legenda e Estatísticas.",
   "Estatísticas: resolução, vídeo, áudio, taxa, conexões, buffer, quadros perdidos e fonte, mais a taxa dos últimos 30 s.",
   "A legenda do vídeo continua visível (estreita para não ficar sob a Coluna)."],
  [(1.2, "no player"), (2.5, "foco"), (4.5, "Estatísticas")])
live_board("R10-H-C2-com-relogio", "10.9 · Segurar CH+ → C2 Coluna (relógio ligado)",
  "A 8.16 terminando na Coluna: o fio enche na pílula do relógio e, ao completar, a pílula vira a Coluna.",
  F.build_hold_C2(True), F.H_MARKS,
  ["Tempos da decisão 4: 200 ms sem nada desenhado + 450 ms enchendo (650 ms).",
   "Toque curto (até 200 ms) não mostra nada.",
   "Ao completar, a pílula se abre na Coluna (largura primeiro) e o relógio desliza para o topo dela.",
   "As linhas entram em cascata de 30 ms."],
  [(2.5, "enchendo"), (2.9, "completou"), (3.1, "abrindo"), (3.6, "aberta")])
live_board("R10-H-C2-sem-relogio", "10.10 · Segurar CH+ → C2 Coluna (relógio desligado)",
  "Sem relógio, o fio aparece onde ele estaria, com “Central”; ao completar, é o fio que vira a Coluna.",
  F.build_hold_C2(False), F.H_MARKS,
  ["O fio de 160 px e o rótulo são os da 8.17.",
   "A Coluna nasce do fio cheio (retângulo de 160 × 14 px) e cresce até 520 × 900.",
   "A hora entra em cascata com as linhas (não há pílula para viajar)."],
  [(2.5, "enchendo"), (2.9, "completou"), (3.1, "abrindo"), (3.6, "aberta")])
live_board("R10-B-busca-meio", "10.11 · CH− Busca · nasce no meio, em cima",
  "Decisão 5: sempre do meio. A pílula nasce no meio, em cima, deslizando de fora da tela até a linha da barra, e ali se abre no campo grudado no topo; o corpo desce dele.",
  F.build_busca_meio(), F.F1B_MARKS,
  ["0–200 ms: uma pílula de 464 × 52, centrada na horizontal, desliza de cima (y −64) até a linha da barra (y 40), com a lupa e o texto de ajuda.",
   "160–550 ms: ali mesmo ela se abre no campo de 1100 × 128 grudado no topo (largura antes da altura); a lupa vai para a esquerda e o texto cresce de 20 para 26 px.",
   "A barra do topo e o relógio somem nos primeiros 160 ms; a Home recua 2,5 % e escurece 60 %.",
   "450–650 ms: o corpo desce do campo; as linhas do teclado sobem em cascata.",
   "Resultados: melhor resultado, títulos, pessoas e canais, em cascata de 60 ms.",
   "Fechar: o caminho inverso; a pílula volta a subir para fora da tela (300 ms)."],
  [(0.9, "deslizando"), (1.05, "na linha da barra"), (1.3, "abrindo"), (3.9, "resultados"), (7.75, "fechando")])
live_board("R10-B-busca-modos", "10.12 · CH− Busca · recentes, voz, celular e nada encontrado",
  "Os estados da Busca que a 8.18 não mostra. O lado esquerdo (teclado, voz, celular) e o direito (recentes, resultados) trocam com a mesma troca de 160 ms.",
  F.build_busca_modos(), F.F1M_MARKS,
  ["Antes de digitar: Recentes (com X para apagar) e Em alta.",
   "Microfone: o teclado dá lugar ao cartão de voz (“Ouvindo…”, sugestões); o que foi entendido vai para o campo e os resultados chegam.",
   "Celular: o cartão traz o QR e o código para digitar pelo celular.",
   "Nada encontrado: mensagem curta com o que tentar, no lugar dos resultados."],
  [(1.5, "recentes"), (2.8, "voz"), (5.2, "celular"), (7.2, "nada encontrado")])

if __name__ == "__main__" and len(sys.argv) < 3:
    idx = json.load(open("/home/claude/dc3/project/canvas.json"))
    Y0 = 29400
    idx["notes"]["t10"] = dict(kind="title1", maxW=7920, text="10 · Fase 1: complementos das pranchas da seção 8 (para aprovar)", x=0, y=Y0 - 300)
    for i, (name, title, h) in enumerate(BOARDS):
        x, y = (i % 3) * 2000, Y0 + (i // 3) * 1700
        f = name + ".dc.html"
        idx["boards"][f] = dict(x=x, y=y, w=SW, h=h, title=title)
        if f not in idx["order"]: idx["order"].append(f)
    json.dump(idx, open(f"{OUT}/canvas.json", "w"), ensure_ascii=False, indent=1)
print("ok", [(n, os.path.getsize(f"{OUT}/{n}.dc.html") // 1024, h) for n, _, h in BOARDS])
