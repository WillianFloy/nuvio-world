# A trilha do menu de segurar OK na prancha R8-MC (Folha), a 60 quadros/s, no
# que NAO depende de onde o cartaz esta na tela (a Home do app nao e a da
# prancha): a Folha em coordenadas da tela. Uma linha por quadro, ver
# tests/r8_menu_espec.sh. (O Desdobrar, R8-MA, saiu do app em 27/11.)
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "r8"))
import exportar
peca, saida = sys.argv[1], sys.argv[2]
if peca != "MC":
    sys.exit("so a Folha (MC)")
d = exportar.exportar(peca)
with open(saida, "w") as f:
    for i, q in enumerate(d["quadros"]):
        t = i / d["fps"]
        f.write(f"{t:.6f} {q['sheet']['x']} {q['sheet']['o']} {q['pill']['y']} {q['pill']['o']} "
                f"{q['tit']['y']} {q['tit']['o']} {q['veil']['o']}\n")
