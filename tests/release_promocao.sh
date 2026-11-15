#!/bin/bash
# Guarda do workflow de Release: manual, sem compilar, com confirmacao, sem sobrescrever tag,
# e o verificador "o que o atualizador vera" aceita o formato certo e recusa os errados.
set -eu
cd "$(dirname "$0")/.."
python3 -c "import yaml" 2>/dev/null || pip install -q --break-system-packages pyyaml
python3 - <<'PY'
import re, json, subprocess, yaml
p = ".github/workflows/release-android-world.yml"
t = open(p).read(); d = yaml.safe_load(t)
on = d[True]
assert list(on.keys()) == ["workflow_dispatch"], "so manual: sem push/schedule"
inp = on["workflow_dispatch"]["inputs"]
assert inp["publicar"]["default"] is False, "padrao e so verificar"
for k in ("versao", "run_id", "publicar", "confirmacao"): assert k in inp
steps = d["jobs"]["release"]["steps"]; names = " | ".join(s.get("name", "") for s in steps)
assert "softprops" not in t, "nada de action que sobrescreve release"
assert not re.search(r"tools/android\.sh|gradle|cargo|ndk", t, re.I), "este workflow nao compila"
assert "PUBLICAR v$VERSAO" in t and "git ls-remote --tags" in t and "gh release view" in t
pub = [s for s in steps if s.get("name", "").startswith("Publish the GitHub Release")][0]
assert "inputs.publicar" in pub["if"] and "--target" in pub["run"] and "--latest" in pub["run"]
assert "gh release create" in t and t.count("gh release create") == 1 and "--clobber" not in t
assert "Nuvio-$VERSAO-android-lite-armv7.apk" in t
assert "certificado DIFERENTE" in t and "versionCode" in t and "collections-regression.yml" in t
# o verificador pos-publicacao (mesmo codigo do workflow) contra respostas simuladas
code = re.search(r"python3 -c '\n(.*?)\n\s*' \"\$VERSAO\"", t, re.S).group(1)
import textwrap; code = textwrap.dedent(code)
def roda(r, v="2.0.26", sha="ab"*32):
    return subprocess.run(["python3", "-c", code, v, sha], input=json.dumps(r), text=True, capture_output=True).returncode
ok = {"tag_name": "v2.0.26", "draft": False, "prerelease": False,
      "assets": [{"name": "Nuvio-2.0.26-android-lite-armv7.apk", "digest": "sha256:" + "ab"*32}]}
assert roda(ok) == 0
import copy
def var(f):
    r = copy.deepcopy(ok); f(r); return roda(r)
assert var(lambda r: r.update(tag_name="v2.0.25")) != 0
assert var(lambda r: r.update(prerelease=True)) != 0
assert var(lambda r: r["assets"][0].update(name="Nuvio-2.0.26-android-lite-armv7-debug.apk")) != 0
assert var(lambda r: r["assets"][0].pop("digest")) != 0
assert var(lambda r: r["assets"][0].update(digest="sha256:" + "cd"*32)) != 0
# notas: formato que o cartao do app mostra (limparNotas)
n = open("deploy/release-notes/2.0.26.md").read()
assert not re.search(r"^---", n, re.M) and not re.search(r"^#+ *(Notes|Notas)", n, re.M | re.I)
assert n.count("\n- ") == 4 and len(n) < 3000
PY
echo "release_promocao.sh: ok"
