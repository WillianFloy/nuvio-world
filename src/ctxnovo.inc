// ============================================================================
// O MENU DE SEGURAR OK, FORMAS NOVAS (World, 24/11/2026) — incluido por ctxmenu.c
//
// O dono reprovou as Facetas na TCL e aprovou duas formas, com um ajuste para
// escolher (Ajustes > Cartazes e trailers > Estilo do menu do cartaz):
//
//   FOLHA (padrao): painel de altura inteira no lado oposto ao item. A arte do
//   PROPRIO titulo voa do cartaz ate o topo da folha (pôster -> imagem larga);
//   embaixo, o nome no canto, notas e sinopse (ctxinfo_compacto) e as acoes.
//   DESDOBRAR: a superficie nasce atras do cartaz e se estende para o lado; o
//   resto da fileira daquele lado desliza a largura do painel (app.c redesenha
//   a faixa da copia parada deslocada). No Palco o painel ocupa a ficha.
//
// UM RELOGIO SO (`fct`, 0..1, linear) conduz tudo; cada camada le a sua janela
// dele (fundo, superficie, arte, conteudo em cascata). Fechar e o mesmo
// caminho ao contrario: o conteudo, que mora no fim da janela, sai primeiro.
// Geometria pura em ctxgeo.c (testada).
//
// MATERIAL (Fundacao, prancha 8.1): Tinta 97 %, fio de 1,5 px a 10 %, sombra
// longa, sem vidro. FOCO: veu osso 13 % + contorno de 2 px (nunca o acento).
// ============================================================================
#define NOVO_TINTA_R .055f
#define NOVO_TINTA_G .055f
#define NOVO_TINTA_B .063f
#define NOVO_OSSO_R  .957f
#define NOVO_OSSO_G  .949f
#define NOVO_OSSO_B  .933f

static GfxRect novoRowRect[CTX_MAX];   // as linhas do ultimo quadro (ponteiro e testes)

static float novoLim(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static float novoMix(float a, float b, float t) { return a + (b - a) * t; }
static GfxRect novoMixR(GfxRect a, GfxRect b, float t) {
  return (GfxRect){ novoMix(a.x, b.x, t), novoMix(a.y, b.y, t), novoMix(a.w, b.w, t), novoMix(a.h, b.h, t) };
}

// A superficie: sombra longa, Tinta 97 %, fio fino.
static void novoSup(GfxRect r, float raioPx, float a) {
  float raio;
  if (a <= 0.004f || r.w < 2.0f || r.h < 2.0f) return;
  raio = raioPx / r.h;
  gfx_sombra_sob((GfxRect){ r.x - 26.0f, r.y - 6.0f, r.w + 52.0f, r.h + 60.0f }, 1.0f, 0, 0.5f,
                 0, 0, 0, .52f * a, r, raioPx, .97f * a);
  gfx_cor(r, raio, NOVO_TINTA_R, NOVO_TINTA_G, NOVO_TINTA_B, .97f * a);
  gfx_anel(r, raio, 1.5f, 1, 1, 1, .10f * a);
}

// O foco de acao: veu claro com contorno de 2 px.
static void novoFoco(GfxRect r, float raioPx, float k, float a) {
  float raio;
  if (k * a <= 0.004f || r.h < 2.0f) return;
  raio = raioPx / r.h;
  gfx_cor(r, raio, NOVO_OSSO_R, NOVO_OSSO_G, NOVO_OSSO_B, .13f * k * a);
  gfx_anel(r, raio, 2.0f, NOVO_OSSO_R, NOVO_OSSO_G, NOVO_OSSO_B, k * a);
}

// Uma linha de acao: icone + rotulo; `k` = foco (mola), `est` 0..1 troca o
// icone pelo de estado (carregando/feito) e `rotEst` o rotulo pela mensagem.
static void novoLinha(GfxRect r, const char *icone, const char *rot, float k, float a,
                      const char *iconeEst, const char *rotEst, float est) {
  float is = 26.0f, ix = r.x + 20.0f, iy = r.y + (r.h - is) * 0.5f, tx = ix + is + 16.0f;
  float tinta = .72f + .28f * k, wmax = r.x + r.w - 20.0f - tx;
  TxtLinha t;
  if (a <= 0.004f) return;
  gfx_icone((GfxRect){ ix, iy, is, is }, icone, NOVO_OSSO_R, NOVO_OSSO_G, NOVO_OSSO_B, tinta * (1.0f - est) * a);
  if (iconeEst && est > 0.004f)
    gfx_icone((GfxRect){ ix, iy, is, is }, iconeEst, NOVO_OSSO_R, NOVO_OSSO_G, NOVO_OSSO_B, est * a);
  t = txt_linha_corta(k > 0.5f ? TXT_V2_LN_B : TXT_V2_LN, rot, 243, 242, 239, 255, wmax);
  txt_desenhar_alpha(t, tx, r.y + (r.h - (float)t.h) * 0.5f, tinta * (1.0f - (rotEst ? est : 0.0f)) * a);
  if (rotEst && est > 0.004f) {
    TxtLinha u = txt_linha_corta(TXT_V2_LN_B, rotEst, 243, 242, 239, 255, wmax);
    txt_desenhar_alpha(u, tx, r.y + (r.h - (float)u.h) * 0.5f + 6.0f * (1.0f - est), est * a);
  }
}

// O ESTADO DA ESCRITA na linha em foco: carregando (linha correndo no
// acento), feito (icone de confirmado + mensagem), falha (mensagem).
typedef struct { const char *icone, *rot; float est, linha; } NovoEstado;
static NovoEstado novoEstado(const char *mensagem, Uint32 agora) {
  NovoEstado e = { NULL, NULL, 0.0f, 0.0f };
  if (!mensagem) return e;
  if (estadoOperacao == CTX_PENDENTE) {
    e.icone = "aj_loader"; e.rot = i18n(mensagem); e.est = 1.0f;
    e.linha = (float)(agora % 1100u) / 1100.0f;
  } else if (estadoOperacao == CTX_CONFIRMADA && agora < facFlashAte) {
    e.icone = "aj_check"; e.rot = i18n(mensagem); e.est = 1.0f;
  } else if (estadoOperacao == CTX_FALHA && agora - facFalhaEm < 3200u) {
    e.icone = "aj_x"; e.rot = i18n(mensagem); e.est = 1.0f;
  }
  return e;
}

// As linhas de acao numa coluna (x, y, w), com a mola de foco, a pressao e o
// estado. `ca(i)` = alfa de cada linha (cascata). Devolve a altura.
static float novoAcoes(const CatItem *ci, float x, float y, float w, float p, float a,
                       float cascIni, const char *mensagem, Uint32 agora) {
  int i;
  NovoEstado e = novoEstado(mensagem, agora);
  for (i = 0; i < nOps && i < CTX_MAX; i++) {
    float ca = a * ctxgeo_fase(p, cascIni + .05f * (float)i, cascIni + .05f * (float)i + .32f);
    float kf = novoLim(focoAnim[i]);
    float pr = i == foco ? .03f * facPulso : 0.0f;
    GfxRect r = { x, y + (float)i * CTXG_LINHA_PASSO + 8.0f * (1.0f - ca / (a > 0.0f ? a : 1.0f)), w, CTXG_LINHA_H };
    GfxRect rp = { r.x + r.w * pr * 0.5f, r.y + r.h * pr * 0.5f, r.w * (1.0f - pr), r.h * (1.0f - pr) };
    novoRowRect[i] = r;
    novoFoco(rp, 16.0f, kf, ca);
    novoLinha(rp, iconeOp(i, ci), ops[i].rot, kf, ca, i == foco ? e.icone : NULL, i == foco ? e.rot : NULL,
              i == foco ? e.est : 0.0f);
    if (i == foco && estadoOperacao == CTX_PENDENTE && e.est > 0.0f) {
      float lw = (rp.w - 40.0f) * .28f, lx = rp.x + 20.0f + (rp.w - 40.0f - lw) * e.linha;
      float ar, ag, ab; ajustes_acento(&ar, &ag, &ab);
      gfx_cor((GfxRect){ lx, rp.y + rp.h - 7.0f, lw, 3.0f }, .5f, ar, ag, ab, ca);
    }
    if (aberto && p > 0.6f && pagina == 0 && ponteiro_ativo())
      ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroCtxOpcao, NULL, i, 0);
  }
  return (float)nOps * CTXG_LINHA_PASSO;
}

// A CONFIRMACAO (Tirar de Continuar, extras): cruza com as acoes, na mesma
// coluna e no mesmo material. Kicker, pergunta, texto e as duas linhas.
static void novoConfirmar(const CatItem *ci, float x, float y, float w, float a) {
  char pergunta[240];
  const char *texto = itemASeguir(ci)
    ? i18n("A série sai da fileira. Ela volta quando você assistir um episódio novo.")
    : i18n("O ponto onde você parou é apagado. O título volta para a fileira se você assistir de novo.");
  const char *kicker = "Continuar assistindo", *botao = "Tirar da fileira";
  float h, ty = y;
  int i;
  if (a <= 0.004f) return;
  snprintf(pergunta, sizeof pergunta, i18n("Tirar %s da fileira?"), ci->titulo);
  if (confExtra >= 0 && confExtra < nExtras) {
    const CtxExtra *x0 = &extras[confExtra];
    snprintf(pergunta, sizeof pergunta, i18n(x0->pergunta ? x0->pergunta : "%s"), ci->titulo);
    texto = i18n(x0->texto ? x0->texto : "");
    kicker = x0->kicker ? x0->kicker : "";
    botao = x0->rot;
  }
  kickerCtx(kicker, x + 4.0f, ty, a);
  ty += 30.0f;
  h = txt_bloco_corta(TXT_ILHA_PERGUNTA, pergunta, 243, 242, 239, x + 4.0f, ty, w - 8.0f, 44.0f, a, 2);
  ty += h + 12.0f;
  h = txt_bloco_corta(TXT_ILHA_TEXTO, texto, 243, 242, 239, x + 4.0f, ty, w - 8.0f, 30.0f, .62f * a, 4);
  ty += h + 22.0f;
  if (aberto && confT > 0.5f && ponteiro_ativo())
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroCtxFora, 0, 0);
  for (i = 0; i < 2; i++) {
    GfxRect r = { x, ty + (float)i * CTXG_LINHA_PASSO, w, CTXG_LINHA_H };
    float kf = novoLim(confAnim[i]);
    novoFoco(r, 16.0f, kf, a);
    novoLinha(r, i == 0 ? "aj_check" : "aj_x", i == 0 ? i18n(botao) : i18n("Cancelar"), kf, a, NULL, NULL, 0.0f);
    if (aberto && confT > 0.5f && ponteiro_ativo())
      ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroConfFoco, ponteiroConfOk, i, 0);
  }
}

// O item de origem, por cima do veu: o proprio cartao da home (nunca arte de
// outro titulo) ou, sem ele, a textura guardada ao abrir.
static void novoCartazVivo(float esc, float a) {
  int ok = 0;
  GfxRect base = cartazRect;
  if (!temCartaz || a <= 0.004f) return;
  if (!cartazFixo) { ESCALA_REAL_INI(); ok = home_cartao_foco_por_cima_esc(idx, esc); ESCALA_REAL_FIM(); }
  if (!ok && cartazArte[0]) {
    GLuint t = tex_obter_larg(cartazArte, base.w);
    if (t) {
      GfxRect q = { base.x - base.w * (esc - 1.0f) * 0.5f, base.y - base.h * (esc - 1.0f) * 0.5f, base.w * esc, base.h * esc };
      float raio = ajustes_raio_poster_px() / gfx_escala_ui() / q.h, teto = 0.5f * q.w / q.h;
      if (raio > teto) raio = teto;
      gfx_tex_aspect_atual = tex_aspecto(cartazArte);
      gfx_rect(q, t, GFX_CARD, 0, 0, 0, raio, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    }
  }
}

// A arte larga do titulo (o backdrop), em cover, com cantos `raioPx`.
static const char *novoBackdrop(const CatItem *ci) {
  if (ci->backdrop[0]) return ci->backdrop;
  if (ci->backdropTmdb[0]) return ci->backdropTmdb;
  if (ci->backdropCatalogo[0]) return ci->backdropCatalogo;
  return NULL;
}
static int novoArte(const char *url, GfxRect r, float raioPx, float a) {
  GLuint t;
  if (!url || !url[0] || a <= 0.004f || r.w < 2.0f || r.h < 2.0f) return 0;
  t = tex_obter_larg(url, r.w);
  if (!t) return 0;
  gfx_tex_aspect_atual = tex_aspecto(url);
  gfx_card_forcar_cover_atual = 1.0f;
  gfx_rect(r, t, GFX_CARD, 0, 0, 0, raioPx / r.h, 0, 0, 0, a);
  gfx_card_forcar_cover_atual = 0.0f;
  gfx_tex_aspect_atual = 0.0f;
  return 1;
}

// O retangulo de onde o menu nasce (tela virtual) e se ha um.
static int novoOrigem(GfxRect *o) {
  if (temCartaz) { *o = cartazRect; return 1; }
  if (heroOrigem) { *o = heroBotao; return 1; }
  return 0;
}

// ---------------------------------------------------------------- FOLHA
static void folhaDesenhar(const CatItem *ci, const char *mensagem, float ag) {
  const int red = ajustes_animacoes_reduzidas();
  const float p = red ? 1.0f : fct, al = (red ? fct : 1.0f) * ag;
  const float k = ctxgeo_dec(p), cf = 1.0f - confT;
  const Uint32 agora = SDL_GetTicks();
  GfxRect orig, sheet, arte, voo;
  CtxFolhaGeo g;
  int temOrig = novoOrigem(&orig), temArte;
  const char *bd = novoBackdrop(ci);
  float yy, sx;
  if (al <= 0.003f) return;
  ctxgeo_folha(temCartaz ? &cartazRect : NULL, NV_TELA_W, NV_TELA_H, &g);
  // A FOLHA TEM A ALTURA DO CONTEUDO (arte, informacoes, acoes), presa no alto;
  // de altura inteira ela sobrava vazia embaixo (captura de 24/11).
  { CtxInfoEstado e0; float hInfo, hTot;
    e0.salvo = tituloSalvo(ci); e0.visto = historicoDe(ci);
    hInfo = ctxinfo_compacto(ci, &e0, 0, 0, g.folha.w - 72.0f, 0.0f, 0);
    hTot = g.arte.h + 22.0f + hInfo + 24.0f + (float)nOps * CTXG_LINHA_PASSO + 24.0f;
    if (confT > 0.003f && hTot < g.arte.h + 22.0f + hInfo + 24.0f + 360.0f) hTot = g.arte.h + 22.0f + hInfo + 24.0f + 360.0f;
    if (hTot < g.folha.h) g.folha.h = hTot; }
  // Sem a copia parada (app.c), o veu e daqui.
  novoFundo.veu = .5f * k * al; novoFundo.esc = 1.0f - .025f * k; novoFundo.dx = 0.0f;
  if (!ctxFundoPronto) gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, novoFundo.veu);
  // O cartaz de origem fica, vivo, por cima do veu.
  novoCartazVivo(1.0f, al);
  // A folha entra 70 px do lado dela.
  sx = (1.0f - k) * 70.0f * (float)g.lado;
  sheet = g.folha; sheet.x += sx;
  arte = g.arte; arte.x += sx;
  novoSupRect = sheet;
  novoSup(sheet, 22.0f, al * novoLim(p / .45f));
  if (aberto && p > 0.5f && ponteiro_ativo() && pagina == 0) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroCtxFora, 0, 0);
    ponteiro_alvo(sheet.x, sheet.y, sheet.w, sheet.h, NULL, NULL, 0, 0);
  }
  // A ARTE VOA do item ate o topo da folha: pôster -> imagem larga no caminho.
  if (heroOrigem && heroArte.w > 8.0f) { orig = heroArte; temOrig = 1; }
  voo = temOrig ? novoMixR(orig, arte, k) : arte;
  { float troca = ctxgeo_fase(p, .2f, .75f), raio = novoMix(14.0f, 22.0f, k);
    float aArte = al * (temOrig ? 1.0f : novoLim(p / .5f));
    if (temCartaz && cartazArte[0] && troca < 0.999f) novoArte(cartazArte, voo, raio, aArte * (1.0f - troca));
    temArte = novoArte(bd ? bd : cartazArte, voo, raio, aArte * (temCartaz ? troca : 1.0f));
    if (!temArte) gfx_cor(voo, raio / voo.h, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, aArte * .6f);
    gfx_veu_base(voo, raio / voo.h, .62f, .85f * aArte * troca); }
  // O NOME NO CANTO da arte (logo ou titulo).
  { float c0 = al * ctxgeo_fase(p, .5f, .82f) * cf;
    if (c0 > 0.004f)
      logotitulo_desenhar(ci, ci->titulo, TXT_ILHA_TITULO, arte.x + 32.0f, arte.y + arte.h - 26.0f - 92.0f + 10.0f * (1.0f - c0 / (al > 0 ? al : 1)),
                          360.0f, 92.0f, arte.w - 64.0f, c0); }
  // NOTAS, META E SINOPSE (o mesmo do cartao de informacoes, compacto).
  yy = arte.y + arte.h + 22.0f;
  { CtxInfoEstado e; float c1 = al * ctxgeo_fase(p, .55f, .88f) * cf;
    e.salvo = tituloSalvo(ci); e.visto = historicoDe(ci);
    yy += ctxinfo_compacto(ci, &e, sheet.x + 36.0f, yy, sheet.w - 72.0f, c1, 1) + 24.0f; }
  // AS ACOES, em cascata; a confirmacao cruza com elas.
  novoAcoes(ci, sheet.x + 20.0f, yy, sheet.w - 40.0f, p, al * cf, .58f, mensagem, agora);
  if (confT > 0.003f) novoConfirmar(ci, sheet.x + 20.0f, yy - 4.0f + 12.0f * (1.0f - confT), sheet.w - 40.0f, al * confT);
}

// ---------------------------------------------------------------- DESDOBRAR
static void desdDesenhar(const CatItem *ci, const char *mensagem, float ag) {
  const int red = ajustes_animacoes_reduzidas();
  const float p = red ? 1.0f : fct, al = (red ? fct : 1.0f) * ag;
  const float k = ctxgeo_dec(p), cf = 1.0f - confT;
  const Uint32 agora = SDL_GetTicks();
  float abaixo = 0.0f, hConteudo, w, h, yy;
  int palco = 0;
  GfxRect it, cont;
  CtxDesdGeo g;
  if (al <= 0.003f || !novoOrigem(&it)) return;
  if (temCartaz && !cartazFixo && home_foco_palco(&abaixo)) palco = 1;
  hConteudo = 28.0f + 40.0f + 34.0f + 26.0f + (float)nOps * CTXG_LINHA_PASSO + 14.0f;
  if (confT > 0.003f) hConteudo = hConteudo > 380.0f ? hConteudo : 380.0f;
  ctxgeo_desdobrar(it, hConteudo, palco, 36.0f, 480.0f, NV_TELA_W, NV_TELA_H, &g);
  novoFundo.veu = .34f * k * al; novoFundo.esc = 1.0f;
  novoFundo.dx = g.empurra * k * (float)g.lado * al;
  { float e = gfx_escala_ui(); novoFundo.faixa = (GfxRect){ g.faixa.x * e, g.faixa.y * e, g.faixa.w * e, g.faixa.h * e };
    novoFundo.dx *= e; }
  if (!ctxFundoPronto) gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, novoFundo.veu);
  // A SUPERFICIE nasce atras do item e se desdobra: largura primeiro, a altura
  // acompanha (so cresce quando o conteudo pede mais que o cartaz).
  w = novoMix(it.w, g.cont.w, k);
  h = novoMix(it.h, g.cont.h, ctxgeo_fase(p, .15f, 1.0f));
  cont = (GfxRect){ g.lado > 0 ? g.cont.x : g.cont.x + g.cont.w - w, novoMix(it.y, g.cont.y, k), w, h };
  if (heroOrigem) cont.y = novoMix(it.y, g.cont.y, k);
  novoSupRect = cont;
  novoSup(cont, 16.0f, al * (p > 0.004f ? 1.0f : 0.0f));
  if (aberto && p > 0.5f && ponteiro_ativo() && pagina == 0) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroCtxFora, 0, 0);
    ponteiro_alvo(cont.x, cont.y, cont.w, cont.h, NULL, NULL, 0, 0);
  }
  // O CARTAZ por cima, no lugar dele (o item real).
  novoCartazVivo(1.0f, al);
  // CONTEUDO no painel: titulo, meta, acoes.
  { GfxRect pn = g.painel;
    float c0 = al * ctxgeo_fase(p, .45f, .78f) * cf, c1 = al * ctxgeo_fase(p, .5f, .84f) * cf;
    TxtLinha t;
    pn.y = cont.y;
    yy = pn.y + 28.0f;
    t = txt_linha_corta(TXT_ILHA_NOME_L, ci->titulo, 243, 242, 239, 255, pn.w - 60.0f);
    txt_desenhar_alpha(t, pn.x + 30.0f, yy + 8.0f * (1.0f - (al > 0 ? c0 / al : 0)), c0);
    yy += (float)t.h + 8.0f;
    { char meta[160];
      snprintf(meta, sizeof meta, "%s%s%s", ci->genero[0] ? ci->genero : "", ci->genero[0] && ci->meta[0] ? " · " : "", ci->meta);
      t = txt_linha_corta(TXT_ILHA_META, meta, 243, 242, 239, 255, pn.w - 60.0f);
      txt_desenhar_alpha(t, pn.x + 30.0f, yy, .62f * c1); }
    yy += 44.0f;
    novoAcoes(ci, pn.x + 16.0f, yy, pn.w - 32.0f, p, al * cf, .55f, mensagem, agora);
    if (confT > 0.003f) novoConfirmar(ci, pn.x + 16.0f, pn.y + 26.0f + 12.0f * (1.0f - confT), pn.w - 32.0f, al * confT); }
}
