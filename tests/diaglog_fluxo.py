#!/usr/bin/env python3
"""Fluxo completo das sessoes de diagnostico contra a TV simulada (tests/diaglog_tv.c).

Usa o servidor de verdade (logrede.c) em 127.0.0.1, curl para HTTP e, se houver,
o Chrome headless para abrir a pagina em tamanho de celular e de computador.
Confere: servidor so com sessao; pagina/estado/log.txt em cada fase; o arquivo
so com o que veio entre as marcas, com as linhas de P1/P2 intactas e sem
segredos; fechamento aos 120 s; download lento que atravessa o prazo inteiro;
sessao nova sem misturar. Uso: diaglog_fluxo.py <binario> <pasta de trabalho>
"""
import os, re, shutil, subprocess, sys, threading, time, json, urllib.request, urllib.error

BIN, WORK = sys.argv[1], sys.argv[2]
SHOTS = os.environ.get("DIAG_SHOTS", "")
falhas = 0
def ok(c, msg):
    global falhas
    print(("PASS: " if c else "FAIL: ") + msg, flush=True)
    if not c: falhas += 1

env = dict(os.environ, NUVIO_DIAG_DIR=os.path.join(WORK, "diag"), NUVIO_LOG=os.path.join(WORK, "nuvio.log"),
           NUVIO_LOGREDE_IP="127.0.0.1", NUVIO_TV_INFO="TCL Smart TV Pro|34|14|2.0.45-teste")
tv = subprocess.Popen([BIN], stdin=subprocess.PIPE, stderr=subprocess.PIPE, stdout=subprocess.DEVNULL, env=env, text=True, bufsize=1)
eventos, cond = [], threading.Condition()
def ler():
    for l in tv.stderr:
        if not l.startswith("EV "): continue
        d = dict(re.findall(r"(\w+)=(\S*)", l)); d["ev"] = l.split()[1]
        with cond: eventos.append(d); cond.notify_all()
threading.Thread(target=ler, daemon=True).start()
def cmd(c): tv.stdin.write(c + "\n"); tv.stdin.flush()
def espera(pred, prazo=10.0, desde=0):
    fim = time.time() + prazo
    with cond:
        while True:
            for d in eventos[desde:]:
                if pred(d): return d
            r = fim - time.time()
            if r <= 0: return None
            cond.wait(r)

def http(url, metodo="GET"):
    try:
        req = urllib.request.Request(url, method=metodo)
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, dict(r.headers), r.read()
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers), e.read()
    except Exception as e:
        return None, {}, str(e).encode()

CHROME = shutil.which("google-chrome") or shutil.which("chromium")
def chrome(url, nome, w, h):
    if not CHROME: return None
    perfil = os.path.join(WORK, "chrome-" + nome)
    base = [CHROME, "--headless=new", "--no-sandbox", "--disable-gpu", "--hide-scrollbars", f"--user-data-dir={perfil}",
            f"--window-size={w},{h}", "--virtual-time-budget=4000"]
    dom = subprocess.run(base + ["--dump-dom", url], capture_output=True, text=True, timeout=60).stdout
    if SHOTS:
        subprocess.run(base + [f"--screenshot={os.path.join(SHOTS, nome + '.png')}", url], capture_output=True, timeout=60)
    return dom

espera(lambda d: d["ev"] == "pronta")
st, _, _ = http("http://127.0.0.1:8099/")
ok(st is None, "sem sessao o servidor nao responde (porta fechada)")

# --- 1. iniciar --------------------------------------------------------------
n0 = len(eventos)
cmd("iniciar")
d = espera(lambda d: d["ev"] == "servidor-ligado", 8, n0)
ini = espera(lambda d: d["ev"] == "iniciou", 2, n0)
ok(ini is not None and d is not None, "Iniciar logs: sessao gravando e servidor ligado")
cod = ini["codigo"] if ini else ""
url = f"http://127.0.0.1:8099/s/{cod}/"
st, cab, corpo = http("http://127.0.0.1:8099/")
ok(st == 302 or (st == 200 and b"Nuvio" in corpo), "a raiz leva a sessao atual (quem digitou so o IP)")
LINHAS_P = [
    "[tela] hz-ativo=60.00 ativo=2:3840x2160@60.00 mesma-resolucao=[60.00,50.00] alternativas-sem-corte=[]",
    "[ritmo] tela=60.00Hz ref=16.67ms(tela) | apresentados=108 fps=36.0 ritmo-medio=27.78ms | frame p50=33.3 p95=33.4 p99=33.5 max=50.1ms | fora-do-prazo=72(67%) varreduras-puladas=72 hist(1/2/3/4+)=36/72/0/0 | cpu p50=6.1 p95=9.8 | swap p50=20.1 p95=26.0 | controle->tela n=4 p50=48 max=66ms",
    "[swap] ritmo=27.80ms ref=16.67ms med=27.78ms pior=50.1ms n=108 limite-jank=33.3ms",
    "FPS=36.0 pior=50.1ms janks=40 | pior-quadro: texto 0.4ms em 2 linhas",
    "[vidro-ritmo] nivel=2 escada-ritmo=2 degrau=0 qualidade=0 reusos=54",
    "[gpu-escada] vidro ritmo 1 -> 2 fps 36.0 alvo 60.0 (fundo a cada 3 quadros, 1 passada, ainda fosco)",
    "[gpu-nivel] live decision: level 0 -> 0 fps=40.0 target=60.0 slow=3 fast=0 step=1",
    "[gpu-escada] step 0 -> 1 fps 40.0/60.0, kept, gain >=10% and >=3 fps",
    "[qd-lento] 66.7ms tecla=Right ha 120ms | ev=0.1 bomb=0.0(0 tex 0.0MB) upd=2.1 clr=40.2 des=5.5 swap=18.0 txt=0.0ms/0 rects=310 fill=1.77 gpu~=0.0 vidro cap=1 pass=3 reusos=0 | home r2 c5",
]
SEGREDOS = ["[addons] GET https://torrentio.strem.fun/realdebrid=ABCDEF1234567890ABCDEF1234567890/stream/movie/tt123.json apikey=XYZ12345",
            "[conta] token: abcdefghijklmnop Authorization: Bearer eyJhbGciOiJIUzI1NiJ9.cGF5bG9hZHBheWxvYWQ.c2ln",
            "[debrid] chave 0123456789abcdef0123456789abcdef0123 senha=hunter2pass"]
for l in LINHAS_P + SEGREDOS: cmd("linha " + l)
time.sleep(0.5)

st, cab, corpo = http(url)
ok(st == 200 and b"<title>Nuvio logs</title>" in corpo and b"viewport" in corpo, "pagina da sessao (HTML responsivo) durante a gravacao")
st, _, corpo = http(url + "estado.json")
j = json.loads(corpo) if st == 200 else {}
ok(j.get("estado") == "gravando" and j.get("decorrido", -1) >= 0 and j.get("pt") == 1, f"estado.json gravando: {j.get('estado')} decorrido={j.get('decorrido')}")
st, _, corpo = http(url + "log.txt")
ok(st == 409 and b"coletados" in corpo, "log.txt antes de finalizar: 409 'ainda sendo coletados'")
st, _, _ = http(f"http://127.0.0.1:8099/s/zzzzzz/")
ok(st == 410, "codigo errado: 410 sessao encerrada")
dom = chrome(url, "web-celular-gravando", 390, 844)
if dom is not None:
    ok("Gravando" in dom and "Os logs ainda est" in dom and "btn off" in dom, "Chrome (celular 390x844): 'Gravando', aviso de coleta, botao desativado")
dom = chrome(url, "web-computador-gravando", 1366, 820)
if dom is not None: ok("Gravando" in dom, "Chrome (computador 1366x820): 'Gravando'")

cmd("volume 4")   # ~4 MB para o download lento
espera(lambda d: d["ev"] == "volume", 30, n0)

# --- 2. finalizar ---------------------------------------------------------------
n1 = len(eventos)
cmd("finalizar")
p = espera(lambda d: d["ev"] == "mudou" and d["estado"] == "pronto", 15, n1)
ok(p is not None and p.get("confirmado") == "1", "Finalizar: arquivo pronto, marca de fim confirmada")
# O instante em que o arquivo ficou pronto, pelo relogio da propria TV: o
# evento sai no tick seguinte, entao desconta o que ja tinha corrido do prazo.
t_pronto = float(p["t"]) - (120000 - int(p["restante"])) / 1000.0 if p else time.monotonic()
t_pronto_parede = time.time()
ok(espera(lambda d: d["ev"] == "servidor-desligado", 0.5, n1) is None, "Finalizar NAO desliga o servidor")
st, _, corpo = http(url + "estado.json")
j = json.loads(corpo) if st == 200 else {}
ok(j.get("estado") == "pronto" and 118000 <= j.get("restante", 0) <= 120000, f"estado.json pronto, restante={j.get('restante')} ms")
dom = chrome(url, "web-celular-pronto", 390, 844)
if dom is not None:
    ok("Pronto para baixar" in dom and "class=\"btn\"" in dom and "Baixar log.txt" in dom, "Chrome (celular): 'Pronto para baixar' e botao Baixar log.txt ativo")
dom = chrome(url, "web-computador-pronto", 1366, 820)
if dom is not None: ok("Pronto para baixar" in dom, "Chrome (computador): 'Pronto para baixar'")

st, cab, corpo = http(url + "log.txt")
txt = corpo.decode("utf-8", "replace")
ok(st == 200 and "attachment" in cab.get("Content-Disposition", "") and int(cab.get("Content-Length", -1)) == len(corpo),
   f"download log.txt: 200, attachment ({cab.get('Content-Disposition','')}), {len(corpo)} bytes = Content-Length")
open(os.path.join(WORK, "sessao1.txt"), "wb").write(corpo)
tam_sessao1 = len(corpo)
ok(txt.startswith("# Nuvio - sessao de diagnostico " + cod), "arquivo abre com o cabecalho da sessao")
ok(f"[diag] ==== inicio da sessao {cod} ====" in txt and f"[diag] ==== fim da sessao {cod} ====" in txt, "marcas de inicio e fim no arquivo")
ok("ANTES da sessao" not in txt and "SEGREDOANTES" not in txt, "nada de antes de Iniciar entrou no arquivo")
for l in LINHAS_P: ok(l in txt, "linha de P1/P2 intacta: " + l[:48])
for s in ["ABCDEF1234567890ABCDEF", "XYZ12345", "abcdefghijklmnop", "eyJhbGci", "hunter2pass", "0123456789abcdef0123456789abcdef"]:
    ok(s not in txt, "segredo removido: " + s[:10] + "...")
ok("torrentio.strem.fun/…" in txt, "URL cortada no host")
ok(txt.rstrip("\n").endswith(f"[diag] ==== fim da sessao {cod} ===="), "o arquivo termina na marca de fim (nada depois)")

# --- 3. prazo de 120 s, com um download lento atravessando ---------------------------
lento = {}
def baixar_lento():
    t0 = time.time()
    r = subprocess.run(["curl", "-s", "-o", os.path.join(WORK, "lento.txt"), "--limit-rate", "120k", "-w", "%{http_code} %{size_download}", url + "log.txt"],
                       capture_output=True, text=True)
    lento.update(fim=time.time(), ini=t0, saida=r.stdout)
while time.monotonic() < t_pronto + 112: time.sleep(0.2)
th = threading.Thread(target=baixar_lento); th.start()
while time.monotonic() < t_pronto + 119: time.sleep(0.05)
st, _, corpo = http(url + "estado.json")
j = json.loads(corpo) if st == 200 else {}
ok(j.get("estado") == "pronto" and 0 < j.get("restante", 0) <= 1100, f"aos 119 s ainda pronto, restante={j.get('restante')} ms")
d = espera(lambda d: d["ev"] == "servidor-desligado", 5, n1)
dt = float(d["t"]) - t_pronto if d else -1
ok(d is not None and 120.0 <= dt <= 120.3, f"servidor desligou {dt:.3f} s depois do arquivo pronto (alvo 120 s)")
st, _, _ = http(url + "estado.json")
ok(st is None, "depois do prazo: o endereco nao responde (porta fechada)")
th.join(120)
tam = os.path.getsize(os.path.join(WORK, "lento.txt")) if os.path.exists(os.path.join(WORK, "lento.txt")) else -1
ok(lento.get("saida", "").startswith("200") and tam == tam_sessao1 and lento.get("fim", 0) > time.time() - 300 and
   lento.get("ini", 0) < t_pronto_parede + 120 < lento.get("fim", 0),
   f"download lento iniciado aos 112 s terminou depois do prazo e completo ({tam} de {tam_sessao1} bytes, {lento.get('fim',0)-lento.get('ini',0):.0f} s)")

# --- 4. sessao nova, sem misturar; e "Nova sessao" no meio da janela -------------------
n2 = len(eventos)
cmd("iniciar")
i2 = espera(lambda d: d["ev"] == "iniciou", 3, n2)
cod2 = i2["codigo"] if i2 else ""
ok(cod2 and cod2 != cod, f"nova sessao com codigo novo ({cod} -> {cod2})")
espera(lambda d: d["ev"] == "servidor-ligado", 8, n2)
st, _, _ = http(url + "estado.json")
ok(st == 410, "endereco da sessao anterior: 410 encerrada")
cmd("linha [tv] linha da segunda sessao")
time.sleep(0.3)
cmd("finalizar")
espera(lambda d: d["ev"] == "mudou" and d["estado"] == "pronto", 15, n2)
url2 = f"http://127.0.0.1:8099/s/{cod2}/"
st, _, corpo2 = http(url2 + "log.txt")
t2 = corpo2.decode("utf-8", "replace")
ok(st == 200 and "linha da segunda sessao" in t2 and "volume 00000001" not in t2 and cod not in t2,
   "segunda sessao: so as proprias linhas (nada da primeira)")
ok(len([f for f in os.listdir(env["NUVIO_DIAG_DIR"]) if f.endswith(".txt")]) == 1, "na pasta so o arquivo da sessao atual")
n3 = len(eventos)
cmd("iniciar")   # "Nova sessao" com a anterior ainda pronta para baixar
i3 = espera(lambda d: d["ev"] == "iniciou", 3, n3)
st, _, _ = http(url2 + "log.txt")
ok(i3 is not None and st == 410, "Nova sessao durante a janela: o endereco anterior morre na hora")
cmd("sair")
tv.wait(5)
print(f"diaglog_fluxo: {'ok' if not falhas else str(falhas) + ' falha(s)'}")
sys.exit(1 if falhas else 0)
