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
ok("home_vitrine_cartao_aberto(vitItem(r, c), card, abre, acc)" in v, "Vitrine pinta o cartaz aberto")
ok("ajustes_col_expandir_atraso() * 1000.0f" in v, "atraso respeitado")
# 2.0.32: Fluxo de capas aposentado, fade da borda direita, "Ver tudo" sem bloco branco
ok("VT_LAY_FLUXO, VT_LAY_RANKING" not in v, "Fluxo de capas fora do seletor")
ok("vals[n] != VT_LAY_FLUXO" in v, "escolha gravada do Fluxo volta a Padrao")
ok('"Vitrine", "Fluxo de capas"' not in aj and "V_COLECAO_EXIBICAO, VT_LAY_FLUXO)" in aj, "Fluxo fora dos Ajustes")
ok("gfx_esvanecer_x(0.0f, esp, NV_TELA_W - esp, NV_TELA_W)" in v and "gfx_sem_esvanecer_x();" in v, "pontas esvanecem por pixel, iguais")
ok("vitgeom_fixo_alvo(col, vitColunas(r), cab, passo)" in v, "foco fixo na ancora (Netflix)")
ok("vitFocoF" in v and "foco_zoom(base, ff)" in v and "foco_anel(base, ajustes_raio_poster_px() / (w < h ? w : h), ff, acc)" in v, "foco entra e sai animado (2.0.45: o anel segue a opacidade do cartaz)")
ok("float ty = topo - 14.0f * sai" in v and "18.0f * (1.0f - entEl[0])" in v, "texto do destaque troca com transicao")
ok("vitTrocaPend = folder;" in v and "vitTrocaSai += dt / 0.25f;" in v, "troca de Collection: sai antes de entrar")
ok("vitHeroEntra = 0.0f;   // o primeiro destaque" in v and "vitLinEntra[r]" in v, "destaque e fileiras entram animados")
ok("ajustes_navegacao_velocidade_cartazes()" in v and "0.85f" not in open("src/ajustes.c").read().split("float ajustes_navegacao_velocidade(void)")[1].split("}")[0], "deslize suave so nas fileiras")
g=open("src/gfx.c").read()
ok("#define main nvCorpo" in g and "uFadeX" in g and "!fadeOn" in g, "esvanecer horizontal no shader comum")
ok("sel && ajustes_borda_foco() && !ajustes_vidro()" in v and "0.115f, 0.12f, 0.14f, acc" in v, "Ver tudo com fundo opaco sem vidro")
print("colecao_fase1: tudo ok")
P
