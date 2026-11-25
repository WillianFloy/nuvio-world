# A ESPECIFICACAO NUMERICA das pranchas aprovadas (secao 8): cada elemento de
# cada peca, quadro a quadro (1/60 s), com x, y, w, h, opacidade, escala e raio.
# E desta tabela que o app tem de chegar perto (Fase 2 em diante: 1 px e 1 %).
#
#   python3 tools/r8/exportar.py /tmp/r8-espec            -> um .json por peca
#   python3 tools/r8/exportar.py /tmp/r8-espec S1 0.95     -> imprime o quadro t=0,95 s
#
# Os .json nao vao para o repositorio (sao gerados a qualquer hora deste codigo,
# que e o mesmo que gerou as pranchas: tools/r8/pranchas.py confere byte a byte).
import json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import base, menus, panels, fase1

FPS = 60
PECAS = {
    "MC": ("Menu de segurar OK · C Folha (padrao)", menus.build_C, menus.TL_MARKS),
    "MA": ("Menu de segurar OK · A Desdobrar", menus.build_A, menus.TL_MARKS),
    "S1": ("CH+ · S1 Estante (padrao)", panels.build_S1, panels.P_MARKS),
    "S2": ("CH+ · S2 Coluna editorial", panels.build_S2, panels.P_MARKS),
    "C2": ("CH+ segurado · C2 Coluna", panels.build_C2, panels.P_MARKS),
    "HO": ("Segurar CH+ sem relogio", lambda: panels.build_hold(False), panels.H_MARKS),
    "HR": ("Segurar CH+ com relogio", lambda: panels.build_hold(True), panels.H_MARKS),
    "BU": ("CH- · Busca (referencia antiga, substituida pela BF)", panels.build_busca, panels.P_MARKS),
    # Fase 1 (secao 10), aprovadas em 25/11 19:00
    "BF": ("CH- · Busca flutuante (10.13)", fase1.build_busca_flut, fase1.F1B_MARKS),
    "BM": ("CH- · Busca flutuante, estados (10.14)", fase1.build_busca_modos_flut, fase1.F1M_MARKS),
    "FHR": ("Segurar CH+ -> C2 com relogio (10.9)", lambda: fase1.build_hold_C2(True), panels.H_MARKS),
    "FHO": ("Segurar CH+ -> C2 sem relogio (10.10)", lambda: fase1.build_hold_C2(False), panels.H_MARKS),
    "FA": ("S1 abas (10.1)", fase1.build_S1_abas, fase1.F1A_MARKS),
    "F2": ("S2 abas e Organizar (10.5)", lambda: fase1.build_S2_abas(True), fase1.F2_MARKS),
    "FC": ("C2 Editar atalhos e cor (10.7)", fase1.build_C2_editar, fase1.F1C_MARKS),
}
CAMPOS = ("x", "y", "w", "h", "o", "s", "r")

def quadro(anim, t):
    out = {}
    for eid, fn, inner, style in anim.els:
        d = fn(t)
        out[eid] = {k: round(float(d.get(k, 1.0 if k in ("o", "s") else 0.0)), 3) for k in CAMPOS}
    return out

def exportar(chave):
    titulo, build, marcas = PECAS[chave]
    a = build()
    n = int(round(a.cycle * FPS))
    return {"peca": chave, "titulo": titulo, "fps": FPS, "ciclo_s": a.cycle,
            "marcas": [{"de": m[0], "ate": m[1], "o_que": m[2]} for m in marcas],
            "elementos": [e[0] for e in a.els],
            "quadros": [quadro(a, i / FPS) for i in range(n + 1)]}

if __name__ == "__main__":
    if len(sys.argv) == 4:
        a = PECAS[sys.argv[2]][1]()
        print(json.dumps(quadro(a, float(sys.argv[3])), ensure_ascii=False, indent=1))
        sys.exit(0)
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/r8-espec"
    os.makedirs(out, exist_ok=True)
    for k in PECAS:
        d = exportar(k)
        with open(os.path.join(out, k + ".json"), "w") as f:
            json.dump(d, f, ensure_ascii=False, separators=(",", ":"))
        print(f"{k}: {len(d['elementos'])} elementos x {len(d['quadros'])} quadros -> {out}/{k}.json")
