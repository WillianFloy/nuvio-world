# O COMPARADOR PRANCHA x APP. Para cada instante t: a prancha desenhada no
# Chrome (tools/r8/quadros.py) a esquerda, o quadro do app no mesmo t a direita
# (tests/r8_app_quadros_shot.sh), numa imagem; com --video, um mp4 dos dois.
#
#   bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4
#   python3 tools/r8/comparar.py BU /tmp/r8-app /tmp/r8-cmp 0.8 0.95 1.1 1.4
#   python3 tools/r8/comparar.py BU /tmp/r8-app /tmp/r8-cmp --de 0.7 --ate 1.6 --fps 30 --video
#
# Saidas: <saida>/lado-<peca>-<t>.png, <saida>/tira-<peca>.png (todos os t numa
# folha so) e, com --video, <saida>/<peca>.mp4 (prancha em cima, app embaixo).
import os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image, ImageDraw, ImageFont
import quadros
from exportar import PECAS

RAIZ = quadros.RAIZ
FONTE = os.path.join(RAIZ, "deploy", "app", "fonts", "Montserrat-Bold.ttf")
# A prancha de referencia de cada peca do app (o app pode ter um nome so dele).
REFERENCIA = {"SV": "S1"}

def rotulo(img, texto):
    d = ImageDraw.Draw(img)
    f = ImageFont.truetype(FONTE, 22)
    d.rectangle([0, 0, d.textlength(texto, font=f) + 24, 38], fill=(0, 0, 0))
    d.text((12, 6), texto, font=f, fill=(244, 242, 238))

def lado(prancha, app, t, peca_p, peca_a, vertical=False):
    w, h = 960, 540
    p = Image.open(prancha).convert("RGB").resize((w, h), Image.LANCZOS)
    a = Image.open(app).convert("RGB").resize((w, h), Image.LANCZOS) if app and os.path.exists(app) else Image.new("RGB", (w, h), (40, 0, 0))
    rotulo(p, f"PRANCHA {peca_p} · t = {t:.2f} s")
    rotulo(a, f"APP {peca_a} · t = {t:.2f} s" if app and os.path.exists(app) else f"APP: sem quadro em t = {t:.2f}")
    if vertical:
        out = Image.new("RGB", (w, h * 2 + 8), (30, 30, 32)); out.paste(p, (0, 0)); out.paste(a, (0, h + 8))
    else:
        out = Image.new("RGB", (w * 2 + 8, h), (30, 30, 32)); out.paste(p, (0, 0)); out.paste(a, (w + 8, 0))
    return out

def main():
    args = sys.argv[1:]
    video = "--video" in args
    if video: args.remove("--video")
    peca_a, dir_app, saida, resto = args[0], args[1], args[2], args[3:]
    peca_p = REFERENCIA.get(peca_a, peca_a)
    if resto and resto[0] == "--de":
        de, ate = float(resto[1]), float(resto[3])
        fps = float(resto[5]) if len(resto) > 5 else 30.0
        tempos = [de + i / fps for i in range(int(round((ate - de) * fps)) + 1)]
    else:
        tempos, fps = [float(x) for x in resto], 30.0
    os.makedirs(saida, exist_ok=True)
    pr = quadros.desenhar(peca_p, tempos, os.path.join(saida, "prancha"))
    lados = []
    for t, f in zip(tempos, pr):
        app = os.path.join(dir_app, f"app-{peca_a}-{t:.3f}.png")
        img = lado(f, app, t, peca_p, peca_a, vertical=video)
        if video:
            img.save(os.path.join(saida, f"_v{len(lados):04d}.png"))
            lados.append(None)
        else:
            arq = os.path.join(saida, f"lado-{peca_a}-{t:.3f}.png")
            img.save(arq); lados.append(img); print(arq)
    if video:
        mp4 = os.path.join(saida, f"{peca_a}.mp4")
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(fps), "-i", os.path.join(saida, "_v%04d.png"),
                        "-c:v", "libx264", "-preset", "veryfast", "-pix_fmt", "yuv420p", mp4], check=True)
        for i in range(len(lados)): os.remove(os.path.join(saida, f"_v{i:04d}.png"))
        print(mp4)
    else:
        w, h = lados[0].size
        tira = Image.new("RGB", (w, (h + 8) * len(lados)), (30, 30, 32))
        for i, im in enumerate(lados): tira.paste(im, (0, i * (h + 8)))
        arq = os.path.join(saida, f"tira-{peca_a}.png"); tira.save(arq); print(arq)

if __name__ == "__main__":
    main()
