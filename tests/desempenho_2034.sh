#!/usr/bin/env bash
# Guardas da 2.0.34 (desempenho e carregamento, medidos no log da TCL de 932 MB).
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'P'
t=open("src/tex_cache.c").read(); v=open("src/vertudo.c").read(); c=open("src/colecoes.c").read()
g=open("src/gfx.c").read(); h=open("src/home.c").read(); m=open("src/main.c").read(); a=open("src/androidcache.c").read()
def ok(x,n):
    if not x: raise SystemExit("FAIL: "+n)
ok("itens[i].sup && quente(&itens[i])) continue;" in t, "pressao dura nao mata decode de arte na tela")
ok("!itens[i].filaRedeEm && !quente(&itens[i])" in t, "pressao dura nao cancela pedido de arte na tela")
ok("#define NV_CACHE_DISCO_MAX (192L * 1024L * 1024L)" in t and "NV_CACHE_DISCO_FRACAO 15" in t, "cache em disco proporcional no Android")
ok("colcache_gravar(p->chave" in v and "vitDoColcache(r)" in v and "COLCACHE_FRESCO_S" in v, "fileiras das Collections do colcache")
ok("vitMesmoConteudo(r, p)" in v, "revalidacao sem mudanca nao troca a fileira")
ok('"colrow-*.bin"' in a, "colcache na pasta de cache do Android")
ok("home_vitrine_arte_troca(camIt[0], al[0], camIt[1], al[1])" in v and "gfx_hero_camadas((GfxRect){ 0, 0, NV_TELA_W" in h, "troca de destaque numa passada")
ok("(camN > 0 && camP[0] < 1.0f)" in v, "sem tela cheia escura extra na troca")
ok("void gfx_desfoque_aquecer(void)" in g and "gfx_desfoque_aquecer();" in m, "desfoque aquecido na abertura")
ok("!it && !vitChave(vitHeroAtual)" in v, "destaque fica no Ver tudo/fileira vazia")
ok("account snapshot identical: kept" in c and "snapshot aplicado em" in c, "snapshot igual nao reaplica; tempo no log")
# 2.0.35: instrumentacao de diagnostico presente e SEM mudar comportamento.
ok("[diag] pressao origem=%s" in t and 'pressaoOrigem = "aliviar(MemAvailable)"' in t, "diag: origem do aviso de pressao")
ok("int tex_diag_pressao(" in t and "tex_diag_pressao(dl, sizeof dl, memlog_livre_mb())" in m, "diag: estado do teto de texturas no relatorio de 3 s")
ok("[diag] androidcache:" in a and "nv_android_cache_diag(dl, sizeof dl)" in m, "diag: podas do androidcache no relatorio de 3 s")
ok("[diag-hero] ruim" in v and "home_vitrine_diag_arte()" in v and "static int homeDiagArte;" in h, "diag: bloco vermelho do destaque")
ok("#define NV_ANDROID_CACHE_BYTES (80ULL << 20)" in open("src/androidcache.h").read(), "diag nao mexe no teto de 80 MB (so mede)")
ok("if (camN >= 2) {\n      trocou = home_vitrine_arte_troca(camIt[0], al[0], camIt[1], al[1]);" in v, "diag: ordem das chamadas de arte preservada")
# 2.0.36: correcoes medidas no log da TV (guardas de codigo-fonte; o teste real e na TV).
ml=open("src/memlog.c").read(); co=open("src/colecoes.c").read()
ok("VtChave heroChave, antChave;" in v and "novoHero" in v and "vitHeroDes = -1;" in v, "destaque sobrevive a reprojecao pela chave do titulo")
ok("if (hero || vitItem(vitFR, vitFC)) {" in v and "} else { fundoUsado = 1; fundoColecao(a); }" in v, "sem arte e com titulo: fundo escuro, nunca o banner da Collection")
ok("memlog_piso_para_ram(lerKB(\"/proc/meminfo\", \"MemTotal:\") / 1024)" in ml and "folgaAviso()" in ml, "limiar de memoria baixa proporcional a RAM (Android)")
ok("  if (sim == arteConta) return;\n  atalhoValido = 0;" in co and "snapshotsIgnorados++;" in co, "col_arte_conta nao invalida o atalho do snapshot identico")
ok("vitgeom_prebusca(vitFR, fc, eixo, dh, dv" in v and "tex_lookahead_pedir(cam, larguraDe(FILEIRA_NORMAL))" in h, "pre-carga direcional da Vitrine")
# 2.0.37: texto sobreposto ao sair, fechamento animado do pôster expandido, prioridade da fila.
ok("if (!aberta) {\n    float f = fmaxf(0.0f, fminf(1.0f, (anim - 0.55f) / 0.45f));" in v, "saida da Collection apaga o conteudo na 1a metade")
ok("static void vitExpFechar(void)" in v and "vitExpAntF = anim_mola(vitExpAntF, 0.0f, dt, NV_MOLA_TELA);" in v and "vitExpExtra(r, c, w, h, &propria)" in v, "pôster expandido fecha animado")
ok("static int prioFila(const Item *it) { return it->urgente * 2 + (it->lookahead ? 0 : 1); }" in t and "int pr = prioFila(&itens[f[p]]);" in t, "fila de rede: pedido visivel antes do especulativo")
# 2.0.38: troca do destaque continua quando interrompida (pilha de camadas), 350 ms.
ok("#define VIT_HERO_DUR 0.35f" in v and "vitHeroEntra += dt / VIT_HERO_DUR;" in v, "troca do destaque em 350 ms")
ok("vitHeroAnt2 = vitHeroAnt; vitHeroEntraAnt2 = vitHeroEntraAnt;" in v and "vitHeroAnt = vitHeroAtual; vitHeroEntraAnt = vitHeroEntra;" in v, "troca interrompida empilha a camada com o progresso que tinha")
ok("home_vitrine_arte_troca(camIt[0], al[0], camIt[1], al[1])" in v and "float ty = topo + 18.0f * (1.0f - ent) - 14.0f * sai;" in v, "arte e texto do destaque por camadas")
# 2.0.39: a reprojecao nao zera mais a posicao horizontal das fileiras na TV.
ok("if (!manter) vitSx[r] = vitSv[r] = 0.0f;\n      vitSxAjuste[r] = 0;\n    } else if (!l->n) {" in v, "reprojecao: so o telefone zera vitSx")
ok("porChave ? mesmaChave(&vitLembChave[r], it) : bruto == lembrado" in v and "static VtChave vitLembChave[VIT_LIN_MAX];" in v, "coluna lembrada reencontrada pela chave estavel (IMDb/TMDB)")
ok("vitSx[r] += alvoX - vitgeom_fixo_alvo(vitSxColAnt[r], vitSxTotAnt[r], cab, passo);" in v and "if (vitSxAjuste[r] == 2) { vitSx[r] = alvoX; vitSv[r] = 0.0f; }" in v, "deslocamento ajustado pela diferenca de alvo, sem deslizar")
ok("vitLembrarLinha(r, vitColLemb[r]);   // vazia: mantem a chave anterior" in v and "if (r < 0 || r >= vitN || !vitLin[r].n) return;" in v, "fileira descarregada guarda a chave para voltar a coluna")
ok("memo[i].chave; vitLembTem[r] = memo[i].tem;" in v, "reagrupamento (fonte unica) segue o nome do grupo")
print("desempenho_2034: tudo ok")
P
