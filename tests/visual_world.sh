#!/bin/bash
# Guardas da etapa VISUAL WORLD (18/11): tom Preto puro, tipografia 10-foot,
# identidade Nuvio World (logos, launcher, banner, nome), video de abertura e a
# variante de teste sem auto-atualizacao. Sem GL e sem rede.
#
#   bash tests/visual_world.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-visualworld.XXXXXX")
trap 'rm -rf "$work"' EXIT
falha() { echo "FALHOU: $*" >&2; exit 1; }

# 1. A conta do Preto puro, extraida do gfx.c (a mesma funcao do app).
python3 - "$work/tema.h" <<'PY'
from pathlib import Path
import sys
s = Path('src/gfx.c').read_text()
a = s.index('static float temaV(float v) {')
b = s.index('\n}\n', s.index('void gfx_tema_cor(')) + 3
Path(sys.argv[1]).write_text(s[a:b])
PY
cc -Wall -Wextra -I"$work" -x c -o "$work/tema" - -lm <<'C'
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tema.h"
static void cor(float r, float g, float b, float *o) { o[0] = r; o[1] = g; o[2] = b; gfx_tema_cor(&o[0], &o[1], &o[2]); }
int main(void) {
  float o[3], p[3], q[3];
  cor(0.051f, 0.051f, 0.051f, o); assert(o[0] == 0 && o[1] == 0 && o[2] == 0);   // fundo do Grafite -> preto
  cor(0.0f, 0.0f, 0.0f, o);       assert(o[0] == 0);                               // preto continua preto
  cor(0.20f, 0.20f, 0.20f, o);    assert(fabsf(o[0] - 0.20f) < 1e-6f);             // teto nao muda (continuidade)
  cor(0.50f, 0.50f, 0.50f, o);    assert(fabsf(o[0] - 0.50f) < 1e-6f);             // cinza claro: nao e superficie
  cor(0.80f, 0.10f, 0.10f, o);    assert(fabsf(o[0] - 0.80f) < 1e-6f && fabsf(o[1] - 0.10f) < 1e-6f); // cor de verdade
  cor(13/255.f, 16/255.f, 30/255.f, o); assert(fabsf(o[2] - 30/255.f) < 1e-6f);    // matiz azul do ladrilho: fica
  // A ORDEM entre superficies se mantem (painel acima do fundo, esqueleto acima do painel).
  cor(0.10f, 0.10f, 0.10f, o); cor(0.14f, 0.14f, 0.14f, p); cor(0.173f, 0.173f, 0.173f, q);
  assert(o[0] > 0.0f && o[0] < p[0] && p[0] < q[0]);
  // O esqueleto (NV_COR_ESQUELETO 0,173) continua visivel sobre o preto.
  assert(q[0] > 0.12f);
  puts("ok tema: fundo -> #000, superficies em ordem, cores e claros intactos");
  return 0;
}
C
"$work/tema"

# 2. Tipografia 10-foot: os corpos pequenos subiram e as entrelinhas acompanham.
cc -Isrc -x c -o "$work/tip" - <<'C'
#include "layout.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  assert(NV_FT_CAPTION >= 23 && NV_FT_CAPTION2 >= 23);
  assert(NV_FT_HERO_META >= 24 && NV_FT_HERO_SIN >= 26 && NV_FT_HERO_SEC >= 20);
  assert(NV_LD_HERO_SIN >= NV_FT_HERO_SIN * 13 / 10);
  assert(NV_LD_HERO_META >= NV_FT_HERO_META * 12 / 10);
  assert(NV_LD_HERO_SEC >= NV_FT_HERO_SEC * 13 / 10);
  puts("ok tipografia: legendas >= 23 px, destaque 25/26 com entrelinhas proporcionais");
  return 0;
}
C
"$work/tip"

# 3. Identidade: arquivos do World, launcher e banner nos tamanhos do Android, o
#    fundo verde preservado, o nome do app e o logo padrao.
python3 - <<'PY'
from PIL import Image
import re
A = 'deploy/app/art/marcas/'
for f, asp in (('logo-world-simbolo.png', 1.12), ('logo-world-marca.png', 0.91), ('logo-world-horizontal.png', 3.49)):
    im = Image.open(A + f)
    assert im.mode == 'RGBA', f
    assert im.getpixel((0, 0))[3] == 0, f + ': fundo transparente'
    a = im.width / im.height
    assert abs(a - asp) < 0.05, (f, a)
R = 'android/app/src/main/res/'
ic = Image.open(R + 'mipmap-nodpi/ic_launcher.png').convert('RGB')
bn = Image.open(R + 'drawable-nodpi/banner.png').convert('RGB')
assert ic.size == (192, 192), ic.size
assert bn.size == (320, 180), bn.size
for im in (ic, bn):
    r, g, b = im.getpixel((2, 2))
    assert abs(r - 186) < 8 and abs(g - 232) < 8 and abs(b - 44) < 8, ('verde', r, g, b)
s = open(R + 'values/strings.xml').read()
assert '<string name="app_name">Nuvio World</string>' in s
l = open('src/logoapp.c').read()
assert '"logo-world-simbolo.png", "logo-world-marca.png", "logo-world-horizontal.png"' in l
assert 'LOGO_NOVO ? "World"' in l
print('ok identidade: logos World, ic_launcher 192, banner 320x180 verdes, app_name Nuvio World')
PY

# 4. Video de abertura: no pacote, pequeno, sem audio e com os quadros-chave
#    dos saltos do SplashVideo.kt (7,875 s e 8,0 s).
v=android/app/src/main/res/raw/abertura_world.mp4
[ -f "$v" ] || falha "sem $v"
[ "$(wc -c < "$v")" -lt 1500000 ] || falha "video de abertura grande demais"
if command -v ffprobe >/dev/null 2>&1; then
  [ -z "$(ffprobe -v error -select_streams a -show_entries stream=index -of csv=p=0 "$v")" ] || falha "video com audio"
  info=$(ffprobe -v error -select_streams v -show_entries stream=codec_name,width,height,pix_fmt -of csv=p=0 "$v")
  [ "$info" = "h264,1920,1080,yuv420p" ] || falha "formato do video: $info"
  chaves=$(ffprobe -v error -select_streams v -skip_frame nokey -show_entries frame=pts_time -of csv=p=0 "$v" | tr '\n' ' ')
  case "$chaves" in *7.875000*8.000000*) ;; *) falha "quadros-chave do salto ausentes: $chaves";; esac
  echo "ok video: h264 1080p sem audio, quadros-chave 7,875/8,0"
else
  echo "aviso: sem ffprobe; formato do video nao conferido"
fi
grep -q '"aberturaVideoLocal"' src/ajustes.c || falha "chave aberturaVideoLocal fora do ajustes.c"
grep -q 'f\[0\] == "aberturaVideoLocal"' android/app/src/main/java/space/nuvio/nativelegacy/SplashVideo.kt || falha "SplashVideo nao le aberturaVideoLocal"
grep -q 'android_abertura_pronta();' src/abertura.c || falha "abertura nao avisa o SplashVideo"
grep -q 'R.raw.abertura_world' android/app/src/main/java/space/nuvio/nativelegacy/SplashVideo.kt || falha "SplashVideo sem o video"

# 5. Variante de teste: pacote proprio e nenhuma auto-atualizacao.
grep -q '"space.nuvio.world.test"' android/app/build.gradle.kts || falha "sem applicationId de teste"
grep -q 'android:label="${appLabel}"' android/app/src/main/AndroidManifest.xml || falha "rotulo do app fixo"
grep -q 'if (atualizacaoDesligada()) return;' src/atualizacao.c || falha "atualizacao sem a trava da variante de teste"
grep -q 'packageName.endsWith(".test")) return 0' android/app/src/main/java/space/nuvio/nativelegacy/NuvioActivity.kt || falha "instalarApk sem a trava"
echo "ok variante de teste: pacote .test, rotulo proprio, sem auto-atualizacao"
# 6. Redesign World (18/11): menu-coluna, veu adaptativo, pre-carga direcional.
grep -q 'static int world(void) { return !padrao() && !topo(); }' src/menu.c || falha "menu World ausente"
grep -q 'gfx_veu_css_base((GfxRect){ 0, 0, w, NV_TELA_H }, 2' src/menu.c || falha "veu da borda do menu ausente"
[ "$(grep -c 'nv_veu_luz(' src/gfx.c)" -ge 5 ] || falha "veu adaptativo do destaque incompleto (simples e camadas)"
grep -q 'for (int k = 1; k <= janela + alcance && !pediuH; k++)' src/home.c || falha "pre-carga horizontal alem da janela ausente"
grep -q 'cheios = curta ? 1 : 2, alcance = curta ? 8 : 6' src/home.c || falha "pre-carga horizontal sem a adaptacao ao orcamento"
grep -q 'passo <= 2 && !pediu' src/home.c || falha "pre-carga vertical deveria ser de 2 fileiras"
echo "ok redesign: menu-coluna, veu adaptativo, pre-carga direcional"

# 7. Carregamento progressivo (19/11): previa a 128 px do mesmo arquivo em
# todas as telas de cartaz; a prova de comportamento e tests/previa_progressiva.sh.
grep -q '#define NV_TEX_PREVIA_LARG 128' src/tex_cache.c || falha "teto da previa mudou"
grep -q 'itens\[alvo\].previa = ' src/tex_cache.c || falha "marca da previa fora do upload (pisca na promocao)"
for t in src/home.c src/vertudo.c src/explorar.c src/biblioteca.c src/busca.c; do
  grep -q 'tex_previa_pedir(' "$t" || falha "$t sem previas"
done
echo "ok carregamento progressivo: Home, Collections, Explorar, Biblioteca, Busca"

# 8. Etapa 2 (19/11): Ajustes, menus, Explorar, entrada da Collection.
grep -q 'static float aj2PilulaEstado' src/ajustes_ux_desenho.inc && grep -q '#define AJ_SW_W 66.0f' src/ajustes_ux_desenho.inc || falha "interruptor dos Ajustes ausente"
grep -q 'static int focoWorld(void) { return !topo(); }' src/menu.c || falha "foco World fora da Padrao"
grep -q 'int escuro = emFoco && focoWorld();' src/menu.c || falha "rodape do menu volta a branco sobre branco"
grep -q 'iconeTraco(d, cx, cy, s, r, g, b, a);' src/menu.c || falha "icones cheios voltaram ao menu"
grep -q 'static int porqueCartao' src/explorar.c || falha "Explorar repete o motivo do grupo"
grep -q 'int vertudo_voo_titulo_ativo(void)' src/vertudo.c && grep -q 'if (!vertudo_voo_titulo_ativo())' src/home.c || falha "titulo da pasta duplo no voo"
grep -q 'home_pasta_nome(folder, &lg)' src/vertudo.c || falha "nome da pasta nao viaja ao cabecalho"
echo "ok etapa 2: interruptor, foco World nas laterais, icones de traco, Explorar, voo do titulo"
echo "tudo ok"
