# Os icones das pranchas R8 (tools/r8/base.py IC + os de fase1.py) como os
# PNG que o app desenha (gfx_icone "r8_<nome>"): 128 x 128, traco 2 em 24
# unidades, pontas redondas, branco (a cor vem do gfx_icone). O .svg fica ao
# lado, como os ws_ de 23/11.
#   python3 tools/r8/icones.py            -> deploy/app/art/icones/r8_*.{svg,png}
import os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import base, fase1   # fase1 acrescenta ao base.IC os icones da secao 10

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
DEST = os.path.join(RAIZ, "deploy", "app", "art", "icones")

def svg(path):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 24 24" fill="none" '
            f'stroke="#fff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{path}</svg>')

def main():
    feitos = []
    for nome, path in sorted(base.IC.items()):
        s = os.path.join(DEST, f"r8_{nome}.svg"); p = os.path.join(DEST, f"r8_{nome}.png")
        open(s, "w").write(svg(path) + "\n")
        subprocess.run(["rsvg-convert", "-w", "128", "-h", "128", "-o", p, s], check=True)
        feitos.append(nome)
    print(len(feitos), "icones:", " ".join(feitos))

if __name__ == "__main__":
    main()
