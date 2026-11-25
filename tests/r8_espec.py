# A especificacao das pranchas continua sendo a APROVADA e e consistente.
#   bash tests/r8_espec.sh
# 1. O gerador versionado (tools/r8) refaz as 17 pranchas da secao 8 byte a byte
#    iguais as publicadas (hashes em tools/r8/aprovadas.sha256).
# 2. A exportacao quadro a quadro tem ids unicos, 60 quadros por segundo e os
#    estados das pranchas: fechado no inicio e no fim do ciclo, aberto no meio.
import hashlib, os, subprocess, sys, tempfile
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, "tools", "r8"))
falhas = 0
def ok(c, m):
    global falhas
    print(("ok: " if c else "FALHA: ") + m)
    if not c: falhas += 1

out = tempfile.mkdtemp(prefix="r8-pranchas-")
subprocess.run([sys.executable, os.path.join(RAIZ, "tools", "r8", "pranchas.py"), out], check=True, stdout=subprocess.DEVNULL)
esperado = {}
for linha in open(os.path.join(RAIZ, "tools", "r8", "aprovadas.sha256")):
    h, nome = linha.split()
    esperado[nome] = h
for nome, h in sorted(esperado.items()):
    got = hashlib.sha256(open(os.path.join(out, nome), "rb").read()).hexdigest()
    ok(got == h, f"{nome} igual a prancha aprovada")

import exportar
for k in exportar.PECAS:
    d = exportar.exportar(k)
    ids = d["elementos"]
    ok(len(ids) == len(set(ids)), f"{k}: ids unicos ({len(ids)})")
    ok(len(d["quadros"]) == int(d["ciclo_s"] * 60) + 1, f"{k}: 60 quadros por segundo")
    q0, qm, qf = d["quadros"][0], d["quadros"][int(len(d["quadros"]) * 0.55)], d["quadros"][-1]
    if "veil" in q0:
        ok(q0["veil"]["o"] < 0.01 and qf["veil"]["o"] < 0.01, f"{k}: veu apagado com tudo fechado")
        ok(qm["veil"]["o"] > 0.3, f"{k}: veu aceso com o painel aberto ({qm['veil']['o']})")
print("r8_espec: " + ("FALHOU" if falhas else "ok"))
sys.exit(1 if falhas else 0)
