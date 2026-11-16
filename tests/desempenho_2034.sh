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
ok("home_vitrine_arte_troca(ant, a, hero, a * e)" in v and "gfx_hero_camadas((GfxRect){ 0, 0, NV_TELA_W" in h, "troca de destaque numa passada")
ok("(vitHeroEntra < 1.0f && !ant)" in v, "sem tela cheia escura extra na troca")
ok("void gfx_desfoque_aquecer(void)" in g and "gfx_desfoque_aquecer();" in m, "desfoque aquecido na abertura")
ok("!it && !vitChave(vitHeroAtual)" in v, "destaque fica no Ver tudo/fileira vazia")
ok("account snapshot identical: kept" in c and "snapshot aplicado em" in c, "snapshot igual nao reaplica; tempo no log")
# 2.0.35: instrumentacao de diagnostico presente e SEM mudar comportamento.
ok("[diag] pressao origem=%s" in t and 'pressaoOrigem = "aliviar(MemAvailable)"' in t, "diag: origem do aviso de pressao")
ok("int tex_diag_pressao(" in t and "tex_diag_pressao(dl, sizeof dl, memlog_livre_mb())" in m, "diag: estado do teto de texturas no relatorio de 3 s")
ok("[diag] androidcache:" in a and "nv_android_cache_diag(dl, sizeof dl)" in m, "diag: podas do androidcache no relatorio de 3 s")
ok("[diag-hero] ruim" in v and "home_vitrine_diag_arte()" in v and "static int homeDiagArte;" in h, "diag: bloco vermelho do destaque")
ok("#define NV_ANDROID_CACHE_BYTES (80ULL << 20)" in open("src/androidcache.h").read(), "diag nao mexe no teto de 80 MB (so mede)")
ok("if (ant && vitHeroEntra < 1.0f) {\n      trocou = home_vitrine_arte_troca(ant, a, hero, a * e);" in v, "diag: ordem das chamadas de arte preservada")
print("desempenho_2034: tudo ok")
P
