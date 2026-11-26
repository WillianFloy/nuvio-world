# As trilhas dos menus de segurar OK nas pranchas (R8-MC Folha, R8-MA
# Desdobrar), a 60 quadros/s, no que NAO depende de onde o cartaz esta na tela
# (a Home do app nao e a da prancha): a Folha em coordenadas da tela; o
# Desdobrar relativo ao cartaz. Uma linha por quadro, ver tests/r8_menu_espec.sh.
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "r8"))
import exportar
peca, saida = sys.argv[1], sys.argv[2]
d = exportar.exportar(peca)
with open(saida, "w") as f:
    for i, q in enumerate(d["quadros"]):
        t = i / d["fps"]
        if peca == "MC":
            f.write(f"{t:.6f} {q['sheet']['x']} {q['sheet']['o']} {q['pill']['y']} {q['pill']['o']} "
                    f"{q['tit']['y']} {q['tit']['o']} {q['veil']['o']}\n")
        else:
            P = q["poster"]
            f.write(f"{t:.6f} {q['surf']['w'] - P['w']} {q['pill']['y'] - P['y']} {q['pill']['o']} "
                    f"{q['tit']['y'] - P['y']} {q['tit']['o']} {q['veil']['o']} {q['n3']['x'] - d['quadros'][0]['n3']['x']}\n")
