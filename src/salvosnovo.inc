// ESTANTE (S1) E COLUNA (S2): as duas apresentacoes novas do CH+ (World,
// 24/11, aprovadas na prancha animada R8). Incluido por salvospainel.c depois
// dos desenhos de celula; usa as mesmas pecas (superficieItem, capaArte,
// barraSobreArte, nomeVerbo) para a lista continuar sendo a mesma lista.

// "T2 E4 · faltam 38 min" quando ha progresso; senao "Série · 2016".
static void apoioDe(const SPLinha *l, char *apoio, size_t tam) {
  if (l->progresso > 0 && l->txtRestante[0]) snprintf(apoio, tam, "%s", l->txtRestante);
  else if (l->ano > 0) snprintf(apoio, tam, "%s · %d", i18n(tipoRotulo(l)), l->ano);
  else snprintf(apoio, tam, "%s", i18n(tipoRotulo(l)));
}
static const CatItem *itemDe(const SPLinha *l) {
  int k = l && l->id[0] ? cat_indice_por_imdb(l->id) : -1;
  return k >= 0 ? cat_item(k) : NULL;
}
static const char *arteDeitada(const SPLinha *l) {
  const CatItem *ci = itemDe(l);
  if (ci && ci->backdrop[0] && !tex_falhou(ci->backdrop)) return ci->backdrop;
  return l->fundo[0] ? l->fundo : l->poster;
}

// A ENTRADA EM CASCATA: cada cartao (ou linha) entra 60 ms depois do vizinho,
// subindo 24 px. `ordem` e a posicao visivel (0 = o primeiro da tela).
static float cascata(float ordem, float *dy) {
  float k;
  if (ajustes_animacoes_reduzidas()) { if (dy) *dy = 0.0f; return 1.0f; }
  k = (entrada - 0.38f - 0.06f * ordem) / 0.34f;
  k = k < 0.0f ? 0.0f : k > 1.0f ? 1.0f : k;
  k = 1.0f - (1.0f - k) * (1.0f - k) * (1.0f - k);
  if (dy) *dy = 24.0f * (1.0f - k);
  return k;
}

// O CARTAO DA ESTANTE: arte 16:9 de 352, a barra de progresso por cima, o
// nome e o apoio embaixo. Foco: a superficie da linha (a mesma das outras
// abas) e a arte 4 % maior, em volta do proprio centro.
static void desenhaCartaoEstante(int i, float dx, float y, float a) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i], v = focoVisual(f), xo = spRolDe(l->fila), dy, k;
  float x = SP_X + dx + SP_PAD + l->lx - xo;
  char apoio[192];
  GfxRect img;
  k = cascata((l->lx - xo) / (SPE_W + SPE_VAO) + (float)l->fila * 0.5f, &dy);
  a *= k; y += dy;
  if (a < 0.01f) return;
  if (f > 0.01f) {
    GfxRect r = { x - 12.0f, y - 12.0f, l->lw + 24.0f, l->lh + 16.0f };
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a);
  }
  { float s = 1.0f + 0.04f * v, w = SPE_W * s, h = SPE_ART_H * s;
    img = (GfxRect){ x + (SPE_W - w) * 0.5f, y + (SPE_ART_H - h) * 0.5f, w, h }; }
  capaArte(img, arteDeitada(l), 10.0f, a);
  barraSobreArte(img, l->progresso, a);
  apoioDe(l, apoio, sizeof apoio);
  nomeVerbo(l->titulo, NULL, x, y + SPE_ART_H + 16.0f, l->lw / 0.6f, v, a);
  txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, apoio, l->lw), x, y + SPE_ART_H + 49.0f, a * 0.62f);
}

// A LINHA DA COLUNA. Fechada: miniatura 16:9 de 176 a esquerda, nome e apoio.
// Em foco ela SE ABRE (colAbre): a miniatura cresce ate a largura da coluna,
// o nome (ou o logo) vai para o canto de baixo da arte, e embaixo ficam o
// apoio e o que o OK faz. As linhas de baixo descem junto (desenharPainel).
static void desenhaLinhaColuna(int i, float dx, float y, float a, float ordem) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i], v = focoVisual(f), e = colAbre[i], dy, k;
  float px = SP_X + dx + SP_PAD, h = SPC_H + SPC_EXTRA * e;
  float ee = e * e * (3.0f - 2.0f * e);
  char apoio[192];
  GfxRect t0 = { px, y + 12.0f, SPC_TW, SPC_TH }, t1 = { px, y + 14.0f, SP_INTERNO, SPC_ART_H }, r;
  k = cascata(ordem, &dy);
  a *= k; y += dy; t0.y += dy; t1.y += dy;
  if (a < 0.01f) return;
  if (f > 0.01f) {
    GfxRect row = { SP_X + dx + SP_LINHA_X, y, SP_LINHA_W, h };
    superficieItem(row, SP_LINHA_RAIO / row.h, f, a);
  }
  r.x = t0.x + (t1.x - t0.x) * ee; r.y = t0.y + (t1.y - t0.y) * ee;
  r.w = t0.w + (t1.w - t0.w) * ee; r.h = t0.h + (t1.h - t0.h) * ee;
  capaArte(r, arteDeitada(l), 8.0f + 4.0f * ee, a);
  barraSobreArte(r, l->progresso, a * (1.0f - ee));
  apoioDe(l, apoio, sizeof apoio);
  { float ta = 1.0f - anim_clamp(e / 0.35f, 0.0f, 1.0f);
    float tx = px + SPC_TW + 22.0f, tw = SP_INTERNO - SPC_TW - 22.0f;
    if (ta > 0.01f) {
      nomeVerbo(l->titulo, NULL, tx, y + 24.0f, tw / 0.6f, v, a * ta);
      txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, apoio, tw), tx, y + 60.0f, a * 0.62f * ta);
    } }
  if (e > 0.45f) {
    float ka = anim_clamp((e - 0.45f) / 0.55f, 0.0f, 1.0f);
    float base = r.y + r.h;
    gfx_veu_base(r, 12.0f / r.h, 0.55f, 0.78f * a * ka);
    logotitulo_desenhar(itemDe(l), l->titulo, TXT_ILHA_NOME_L, r.x + 24.0f, base - 24.0f - 70.0f,
                        r.w * 0.6f, 70.0f, r.w - 48.0f, a * ka);
    if (l->progresso > 0) {
      GfxRect tr = { r.x + 24.0f, base - 14.0f, r.w - 48.0f, 4.0f }, pr = tr;
      float cr, cg, cb;
      ajustes_acento(&cr, &cg, &cb);
      pr.w = tr.w * anim_clamp(l->progresso / 100.0f, 0.0f, 1.0f);
      gfx_cor(tr, 0.5f, 1.0f, 1.0f, 1.0f, 0.22f * a * ka);
      if (pr.w > 4.0f) gfx_cor(pr, 0.5f, cr, cg, cb, a * ka);
    }
    txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, apoio, SP_INTERNO), px, base + 16.0f, a * 0.78f * ka);
    { char dica[200];
      snprintf(dica, sizeof dica, "%s  \xc2\xb7  %s", i18n("OK para abrir"), i18n("Segure OK para mais opções"));
      txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, dica, SP_INTERNO), px, base + 50.0f, a * 0.45f * ka); }
  }
}

// O BANNER DA ESTANTE: a arte do titulo em foco no alto da tela, o nome (ou o
// logo) no canto de baixo a esquerda, o apoio e o progresso embaixo dele. A
// troca de foco cruza as duas artes em 320 ms, a nova assentando de 104 %.
// Fora da aba Salvos ele fica, apagado a 45 % e sem o nome: a folha diz em
// que aba a pessoa esta.
static char banUrl[2][NV_ARTE_URL_MAX], banNome[2][160], banId[2][24], banApoio[2][192];
static int banProg[2];
static float banT = 1.0f, banNomeA, banLuz = 1.0f;
static void bannerAtualizar(float dt) {
  int i = -1;
  if (spModo() != SPM_ESTANTE || (!aberto && entrada < 0.002f)) return;
  if (aba == SP_ABA_SALVOS && foco >= 0 && foco < nLinhas) i = foco;
  else if (!banId[0][0] && nLinhas > 0) i = 0;
  if (i >= 0 && strcmp(linhas[i].id, banId[0])) {
    const SPLinha *l = &linhas[i];
    int tinha = banId[0][0] != 0;
    memcpy(banUrl[1], banUrl[0], sizeof banUrl[0]); memcpy(banNome[1], banNome[0], sizeof banNome[0]);
    memcpy(banId[1], banId[0], sizeof banId[0]); memcpy(banApoio[1], banApoio[0], sizeof banApoio[0]);
    banProg[1] = banProg[0];
    snprintf(banUrl[0], sizeof banUrl[0], "%s", arteDeitada(l));
    snprintf(banNome[0], sizeof banNome[0], "%s", l->titulo);
    snprintf(banId[0], sizeof banId[0], "%s", l->id);
    apoioDe(l, banApoio[0], sizeof banApoio[0]);
    banProg[0] = l->progresso;
    banT = tinha && !ajustes_animacoes_reduzidas() ? 0.0f : 1.0f;
  }
  banT = anim_rampa(banT, 1.0f, dt, 320.0f);
  { float alvo = aba == SP_ABA_SALVOS && !editando && nLinhas > 0 ? 1.0f : 0.0f;
    float luz = aba == SP_ABA_SALVOS && !editando ? 1.0f : 0.45f;
    banNomeA = ajustes_animacoes_reduzidas() ? alvo : anim_rampa(banNomeA, alvo, dt, 220.0f);
    banLuz = ajustes_animacoes_reduzidas() ? luz : anim_rampa(banLuz, luz, dt, 220.0f); }
}
static void bannerZerar(void) {
  memset(banId, 0, sizeof banId); memset(banUrl, 0, sizeof banUrl);
  banT = 1.0f; banNomeA = 0.0f; banLuz = 1.0f;
}
static void bannerNome(int k, float a, float dxT) {
  const SPLinha *l = NULL;
  int i;
  for (i = 0; i < nLinhas; i++) if (!strcmp(linhas[i].id, banId[k])) { l = &linhas[i]; break; }
  if (a < 0.01f || !banNome[k][0]) return;
  logotitulo_desenhar(l ? itemDe(l) : NULL, banNome[k], TXT_NOV_TITULO, 96.0f + dxT, 178.0f, 620.0f, 120.0f,
                      1100.0f, a);
  txt_desenhar_alpha(txtIlha(TXT_ILHA_NOME, banApoio[k], 1100.0f), 96.0f + dxT, 316.0f, a * 0.72f);
  if (banProg[k] > 0) {
    GfxRect tr = { 96.0f + dxT, 362.0f, 360.0f, 5.0f }, pr = tr;
    float cr, cg, cb;
    ajustes_acento(&cr, &cg, &cb);
    pr.w = tr.w * anim_clamp(banProg[k] / 100.0f, 0.0f, 1.0f);
    gfx_cor(tr, 0.5f, 1.0f, 1.0f, 1.0f, 0.2f * a);
    if (pr.w > 5.0f) gfx_cor(pr, 0.5f, cr, cg, cb, a);
  }
}
// O banner inteiro dentro de `clip` (o retangulo que cresce da origem na
// abertura) e a folha de baixo. `e` e a abertura (0..1), `a` o alfa.
static void estanteMoldura(GfxRect clip, float e, float a) {
  GfxRect R = { 0.0f, 0.0f, NV_TELA_W, SPE_Y + 150.0f };
  float sheetY = SPE_Y + (1.0f - e) * 200.0f;
  if (a < 0.005f) return;
  gfx_recorte(clip.x, clip.y, clip.w, clip.h);
  gfx_cor(R, 0.0f, 0.02f, 0.02f, 0.025f, a);
  { int k;
    for (k = 1; k >= 0; k--) {
      float ak = k == 0 ? banT : 1.0f - banT, s = k == 0 ? 1.0f + 0.04f * (1.0f - banT) : 1.0f;
      GfxRect r = { R.x - R.w * (s - 1.0f) * 0.5f, R.y - R.h * (s - 1.0f) * 0.4f, R.w * s, R.h * s };
      if (k == 1 && banT >= 0.999f) continue;
      // Sem o esqueleto cinza: enquanto a arte nao chegou, o banner e o escuro.
      if (banUrl[k][0] && tex_obter_larg(banUrl[k], (int)r.w)) capaArte(r, banUrl[k], 0.0f, a * ak * banLuz);
    } }
  gfx_veu_css(R, 2, 0.0f, 0.72f, 0.88f * a);
  gfx_veu_css(R, 0, 0.0f, 0.5f, a);
  { float na = a * banNomeA * anim_clamp((e - 0.35f) / 0.5f, 0.0f, 1.0f);
    if (banT < 0.999f) bannerNome(1, na * (1.0f - banT), -20.0f * banT);
    bannerNome(0, na * (banT < 0.999f ? banT : 1.0f), 20.0f * (1.0f - banT)); }
  gfx_sem_recorte();
  // A folha: degrade de 120 px que vira o preto da ilha, ate o pe da tela.
  gfx_veu_css((GfxRect){ 0.0f, sheetY - 120.0f, NV_TELA_W, 121.0f }, 0, 0.0f, 1.0f, 0.96f * a);
  gfx_cor((GfxRect){ 0.0f, sheetY, NV_TELA_W, NV_TELA_H - sheetY + 2.0f }, 0.0f, 0.0f, 0.0f, 0.0f, 0.96f * a);
}
