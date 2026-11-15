#!/bin/bash
# Fase 1 das Collections (dono, 15/11): guardas de fonte para o que nao tem
# teste de tela no Linux.
#  - Collection na TV: sem relogio e sem o botao "Exibição"; a exibicao mora na
#    primeira aba do painel do CH+ (salvospainel.c, SP_ABA_COLECAO).
#  - Itens por fileira 12..42 (ajuste), memoria por fileira com janela de carga.
#  - Expansao do cartaz vertical nas Collections, com atraso.
set -eu
cd "$(dirname "$0")/.."
python3 - <<'P'
import re, sys
v = open("src/vertudo.c").read()
s = open("src/salvospainel.c").read()
a = open("src/app.c").read()
aj = open("src/ajustes.c").read()
d = open("src/descoberta.c").read()
pad = open("src/ajustes_ux_padrao.inc").read()
def ok(c, m):
    if not c: print("FAIL:", m); sys.exit(1)
# 1a
ok("static void botaoExibicao(float a, float cy) {\n  if (chipOculto()) return;" in v, "botao Exibicao oculto na Collection da TV")
ok("SP_ABA_COLECAO = 5, SP_ABA_N = 6" in s, "aba Collection acrescentada no fim do enum (ids persistidos)")
ok("abaOrdem[0] = SP_ABA_COLECAO" in s and "colecaoPrimeira();" in s, "aba Collection sempre primeira")
ok("aba = abaExiste(SP_ABA_COLECAO) ? SP_ABA_COLECAO : SP_ABA_SALVOS;" in s, "CH+ abre na aba Collection")
ok("&& !vertudo_em_colecao());" in a, "relogio oculto na Collection")
ok("else if (vertudo_em_colecao()) spainel_abrir();" in a, "CH+ na Collection abre o painel")
# 1b
ok('NUM("Quantidade de itens por fileira", 12, 42, 1,' in aj, "ajuste 12..42")
ok("12, /* Collections: items per Vitrine row" in pad, "padrao 12")
ok("#define VIT_ITENS   42" in v, "teto 42")
ok("vitCap = ajustes_col_itens_fileira();" in v, "Vitrine usa o ajuste")
ok("limite > 42" in d, "carga inicial aceita ate 42")
ok("if (r < cargaIni || r > cargaFim) continue;" in v, "janela de carga")
ok("vitLinSoltar(r);" in v and "VIT_SOLTA_FRENTE" in v, "fileiras distantes devolvem memoria")
ok("vitBuf[" not in v, "sem o buffer unico antigo")
# 1c
ok('ESC("Expandir posters verticais das Collections ao focar", V_LIGA, 2)' in aj, "ajuste de expansao")
ok("home_vitrine_cartao_aberto(vitItem(r, c), card, abre, ac)" in v, "Vitrine pinta o cartaz aberto")
ok("ajustes_col_expandir_atraso() * 1000.0f" in v, "atraso respeitado")
print("colecao_fase1: tudo ok")
P
