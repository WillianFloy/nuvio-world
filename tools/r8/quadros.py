# QUADROS DA PRANCHA: desenha a peca no instante t (s), 1920x1080, num Chrome sem
# tela, com a Montserrat do proprio app (deploy/app/fonts). E o lado "prancha"
# do comparador (tools/r8/comparar.py).
#
#   python3 tools/r8/quadros.py MC /tmp/r8-q 1.0 1.42 1.6     -> MC-1.000.png ...
#   python3 tools/r8/quadros.py MC /tmp/r8-q --de 1.2 --ate 1.9 --fps 30
#
# Fontes: o app tem Montserrat 400/500/700 (a ExtraBold 800 entra na Fase 2);
# ate la o Chrome sintetiza o 600/800 a partir das que existem.
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import base
from exportar import PECAS

RAIZ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FONTES = os.path.join(RAIZ, "deploy", "app", "fonts")

def fontes_css():
    pesos = [(400, "Montserrat-Regular.ttf"), (500, "Montserrat-Medium.ttf"), (700, "Montserrat-Bold.ttf"),
             (800, "Montserrat-ExtraBold.ttf")]
    css = ""
    for p, f in pesos:
        cam = os.path.join(FONTES, f)
        if os.path.exists(cam):
            css += f"@font-face{{font-family:Montserrat;font-weight:{p};src:url('file://{cam}')}}\n"
    return css

def pagina(cena):
    return ("<!doctype html><html><head><meta charset='utf-8'><style>" + fontes_css() +
            "html,body{margin:0;background:#000}*{box-sizing:border-box}</style></head><body>"
            f"<div style='width:1920px;height:1080px;position:relative;overflow:hidden;background:#000;color:{base.BONE};"
            f"font-family:Montserrat,system-ui,sans-serif;-webkit-font-smoothing:antialiased'>{cena}</div></body></html>")

def desenhar(peca, tempos, saida):
    from playwright.sync_api import sync_playwright
    a = PECAS[peca][1]()
    os.makedirs(saida, exist_ok=True)
    arquivos = []
    with sync_playwright() as p:
        nav = p.chromium.launch(executable_path="/usr/bin/google-chrome", args=["--allow-file-access-from-files"])
        pg = nav.new_page(viewport={"width": 1920, "height": 1080})
        for t in tempos:
            html = os.path.join(saida, f"_{peca}.html")
            open(html, "w").write(pagina(a.static(t)))
            pg.goto("file://" + html)
            pg.evaluate("document.fonts.ready")
            arq = os.path.join(saida, f"{peca}-{t:.3f}.png")
            pg.screenshot(path=arq)
            arquivos.append(arq)
        nav.close()
    return arquivos

if __name__ == "__main__":
    peca, saida, resto = sys.argv[1], sys.argv[2], sys.argv[3:]
    if resto and resto[0] == "--de":
        de, ate, fps = float(resto[1]), float(resto[3]), float(resto[5]) if len(resto) > 5 else 30.0
        n = int(round((ate - de) * fps))
        tempos = [de + i / fps for i in range(n + 1)]
    else:
        tempos = [float(x) for x in resto]
    for f in desenhar(peca, tempos, saida):
        print(f)
