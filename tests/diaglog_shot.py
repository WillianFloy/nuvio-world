#!/usr/bin/env python3
"""Le o QR da CAPTURA da tela da TV (tests/diaglog_shot.c) e abre o endereco.

Confere: o QR da fase Gravando e da fase Pronto decodificam para o endereco que
o app mostra; esse endereco responde (estado.json) e entrega o log.txt com o que
foi gravado entre Iniciar e Finalizar, sem o que veio antes.
Uso: diaglog_shot.py <binario> <pasta de trabalho> <prefixo das capturas>
"""
import json, os, subprocess, sys, urllib.request, urllib.error
import cv2

BIN, WORK, PREF = sys.argv[1:4]
falhas = 0
def ok(c, m):
    global falhas
    print(("PASS: " if c else "FAIL: ") + m, flush=True)
    falhas += 0 if c else 1

env = dict(os.environ, NUVIO_DADOS=os.path.join(WORK, "dados"), NUVIO_LOG=os.path.join(WORK, "nuvio.log"),
           NUVIO_DIAG_DIR=os.path.join(WORK, "diag"), NUVIO_LOGREDE_IP="127.0.0.1")
os.makedirs(env["NUVIO_DADOS"], exist_ok=True)
p = subprocess.Popen([BIN, PREF], stdin=subprocess.PIPE, stderr=subprocess.PIPE, env=env, text=True, bufsize=1)
info = {}
for l in p.stderr:
    l = l.strip()
    for k in ("URL", "PRONTO", "ESTADO-depois-de-voltar", "PAINEL-PELOS-AJUSTES"):
        if l.startswith(k + " "): info[k] = l[len(k) + 1:]
    if l.startswith("ESTADO "): info.setdefault("estados", []).append(l[7:])
    if l == "ESPERANDO": break

def qr(arq):
    img = cv2.imread(arq)
    if img is None: return ""
    txt, _, _ = cv2.QRCodeDetector().detectAndDecode(img)
    return txt

ok(info.get("PAINEL-PELOS-AJUSTES") == "1", "Ajustes: OK na linha 'Logs de diagnostico' abre o painel")
ok(info.get("estados", [None])[0] == "gravando", "OK em 'Iniciar logs' comecou a gravar")
ok(info.get("ESTADO-depois-de-voltar", "").startswith("gravando aberto=0"), "Voltar fecha o painel e a gravacao continua")
ok("pronto" in info.get("estados", []), "OK em 'Finalizar logs' levou ao arquivo pronto")
url = info.get("URL", "")
q1 = qr(PREF + "-2-gravando.bmp")
ok(url.startswith("http://127.0.0.1:") and q1 == url, f"QR da tela 'Gravando' decodifica para o endereco mostrado ({q1!r})")
q2 = qr(PREF + "-4-pronto.bmp")
ok(q2 == info.get("PRONTO") == url, f"QR da tela 'Pronto' e o MESMO endereco ({q2!r})")
try:
    with urllib.request.urlopen(q2 + "estado.json", timeout=5) as r: j = json.loads(r.read())
except Exception as e: j = {"erro": str(e)}
ok(j.get("estado") == "pronto" and j.get("restante", 0) > 100000, f"o endereco lido do QR responde: {j.get('estado')} restante={j.get('restante')}")
try:
    with urllib.request.urlopen(q2 + "log.txt", timeout=5) as r: t = r.read().decode()
except Exception as e: t = ""
ok("n=39" in t and "ANTES da sessao" not in t and "SEGREDOANTES" not in t,
   "log.txt baixado pelo endereco do QR: as 40 linhas gravadas, nada de antes (caminho sem o fio do Android)")
p.stdin.write("sair\n"); p.stdin.flush(); p.wait(10)
print("diaglog_shot: " + ("ok" if not falhas else f"{falhas} falha(s)"))
sys.exit(1 if falhas else 0)
