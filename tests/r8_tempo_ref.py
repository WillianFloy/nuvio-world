# Tira da especificacao das pranchas (tools/r8/exportar.py, 60 fps) as trilhas
# que o modulo de tempo (src/r8tempo.c) tem de reproduzir. Uma linha por
# quadro: PECA ELEMENTO CAMPO t valor. Ver tests/r8_tempo.sh.
import json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "r8"))
import exportar

TRILHAS = {
    "S1": [("bg", "y"), ("bg", "s"), ("bg", "o"), ("heroT0", "o"), ("heroT1", "o"), ("hero1", "o")],
    "C2": [("card", "x"), ("card", "w"), ("card", "h")],
    "MC": [("veil", "o"), ("sheet", "x"), ("sheet", "o")],
    "HR": [("fill", "w"), ("fill", "o")],
    "FHR": [("card", "x"), ("card", "w"), ("card", "h")],
    "BF": [("field", "x"), ("field", "y"), ("field", "w"), ("field", "h"), ("corpo", "h"), ("veil", "o")],
}
out = sys.stdout if len(sys.argv) < 2 else open(sys.argv[1], "w")
for peca, trilhas in TRILHAS.items():
    d = exportar.exportar(peca)
    for i, q in enumerate(d["quadros"]):
        t = i / d["fps"]
        for el, campo in trilhas:
            out.write(f"{peca} {el} {campo} {t:.6f} {q[el][campo]}\n")
