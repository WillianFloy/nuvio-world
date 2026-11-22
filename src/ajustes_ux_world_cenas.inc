// AS PRÉVIAS DOS AJUSTES WORLD (21/11; o dono: "cada opção tem que ter um
// design que mostra onde e como vai ficar a configuração, mas sem utilizar o
// que já tem"). Desenho novo e esquemático: uma miniatura da tela onde a opção
// age (Início, Collections, página do título, player, fontes, TV ao vivo...),
// com a parte que muda acesa e contornada no acento, e o resto apagado. Algumas
// opções mudam a própria miniatura com o valor em foco (posição do menu,
// destaque, Continuar assistindo, pôsteres horizontais, exibição das coleções,
// tamanho e posição da legenda, arredondamento e largura do pôster...).
//
// Sem arte, sem rede, sem textura: so retangulos (gfx_cor) e alguns icones.
// Custa dezenas de retangulos pequenos num quadro que ja esta parado.

enum {
  AJW_C_INICIO = 0, AJW_C_COLECOES, AJW_C_COLECAO, AJW_C_TITULO, AJW_C_PLAYER, AJW_C_FONTES,
  AJW_C_TVVIVO, AJW_C_APARENCIA, AJW_C_PERFIS, AJW_C_CONTA, AJW_C_SISTEMA, AJW_C_SOBRE,
  AJW_C_ADDONS, AJW_C_DESCANSO, AJW_C_N
};
enum {
  AJW_A_TUDO = 0, AJW_A_RAIL, AJW_A_TOPO, AJW_A_HERO, AJW_A_CW, AJW_A_FILEIRAS, AJW_A_CARTAZ,
  AJW_A_RELOGIO, AJW_A_PASTA, AJW_A_TITULO_TOPO, AJW_A_BOTOES, AJW_A_NOTAS, AJW_A_EPISODIOS,
  AJW_A_ELENCO, AJW_A_VIDEO, AJW_A_INFO, AJW_A_BARRA, AJW_A_LEGENDA, AJW_A_LEGENDA2, AJW_A_PULAR,
  AJW_A_PROXIMO, AJW_A_PAUSA, AJW_A_LISTA, AJW_A_SELOS, AJW_A_GRADE, AJW_A_CANAL, AJW_A_CORES,
  AJW_A_TEXTO, AJW_A_LOGO, AJW_A_N
};

// --- ONDE MUDA ------------------------------------------------------------------------
static const char *ajwOndeMuda(int cena, int alvo) {
  switch (cena) {
    case AJW_C_INICIO:
      switch (alvo) {
        case AJW_A_RAIL: return "Início › barra lateral";
        case AJW_A_TOPO: return "Início › menu no alto";
        case AJW_A_HERO: return "Início › destaque no alto";
        case AJW_A_CW: return "Início › Continuar assistindo";
        case AJW_A_FILEIRAS: return "Início › fileiras";
        case AJW_A_CARTAZ: return "Início › cartaz em foco";
        case AJW_A_RELOGIO: return "Todas as telas › relógio no canto";
        default: return "Início";
      }
    case AJW_C_COLECOES: return "Início › fileira das Collections";
    case AJW_C_COLECAO:
      return alvo == AJW_A_CARTAZ ? "Dentro das Collections › pôster em foco"
           : alvo == AJW_A_FILEIRAS ? "Dentro das Collections › fileiras" : "Ao abrir uma coleção";
    case AJW_C_TITULO:
      switch (alvo) {
        case AJW_A_TITULO_TOPO: return "Página do título › topo";
        case AJW_A_BOTOES: return "Página do título › botões";
        case AJW_A_NOTAS: return "Página do título › notas";
        case AJW_A_EPISODIOS: return "Página do título › episódios";
        case AJW_A_ELENCO: return "Página do título › elenco";
        default: return "Página do título";
      }
    case AJW_C_PLAYER:
      switch (alvo) {
        case AJW_A_INFO: return "Player › informações na tela";
        case AJW_A_BARRA: return "Player › barra de tempo";
        case AJW_A_LEGENDA: return "Player › legenda";
        case AJW_A_LEGENDA2: return "Player › segunda legenda";
        case AJW_A_PULAR: return "Player › botão de pular";
        case AJW_A_PROXIMO: return "Player › fim do episódio";
        case AJW_A_PAUSA: return "Player › ao pausar";
        default: return "Player › imagem e som";
      }
    case AJW_C_FONTES: return alvo == AJW_A_SELOS ? "Lista de fontes › selos" : "Lista de fontes";
    case AJW_C_TVVIVO:
      return alvo == AJW_A_GRADE ? "TV ao vivo › guia" : alvo == AJW_A_VIDEO ? "TV ao vivo › canal tocando" : "TV ao vivo › canais";
    case AJW_C_APARENCIA:
      return alvo == AJW_A_CORES ? "Todas as telas › cores" : alvo == AJW_A_TEXTO ? "Todas as telas › texto"
           : alvo == AJW_A_LOGO ? "Abertura e logo do app" : "Todas as telas";
    case AJW_C_PERFIS: return "Escolha de perfil";
    case AJW_C_CONTA: return "Conta e sincronização";
    case AJW_C_SISTEMA: return "Esta TV";
    case AJW_C_SOBRE: return "Sobre o app";
    case AJW_C_ADDONS: return "Addons e servidores";
    case AJW_C_DESCANSO: return "Tela de descanso";
  }
  return "";
}

// --- QUE CENA E QUE PARTE ---------------------------------------------------------------
static int ajwSecaoChamada(int s, const char *titulo) {
  return s >= 0 && s < nSecoes && !strcmp(TELA[secIni[s]].titulo, titulo);
}
static int ajwCenaDe(int op, int *alvo) {
  int s = uxSecaoDe(op);
  const char *b = uxBloco(op);
  *alvo = AJW_A_TUDO;
  // Opcoes com lugar proprio, antes das regras por bloco.
  switch (op) {
    case AJ_CW_CONCLUIDO: case AJ_CW_MINIMO: case AJ_CW_FONTE: case AJ_CW_PROXIMO: case AJ_HIST_CONTA:
      *alvo = AJW_A_CW; return AJW_C_INICIO;
    case AJ_PULAR_ABERTURA: case AJ_PULAR_RESUMO: case AJ_PULAR_CREDITOS: *alvo = AJW_A_PULAR; return AJW_C_PLAYER;
    case AJ_PROXIMO_AUTO: case AJ_PROXIMO_CONTAGEM: case AJ_REACAO_CREDITOS: *alvo = AJW_A_PROXIMO; return AJW_C_PLAYER;
    case AJ_PAUSA_OVERLAY: *alvo = AJW_A_PAUSA; return AJW_C_PLAYER;
    case AJ_ESTAT_TAMANHO: case AJ_ESTAT_LADO: case AJ_PLR_CLASSIF: case AJ_PLR_FORMATO: case AJ_SAIDA_PLAYER:
      *alvo = AJW_A_INFO; return AJW_C_PLAYER;
    case AJ_IDIOMA: *alvo = AJW_A_TEXTO; return AJW_C_APARENCIA;
    case AJ_AUD_LINGUA: case AJ_AUD_LINGUA2: *alvo = AJW_A_VIDEO; return AJW_C_PLAYER;
    case AJ_TMDB_IDIOMA: *alvo = AJW_A_TITULO_TOPO; return AJW_C_TITULO;
    case AJ_TMDB_ELENCO: *alvo = AJW_A_ELENCO; return AJW_C_TITULO;
    case AJ_TMDB_EPS: case AJ_EP_TAMANHO: case AJ_DET_BLUR_NAO_VISTOS: case AJ_NT_EP: *alvo = AJW_A_EPISODIOS; return AJW_C_TITULO;
    case AJ_DET_TRAILER: *alvo = AJW_A_BOTOES; return AJW_C_TITULO;
    case AJ_TMDB_TRAILERS: return AJW_C_TITULO;
    case AJ_DET_TRAILER_AUTO: *alvo = AJW_A_TITULO_TOPO; return AJW_C_TITULO;
    case AJ_HERO_TRAILER: case AJ_HERO_TRAILER_ESPERA: case AJ_HERO_TRAILER_SOM: *alvo = AJW_A_HERO; return AJW_C_INICIO;
    case AJ_FOCO_TRAILER: *alvo = AJW_A_CARTAZ; return AJW_C_INICIO;
    case AJ_MENU_TOPO: case AJ_MENU_TOPO_MODO: case AJ_MENU_TOPO_TAMANHO: case AJ_MENU_TOPO_ORDEM:
    case AJ_MENU_TOPO_FIXO: case AJ_MENU_TOPO_FOCO_EXPANDIDO:
      *alvo = ajustes_menu_topo() || op == AJ_MENU_TOPO_TAMANHO || op == AJ_MENU_TOPO_FIXO ||
              op == AJ_MENU_TOPO_FOCO_EXPANDIDO ? AJW_A_TOPO : AJW_A_RAIL;
      return AJW_C_INICIO;
    case AJ_RELOGIO: case AJ_RELOGIO_12H: *alvo = AJW_A_RELOGIO; return AJW_C_INICIO;
    case AJ_ANIM: *alvo = AJW_A_TUDO; return AJW_C_INICIO;
    case AJ_TITULO_FIL: case AJ_PALCO_FUNDO: *alvo = AJW_A_FILEIRAS; return AJW_C_INICIO;
    case AJ_LOGO_APP: case AJ_ABERTURA: case AJ_ABERTURA_VIDEO: case AJ_ICONE_APP: *alvo = AJW_A_LOGO; return AJW_C_APARENCIA;
    case AJ_TELA_ABERTURA: case AJ_ABERTURA_ADDON: case AJ_ABERTURA_FUNDO: case AJ_ABERTURA_ESTILO:
      *alvo = AJW_A_VIDEO; return AJW_C_PLAYER;
    case AJ_COLECAO_EXIBICAO: *alvo = AJW_A_TUDO; return AJW_C_COLECAO;
    case AJ_COL_ITENS: *alvo = AJW_A_FILEIRAS; return AJW_C_COLECAO;
    case AJ_COL_EXPANDIR: case AJ_COL_EXPANDIR_ATRASO: *alvo = AJW_A_CARTAZ; return AJW_C_COLECAO;
    case AJ_PS_FUNDO: case AJ_PS_CONTINUAR: case AJ_PERFIL_ATIVO: case AJ_PERFIL_EDITAR: return AJW_C_PERFIS;
    case AJ_GUIA: case AJ_NOVIDADES20: case AJ_DICASCONTROLE: case AJ_APOIAR: case AJ_AVISO_LEGAL: return AJW_C_SOBRE;
  }
  if (ajwSecaoChamada(s, "Reprodução")) {
    if (!strcmp(b, "Miniatura ao avançar")) { *alvo = AJW_A_BARRA; return AJW_C_PLAYER; }
    if (!strcmp(b, "Escolha da fonte") || !strcmp(b, "P2P")) { *alvo = AJW_A_LISTA; return AJW_C_FONTES; }
    *alvo = AJW_A_VIDEO; return AJW_C_PLAYER;
  }
  if (ajwSecaoChamada(s, "Idiomas e legendas")) {
    if (!strcmp(b, "Segunda legenda") || op == AJ_LEG_LINGUA2) { *alvo = AJW_A_LEGENDA2; return AJW_C_PLAYER; }
    *alvo = AJW_A_LEGENDA; return AJW_C_PLAYER;
  }
  if (ajwSecaoChamada(s, "Fontes e addons")) {
    if (!strcmp(b, "Debrid")) { *alvo = AJW_A_SELOS; return AJW_C_FONTES; }
    if (!strcmp(b, "Falha na reprodução")) { *alvo = AJW_A_LISTA; return AJW_C_FONTES; }
    return AJW_C_ADDONS;
  }
  if (ajwSecaoChamada(s, "TV ao vivo")) {
    if (!strcmp(b, "Guia de TV")) { *alvo = AJW_A_GRADE; return AJW_C_TVVIVO; }
    if (!strcmp(b, "Se o canal não abre")) { *alvo = AJW_A_VIDEO; return AJW_C_TVVIVO; }
    *alvo = AJW_A_CANAL; return AJW_C_TVVIVO;
  }
  if (ajwSecaoChamada(s, "Tela inicial")) {
    if (!strcmp(b, "Estilo")) {
      *alvo = op == AJ_HOME_LAYOUT || op == AJ_LANDSCAPE ? AJW_A_TUDO : AJW_A_HERO;
      if (op == AJ_LANDSCAPE) *alvo = AJW_A_FILEIRAS;
      return AJW_C_INICIO;
    }
    if (!strcmp(b, "Continuar assistindo")) { *alvo = AJW_A_CW; return AJW_C_INICIO; }
    if (!strcmp(b, "Barra lateral")) { *alvo = AJW_A_RAIL; return AJW_C_INICIO; }
    if (!strcmp(b, "Escolha de perfil")) return AJW_C_PERFIS;
    *alvo = AJW_A_FILEIRAS; return AJW_C_INICIO;
  }
  if (ajwSecaoChamada(s, "Coleções")) { *alvo = AJW_A_PASTA; return AJW_C_COLECOES; }
  if (ajwSecaoChamada(s, "Aparência")) {
    if (!strcmp(b, "Texto e tamanho")) { *alvo = AJW_A_TEXTO; return AJW_C_APARENCIA; }
    if (!strcmp(b, "Tela de descanso")) return AJW_C_DESCANSO;
    *alvo = AJW_A_CORES; return AJW_C_APARENCIA;
  }
  if (ajwSecaoChamada(s, "Página do título")) {
    if (!strcmp(b, "Notas")) { *alvo = AJW_A_NOTAS; return AJW_C_TITULO; }
    *alvo = !strcmp(b, "O que aparece") ? AJW_A_TITULO_TOPO : AJW_A_TUDO;
    return AJW_C_TITULO;
  }
  if (ajwSecaoChamada(s, "Cartazes e trailers")) {
    if (!strcmp(b, "De onde vem a arte") || !strcmp(b, "Som e imagem")) { *alvo = AJW_A_HERO; return AJW_C_INICIO; }
    if (!strcmp(b, "Trailers")) { *alvo = AJW_A_HERO; return AJW_C_INICIO; }
    *alvo = AJW_A_CARTAZ; return AJW_C_INICIO;
  }
  if (ajwSecaoChamada(s, "Conta e serviços")) return AJW_C_CONTA;
  if (ajwSecaoChamada(s, "Esta TV")) return AJW_C_SISTEMA;
  if (ajwSecaoChamada(s, "Sobre e ajuda")) return AJW_C_SOBRE;
  return AJW_C_INICIO;
}

// --- DESENHO --------------------------------------------------------------------------
static int ajwAlvo, ajwDesl;
static float ajwCA = 1.0f;          // alfa da cena (troca)
static GfxRect ajwAlvoR;            // caixa da parte acesa (o contorno)
static int ajwAlvoTem;
static GfxRect ajwRc;               // a moldura da cena
static float ajwAP(int parte) {
  if (ajwAlvo == AJW_A_TUDO) return ajwCA;
  if (parte == ajwAlvo) return ajwCA * (ajwDesl ? 0.2f : 1.0f);
  return ajwCA * 0.5f;
}
// Coordenadas da cena em fracao da moldura.
static GfxRect ajwF(float x, float y, float w, float h) {
  return (GfxRect){ ajwRc.x + ajwRc.w * x, ajwRc.y + ajwRc.h * y, ajwRc.w * w, ajwRc.h * h };
}
static void ajwB(int parte, GfxRect r, float raio, float cinza) {
  ajwRet(r, raio, cinza, ajwAP(parte));
}
static void ajwParte(int parte, GfxRect caixa) {
  if (parte != ajwAlvo || ajwAlvo == AJW_A_TUDO) return;
  if (!ajwAlvoTem) { ajwAlvoR = caixa; ajwAlvoTem = 1; return; }
  { float x0 = fminf(ajwAlvoR.x, caixa.x), y0 = fminf(ajwAlvoR.y, caixa.y);
    float x1 = fmaxf(ajwAlvoR.x + ajwAlvoR.w, caixa.x + caixa.w), y1 = fmaxf(ajwAlvoR.y + ajwAlvoR.h, caixa.y + caixa.h);
    ajwAlvoR = (GfxRect){ x0, y0, x1 - x0, y1 - y0 }; }
}
static void ajwAcentoP(int parte, GfxRect r, float raio) {
  float ar, ag, ab; ajwAcento(&ar, &ag, &ab);
  ajwRetRGB(r, raio, ar, ag, ab, ajwAP(parte));
}
// Seta Lucide em qualquer direcao (so ha a chevron-right nos icones): 0 dir, 1 esq, 2 baixo, 3 cima.
static void ajwSeta(int dir, float x, float y, float lado, float c, float a) {
  static const float ANG[4] = { 0.0f, 3.14159265f, 1.5707963f, -1.5707963f };
  if (dir) gfx_girar(ANG[dir & 3], x + lado * 0.5f, y + lado * 0.5f);
  gfx_icone((GfxRect){ x, y, lado, lado }, "aj_chevron-right", c, c, c, a);
  if (dir) gfx_sem_girar();
}

// Pôster esquemático (com o arredondamento e o foco).
static void ajwPoster(int parte, GfxRect r, float raio, float cinza, int foco) {
  ajwB(parte, r, raio, foco ? cinza + 0.12f : cinza);
  if (foco) gfx_anel_fora(r, raio / r.h, 3, 3, AJW_BRANCO, ajwAP(parte));
}

static void ajwCenaInicio(int op, int v) {
  int topo = op == AJ_MENU_TOPO ? v == 1 : ajustes_menu_topo();
  int hero = op == AJ_HERO ? v == 0 : lig(AJ_HERO);
  int cw = op == AJ_CW_LIGADO ? v == 0 : lig(AJ_CW_LIGADO);
  int deitado = op == AJ_LANDSCAPE ? v == 0 : lig(AJ_LANDSCAPE);
  int padrao = op == AJ_HOME_LAYOUT ? v == HOME_LAYOUT_PADRAO : ajustes_home_layout() == HOME_LAYOUT_PADRAO;
  int relogio = op == AJ_RELOGIO ? v == 0 : lig(AJ_RELOGIO);
  int expande = op == AJ_EXPANDIR ? v == 0 : lig(AJ_EXPANDIR);
  float x0 = topo ? 0.04f : 0.10f, y = topo ? 0.13f : 0.05f, raio = 6.0f, lp = 0.085f;
  if (op == AJ_RAIO_DP) raio = 1.0f + 14.0f * (float)(v - OPCOES[op].min) / (float)(OPCOES[op].max - OPCOES[op].min);
  if (op == AJ_LARGURA_DP) lp = 0.05f + 0.07f * (float)(v - OPCOES[op].min) / (float)(OPCOES[op].max - OPCOES[op].min);
  // Menu: barra lateral ou no alto.
  if (topo) {
    GfxRect r = ajwF(0.0f, 0.0f, 1.0f, 0.09f);
    ajwB(AJW_A_TOPO, r, 0, 0.09f);
    for (int i = 0; i < 6; i++) ajwB(AJW_A_TOPO, ajwF(0.25f + i * 0.085f, 0.03f, 0.06f, 0.03f), 3, i == 0 ? 0.62f : 0.34f);
    ajwParte(AJW_A_TOPO, r);
  } else {
    GfxRect r = ajwF(0.0f, 0.0f, 0.06f, 1.0f);
    ajwB(AJW_A_RAIL, r, 0, 0.09f);
    for (int i = 0; i < 6; i++) ajwB(AJW_A_RAIL, ajwF(0.018f, 0.16f + i * 0.09f, 0.024f, 0.042f), 4, i == 0 ? 0.62f : 0.34f);
    ajwParte(AJW_A_RAIL, r);
  }
  if (relogio) { GfxRect r = ajwF(0.86f, topo ? 0.025f : 0.04f, 0.1f, 0.045f); ajwB(AJW_A_RELOGIO, r, 6, 0.55f); ajwParte(AJW_A_RELOGIO, r); }
  // Destaque.
  if (hero) {
    float hh = padrao ? 0.30f : 0.44f;
    GfxRect r = ajwF(x0 - (topo ? 0 : 0.04f), y - (topo ? 0 : 0.05f), 1.0f - x0 + (topo ? 0 : 0.04f), hh + (topo ? 0 : 0.05f));
    ajwB(AJW_A_HERO, r, 0, padrao ? 0.12f : 0.17f);
    ajwB(AJW_A_HERO, ajwF(x0 + 0.02f, y + hh * 0.42f, 0.24f, 0.06f), 4, 0.7f);
    ajwB(AJW_A_HERO, ajwF(x0 + 0.02f, y + hh * 0.42f + 0.085f, 0.34f, 0.022f), 3, 0.42f);
    ajwB(AJW_A_HERO, ajwF(x0 + 0.02f, y + hh * 0.42f + 0.125f, 0.28f, 0.022f), 3, 0.32f);
    if (op == AJ_HERO_TRAILER || op == AJ_HERO_TRAILER_ESPERA || op == AJ_HERO_TRAILER_SOM) {
      GfxRect p = ajwF(0.62f, y + hh * 0.3f, 0.07f, 0.12f);
      gfx_icone(p, "aj_play", AJW_BRANCO, ajwAP(AJW_A_HERO) * (ajwDesl ? 0.4f : 0.9f));
    }
    ajwParte(AJW_A_HERO, r);
    y += hh + 0.04f;
  }
  // Continuar assistindo.
  if (cw) {
    ajwB(AJW_A_CW, ajwF(x0, y, 0.12f, 0.022f), 3, 0.42f);
    y += 0.04f;
    for (int i = 0; i < 4; i++) {
      GfxRect r = ajwF(x0 + i * 0.205f, y, 0.19f, 0.105f);
      ajwB(AJW_A_CW, r, 5, 0.22f);
      ajwB(AJW_A_CW, ajwF(x0 + i * 0.205f + 0.01f, y + 0.088f, 0.17f * 0.55f, 0.008f), 2, 0.75f);
    }
    ajwParte(AJW_A_CW, ajwF(x0 - 0.01f, y - 0.05f, 0.83f, 0.17f));
    y += 0.15f;
  }
  // Fileiras de pôsteres (a primeira com o cartaz em foco).
  for (int f = 0; f < 2 && y < 0.97f; f++) {
    float ph = deitado ? 0.13f : 0.24f, pw = deitado ? 0.15f : lp, gx = deitado ? 0.017f : 0.016f, x = x0;
    GfxRect cab = ajwF(x0, y, 0.14f, 0.022f);
    ajwB(AJW_A_FILEIRAS, cab, 3, op == AJ_TITULO_FIL ? 0.7f : 0.42f);
    if (op == AJ_TITULO_FIL && valor[AJ_TITULO_FIL] != 0) ajwAcentoP(AJW_A_FILEIRAS, ajwF(x0 - 0.012f, y, 0.005f, 0.022f), 2);
    y += 0.04f;
    for (int i = 0; x < 0.99f && i < 12; i++) {
      int foco = f == 0 && i == 1;
      float w = pw;
      if (foco && expande) w = deitado ? pw : pw * 2.6f;
      GfxRect r = ajwF(x, y, fminf(w, 0.995f - x), ph);
      if (r.w > 4) {
        int parte = foco ? AJW_A_CARTAZ : AJW_A_FILEIRAS;
        ajwPoster(parte, r, raio, 0.21f + 0.03f * (i % 3), foco);
        if (foco) {
          ajwParte(AJW_A_CARTAZ, r);
          if (op == AJ_FOCO_TRAILER) gfx_icone(ajwF(x + w * 0.5f - 0.03f, y + ph * 0.5f - 0.05f, 0.06f, 0.1f), "aj_play", AJW_BRANCO, ajwAP(parte) * 0.9f);
        }
      }
      x += w + gx;
    }
    ajwParte(AJW_A_FILEIRAS, ajwF(x0 - 0.01f, y - 0.05f, 0.995f - x0 + 0.01f, ph + 0.06f));
    y += ph + 0.05f;
  }
}

static void ajwCenaColecoes(int op, int v) {
  int gif = op == AJ_COL_GIF ? v != 2 : ajustes_col_gif() != 2;
  float x0 = 0.10f, y = 0.06f;
  ajwB(AJW_A_TUDO, ajwF(0, 0, 0.06f, 1), 0, 0.09f);
  ajwB(AJW_A_FILEIRAS, ajwF(x0, y, 0.62f, 0.26f), 6, 0.14f);
  y += 0.33f;
  ajwB(AJW_A_PASTA, ajwF(x0, y, 0.14f, 0.024f), 3, 0.55f);
  y += 0.045f;
  for (int i = 0; i < 4; i++) {
    GfxRect r = ajwF(x0 + i * 0.215f, y, 0.2f, 0.2f);
    int foco = i == 1;
    ajwB(AJW_A_PASTA, r, 8, foco ? 0.34f : 0.2f);
    if (foco) {
      gfx_anel_fora(r, 8.0f / r.h, 3, 3, AJW_BRANCO, ajwAP(AJW_A_PASTA));
      if (gif && op == AJ_COL_GIF) {   // o GIF: quadros que passam
        for (int q = 0; q < 3; q++) ajwB(AJW_A_PASTA, ajwF(x0 + i * 0.215f + 0.03f + q * 0.05f, y + 0.15f, 0.035f, 0.025f), 3, q == 1 ? 0.8f : 0.45f);
        gfx_icone(ajwF(x0 + i * 0.215f + 0.075f, y + 0.045f, 0.05f, 0.08f), "aj_play", AJW_BRANCO, ajwAP(AJW_A_PASTA));
      }
      if (op == AJ_COLECOES_STATUS) ajwAcentoP(AJW_A_PASTA, ajwF(x0 + i * 0.215f + 0.14f, y + 0.015f, 0.045f, 0.035f), 6);
      ajwParte(AJW_A_PASTA, r);
    }
    if (op == AJ_COL_ENTRADA && v == 0) ajwB(AJW_A_PASTA, ajwF(x0 + i * 0.215f, y + 0.22f, 0.2f * (1.0f - i * 0.22f), 0.008f), 2, 0.5f);
  }
  y += 0.27f;
  ajwB(AJW_A_FILEIRAS, ajwF(x0, y, 0.12f, 0.022f), 3, 0.38f);
  for (int i = 0; i < 9; i++) ajwB(AJW_A_FILEIRAS, ajwF(x0 + i * 0.1f, y + 0.045f, 0.085f, 0.24f), 5, 0.19f);
}

// Dentro de uma colecao, no arranjo `ex` (V_COLECAO_EXIBICAO).
static void ajwCenaColecao(int op, int v) {
  int ex = op == AJ_COLECAO_EXIBICAO ? v : valor[AJ_COLECAO_EXIBICAO];
  int expande = op == AJ_COL_EXPANDIR ? v == 0 : 1;
  float x0 = 0.06f;
  ajwB(AJW_A_TUDO, ajwF(x0, 0.05f, 0.3f, 0.05f), 4, 0.62f);   // nome da colecao
  if (ex == 0) {   // Automatico: o arranjo que o app escolhe (aqui, a vitrine) com o selo A
    ajwB(AJW_A_TUDO, ajwF(0.83f, 0.05f, 0.11f, 0.06f), 8, 0.32f);
    ex = 7;
  }
  switch (ex) {
    case 1:   // Grade
      for (int l = 0; l < 2; l++) for (int i = 0; i < 8; i++)
        ajwPoster(AJW_A_TUDO, ajwF(x0 + i * 0.112f, 0.16f + l * 0.4f, 0.1f, 0.36f), 5, 0.2f + 0.03f * ((i + l) % 3), l == 0 && i == 2);
      break;
    case 2:   // Linha do tempo
      ajwB(AJW_A_TUDO, ajwF(x0, 0.6f, 0.88f, 0.006f), 2, 0.4f);
      for (int i = 0; i < 6; i++) {
        ajwPoster(AJW_A_TUDO, ajwF(x0 + i * 0.15f, 0.2f, 0.12f, 0.33f), 5, 0.22f, i == 2);
        ajwB(AJW_A_TUDO, ajwF(x0 + i * 0.15f + 0.055f, 0.585f, 0.012f, 0.035f), 6, 0.7f);
        ajwB(AJW_A_TUDO, ajwF(x0 + i * 0.15f + 0.03f, 0.65f, 0.06f, 0.025f), 3, 0.45f);
      }
      break;
    case 3:   // Ranking: o numero grande ao lado de cada poster
      for (int i = 0; i < 5; i++) {
        ajwB(AJW_A_TUDO, ajwF(x0 + i * 0.185f, 0.3f, 0.05f, 0.34f), 6, 0.42f);
        ajwPoster(AJW_A_TUDO, ajwF(x0 + i * 0.185f + 0.06f, 0.22f, 0.11f, 0.42f), 5, 0.22f, i == 1);
      }
      break;
    case 4:   // Saga: em ordem, ligados
      for (int i = 0; i < 5; i++) {
        if (i) ajwB(AJW_A_TUDO, ajwF(x0 + i * 0.185f - 0.045f, 0.43f, 0.045f, 0.008f), 2, 0.45f);
        ajwPoster(AJW_A_TUDO, ajwF(x0 + i * 0.185f, 0.22f, 0.14f, 0.42f), 5, 0.22f, i == 1);
        ajwB(AJW_A_TUDO, ajwF(x0 + i * 0.185f + 0.05f, 0.68f, 0.04f, 0.05f), 10, 0.5f);
      }
      break;
    case 5:   // Decadas: grupos com o cabecalho do ano
      for (int g = 0; g < 2; g++) {
        ajwB(AJW_A_TUDO, ajwF(x0 + g * 0.47f, 0.17f, 0.1f, 0.04f), 4, 0.6f);
        for (int i = 0; i < 4; i++) ajwPoster(AJW_A_TUDO, ajwF(x0 + g * 0.47f + i * 0.105f, 0.25f, 0.095f, 0.34f), 5, 0.21f, g == 0 && i == 1);
      }
      break;
    case 6:   // Mural: mosaico denso
      { static const float M[][4] = { {0,0,.2f,.5f}, {.21f,0,.14f,.24f}, {.21f,.26f,.14f,.24f}, {.36f,0,.26f,.5f}, {.63f,0,.13f,.24f},
                                      {.63f,.26f,.13f,.24f}, {.77f,0,.17f,.5f}, {0,.52f,.14f,.22f}, {.15f,.52f,.3f,.22f}, {.46f,.52f,.16f,.22f}, {.63f,.52f,.31f,.22f} };
        for (size_t i = 0; i < sizeof M / sizeof *M; i++)
          ajwPoster(AJW_A_TUDO, ajwF(x0 + M[i][0] * 0.94f, 0.16f + M[i][1], M[i][2] * 0.94f, M[i][3]), 5, 0.18f + 0.03f * (i % 3), i == 3); }
      break;
    default: {   // Vitrine: o destaque com fundo e logo, a fileira embaixo
      ajwB(AJW_A_TUDO, ajwF(0, 0.14f, 1.0f, 0.42f), 0, 0.16f);
      ajwB(AJW_A_TUDO, ajwF(x0, 0.3f, 0.22f, 0.06f), 4, 0.72f);
      ajwB(AJW_A_TUDO, ajwF(x0, 0.39f, 0.32f, 0.02f), 3, 0.42f);
      ajwB(AJW_A_TUDO, ajwF(x0, 0.43f, 0.26f, 0.02f), 3, 0.34f);
      float x = x0;
      for (int i = 0; i < 9 && x < 0.99f; i++) {
        int foco = i == 0;
        float w = foco && expande ? 0.2f : 0.08f;
        GfxRect r = ajwF(x, 0.64f, fminf(w, 0.995f - x), 0.3f);
        int parte = foco ? AJW_A_CARTAZ : AJW_A_FILEIRAS;
        ajwPoster(parte, r, 5, 0.22f + 0.03f * (i % 3), foco);
        if (foco) ajwParte(AJW_A_CARTAZ, r);
        x += w + 0.012f;
      }
      ajwParte(AJW_A_FILEIRAS, ajwF(x0 - 0.01f, 0.62f, 0.94f, 0.34f));
    }
  }
}

static void ajwCenaTitulo(int op, int v) {
  int veu = op == AJ_DET_VEU ? v == 0 : 1;
  (void)veu;
  ajwB(AJW_A_TITULO_TOPO, ajwF(0, 0, 1, 0.58f), 0, 0.17f);
  ajwB(AJW_A_TITULO_TOPO, ajwF(0, 0.38f, 1, 0.2f), 0, 0.1f);
  ajwB(AJW_A_TITULO_TOPO, ajwF(0.06f, 0.16f, 0.3f, 0.09f), 5, 0.72f);
  ajwB(AJW_A_TITULO_TOPO, ajwF(0.06f, 0.29f, 0.36f, 0.025f), 3, 0.45f);
  ajwB(AJW_A_TITULO_TOPO, ajwF(0.06f, 0.335f, 0.44f, 0.022f), 3, 0.35f);
  ajwB(AJW_A_TITULO_TOPO, ajwF(0.06f, 0.37f, 0.38f, 0.022f), 3, 0.35f);
  ajwParte(AJW_A_TITULO_TOPO, ajwF(0.04f, 0.13f, 0.5f, 0.28f));
  { GfxRect b = ajwF(0.06f, 0.44f, 0.15f, 0.075f); ajwRetRGB(b, b.h * 0.5f, AJW_BRANCO, ajwAP(AJW_A_BOTOES));
    for (int i = 0; i < 3; i++) { GfxRect c = ajwF(0.23f + i * 0.06f, 0.44f, 0.045f, 0.075f); ajwB(AJW_A_BOTOES, c, c.h * 0.5f, 0.3f); }
    ajwParte(AJW_A_BOTOES, ajwF(0.06f, 0.44f, 0.35f, 0.075f)); }
  for (int i = 0; i < 4; i++) ajwB(AJW_A_NOTAS, ajwF(0.06f + i * 0.085f, 0.55f, 0.075f, 0.04f), 6, i == 0 ? 0.5f : 0.32f);
  ajwParte(AJW_A_NOTAS, ajwF(0.06f, 0.55f, 0.33f, 0.04f));
  ajwB(AJW_A_EPISODIOS, ajwF(0.06f, 0.64f, 0.12f, 0.025f), 3, 0.45f);
  for (int i = 0; i < 4; i++) ajwB(AJW_A_EPISODIOS, ajwF(0.06f + i * 0.225f, 0.69f, 0.21f, 0.13f), 5, 0.22f);
  ajwParte(AJW_A_EPISODIOS, ajwF(0.06f, 0.64f, 0.885f, 0.18f));
  for (int i = 0; i < 7; i++) { GfxRect c = ajwF(0.06f + i * 0.08f, 0.86f, 0.055f, 0.1f); ajwB(AJW_A_ELENCO, c, c.h * 0.5f, 0.26f); }
  ajwParte(AJW_A_ELENCO, ajwF(0.06f, 0.86f, 0.535f, 0.1f));
}

static void ajwCenaPlayer(int op, int v) {
  int alvo = ajwAlvo;
  // A imagem: um quadro de filme esquematico.
  ajwB(AJW_A_VIDEO, ajwF(0, 0, 1, 1), 0, 0.15f);
  ajwB(AJW_A_VIDEO, ajwF(0.55f, 0.18f, 0.3f, 0.52f), 40, 0.22f);
  ajwB(AJW_A_VIDEO, ajwF(0.1f, 0.5f, 0.5f, 0.3f), 30, 0.2f);
  ajwParte(AJW_A_VIDEO, ajwF(0.0f, 0.0f, 1.0f, 1.0f));
  // Informacoes no alto (titulo, formato, relogio).
  ajwB(AJW_A_INFO, ajwF(0.05f, 0.06f, 0.24f, 0.04f), 4, 0.6f);
  ajwB(AJW_A_INFO, ajwF(0.05f, 0.12f, 0.16f, 0.025f), 3, 0.4f);
  ajwB(AJW_A_INFO, ajwF(0.85f, 0.06f, 0.1f, 0.04f), 4, 0.5f);
  if (op == AJ_ESTAT_TAMANHO || op == AJ_ESTAT_LADO) {
    float x = (op == AJ_ESTAT_LADO && v == 1) ? 0.05f : 0.7f;
    ajwB(AJW_A_INFO, ajwF(x, 0.2f, 0.25f, 0.2f), 8, 0.1f);
    for (int i = 0; i < 4; i++) ajwB(AJW_A_INFO, ajwF(x + 0.02f, 0.23f + i * 0.04f, 0.18f, 0.018f), 2, 0.45f);
    ajwParte(AJW_A_INFO, ajwF(x, 0.2f, 0.25f, 0.2f));
  } else ajwParte(AJW_A_INFO, ajwF(0.04f, 0.05f, 0.92f, 0.1f));
  // Barra de tempo e miniatura do avanco.
  ajwB(AJW_A_BARRA, ajwF(0.05f, 0.9f, 0.9f, 0.012f), 3, 0.32f);
  { float ar, ag, ab; ajwAcento(&ar, &ag, &ab);
    ajwRetRGB(ajwF(0.05f, 0.9f, 0.38f, 0.012f), 3, ar, ag, ab, ajwAP(AJW_A_BARRA)); }
  if (alvo == AJW_A_BARRA) { ajwB(AJW_A_BARRA, ajwF(0.36f, 0.68f, 0.15f, 0.17f), 6, 0.35f); ajwParte(AJW_A_BARRA, ajwF(0.05f, 0.68f, 0.9f, 0.24f)); }
  else ajwParte(AJW_A_BARRA, ajwF(0.05f, 0.88f, 0.9f, 0.04f));
  // Legendas: a principal embaixo, a segunda no alto.
  if (alvo == AJW_A_LEGENDA || alvo == AJW_A_LEGENDA2 || op == AJ_LEG_MOSTRAR) {
    float esc = 1.0f, pos = 0.0f;
    if (op == AJ_LEG1_TAMANHO || op == AJ_LEG2_TAMANHO) esc = (float)v / 100.0f;
    if (op == AJ_LEG1_POSICAO || op == AJ_LEG2_POSICAO) pos = (float)v * 0.04f;
    esc = fmaxf(0.5f, fminf(2.2f, esc));
    { float lw1 = 0.46f * esc, lw2 = 0.34f * esc, lh = 0.035f * esc, yb = 0.82f - pos - lh * 2.4f;
      ajwB(AJW_A_LEGENDA, ajwF(0.5f - lw1 * 0.5f, yb, lw1, lh), 3, 0.86f);
      ajwB(AJW_A_LEGENDA, ajwF(0.5f - lw2 * 0.5f, yb + lh * 1.35f, lw2, lh), 3, 0.86f);
      ajwParte(AJW_A_LEGENDA, ajwF(0.5f - lw1 * 0.5f - 0.02f, yb - 0.02f, lw1 + 0.04f, lh * 2.35f + 0.04f)); }
    if (alvo == AJW_A_LEGENDA2) {
      ajwB(AJW_A_LEGENDA2, ajwF(0.32f, 0.18f, 0.36f, 0.03f), 3, 0.7f);
      ajwParte(AJW_A_LEGENDA2, ajwF(0.3f, 0.16f, 0.4f, 0.07f));
    }
  }
  if (alvo == AJW_A_PULAR) {
    GfxRect r = ajwF(0.74f, 0.74f, 0.21f, 0.085f);
    ajwRetRGB(r, r.h * 0.5f, AJW_BRANCO, ajwAP(AJW_A_PULAR));
    ajwParte(AJW_A_PULAR, r);
  }
  if (alvo == AJW_A_PROXIMO) {
    GfxRect r = ajwF(0.62f, 0.56f, 0.33f, 0.28f);
    ajwB(AJW_A_PROXIMO, r, 10, 0.1f);
    ajwB(AJW_A_PROXIMO, ajwF(0.64f, 0.59f, 0.12f, 0.13f), 5, 0.3f);
    ajwB(AJW_A_PROXIMO, ajwF(0.78f, 0.6f, 0.14f, 0.03f), 3, 0.6f);
    ajwB(AJW_A_PROXIMO, ajwF(0.78f, 0.65f, 0.1f, 0.02f), 3, 0.4f);
    ajwRetRGB(ajwF(0.64f, 0.76f, 0.29f, 0.05f), 8, AJW_BRANCO, ajwAP(AJW_A_PROXIMO));
    ajwParte(AJW_A_PROXIMO, r);
  }
  if (alvo == AJW_A_PAUSA) {
    GfxRect r = ajwF(0, 0, 1, 1);
    ajwRet(r, 0, 0.0f, 0.45f * ajwAP(AJW_A_PAUSA));
    gfx_icone(ajwF(0.46f, 0.38f, 0.08f, 0.14f), "aj_pause", AJW_BRANCO, ajwAP(AJW_A_PAUSA));
    ajwB(AJW_A_PAUSA, ajwF(0.05f, 0.62f, 0.4f, 0.05f), 4, 0.7f);
    ajwB(AJW_A_PAUSA, ajwF(0.05f, 0.7f, 0.5f, 0.025f), 3, 0.45f);
    ajwParte(AJW_A_PAUSA, ajwF(0.03f, 0.35f, 0.94f, 0.42f));
  }
}

static void ajwCenaFontes(void) {
  ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.12f);
  ajwB(AJW_A_TUDO, ajwF(0.05f, 0.06f, 0.26f, 0.05f), 4, 0.62f);
  for (int i = 0; i < 5; i++) {
    float y = 0.17f + i * 0.155f;
    int foco = i == 0;
    GfxRect r = ajwF(0.05f, y, 0.9f, 0.13f);
    if (foco) ajwRetRGB(r, 8, AJW_BRANCO, ajwAP(AJW_A_LISTA) * 0.95f);
    else ajwB(AJW_A_LISTA, r, 8, 0.17f);
    ajwB(AJW_A_SELOS, ajwF(0.07f, y + 0.035f, 0.07f, 0.06f), 5, foco ? 0.3f : 0.5f);
    ajwB(AJW_A_SELOS, ajwF(0.15f, y + 0.035f, 0.05f, 0.06f), 5, foco ? 0.4f : 0.36f);
    ajwB(AJW_A_LISTA, ajwF(0.23f, y + 0.035f, 0.42f, 0.025f), 3, foco ? 0.15f : 0.5f);
    ajwB(AJW_A_LISTA, ajwF(0.23f, y + 0.075f, 0.28f, 0.02f), 3, foco ? 0.35f : 0.34f);
    ajwB(AJW_A_LISTA, ajwF(0.83f, y + 0.045f, 0.09f, 0.03f), 3, foco ? 0.25f : 0.42f);
  }
  ajwParte(AJW_A_LISTA, ajwF(0.04f, 0.16f, 0.92f, 0.79f));
  ajwParte(AJW_A_SELOS, ajwF(0.06f, 0.18f, 0.16f, 0.75f));
}

static void ajwCenaTvVivo(void) {
  ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.1f);
  { GfxRect vr = ajwF(0.62f, 0.05f, 0.33f, 0.3f); ajwB(AJW_A_VIDEO, vr, 8, 0.24f); ajwParte(AJW_A_VIDEO, vr); }
  ajwB(AJW_A_TUDO, ajwF(0.05f, 0.08f, 0.3f, 0.05f), 4, 0.6f);
  ajwB(AJW_A_TUDO, ajwF(0.05f, 0.16f, 0.4f, 0.025f), 3, 0.38f);
  for (int t = 0; t < 4; t++) ajwB(AJW_A_GRADE, ajwF(0.22f + t * 0.19f, 0.41f, 0.06f, 0.02f), 3, 0.42f);
  for (int c = 0; c < 5; c++) {
    float y = 0.46f + c * 0.105f;
    ajwB(AJW_A_CANAL, ajwF(0.05f, y, 0.14f, 0.085f), 6, c == 1 ? 0.45f : 0.24f);
    float x = 0.21f;
    for (int p = 0; p < 4 && x < 0.95f; p++) {
      float w = 0.12f + 0.07f * ((c + p) % 3);
      if (x + w > 0.95f) w = 0.95f - x;
      ajwB(AJW_A_GRADE, ajwF(x, y, w - 0.008f, 0.085f), 5, c == 1 && p == 0 ? 0.42f : 0.18f);
      x += w;
    }
  }
  ajwParte(AJW_A_CANAL, ajwF(0.04f, 0.45f, 0.16f, 0.54f));
  ajwParte(AJW_A_GRADE, ajwF(0.2f, 0.39f, 0.76f, 0.6f));
}

static void ajwCenaAparencia(int op, int v) {
  float ar, ag, ab;
  ajwAcento(&ar, &ag, &ab);
  if (ajwAlvo == AJW_A_TEXTO) {
    float esc = 1.0f;
    if (op == AJ_ESCALA_TEXTO) { static const float E[] = { .85f, .9f, 1, 1.1f, 1.2f, 1.3f }; if (v >= 0 && v < 6) esc = E[v]; }
    if (op == AJ_TAMANHO_UI || op == AJ_TAMANHO_AJUSTES) esc = 0.85f + 0.1f * (float)v;
    ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.1f);
    { TxtLinha l = txt_linha(TXT_TITULO2, "Aa", 243, 242, 239, 255);
      float s = esc * ajwRc.h * 0.32f / (float)(l.h > 0 ? l.h : 1);
      gfx_transformar(ajwRc.x + ajwRc.w * 0.08f, ajwRc.y + ajwRc.h * 0.18f, s, 0, 0);
      txt_desenhar_alpha(l, ajwRc.x + ajwRc.w * 0.08f, ajwRc.y + ajwRc.h * 0.18f, ajwAP(AJW_A_TEXTO));
      gfx_sem_transformar(); }
    for (int i = 0; i < 4; i++) ajwB(AJW_A_TEXTO, ajwF(0.08f, 0.62f + i * 0.08f * esc, (0.8f - i * 0.12f), 0.035f * esc), 3, 0.5f);
    ajwParte(AJW_A_TEXTO, ajwF(0.06f, 0.14f, 0.88f, 0.8f));
    return;
  }
  if (ajwAlvo == AJW_A_LOGO) {
    ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.06f);
    GfxRect r = ajwF(0.42f, 0.28f, 0.16f, 0.284f);
    ajwRetRGB(r, r.h * 0.22f, ar, ag, ab, ajwAP(AJW_A_LOGO));
    ajwB(AJW_A_LOGO, ajwF(0.38f, 0.66f, 0.24f, 0.05f), 4, 0.62f);
    ajwParte(AJW_A_LOGO, ajwF(0.36f, 0.24f, 0.28f, 0.5f));
    return;
  }
  // Cores: a Home com o acento aplicado nos pontos que ele pinta.
  ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, op == AJ_TEMA_FUNDO ? 0.02f + 0.04f * (float)(v % 4) : 0.08f);
  ajwB(AJW_A_TUDO, ajwF(0, 0, 0.06f, 1), 0, 0.12f);
  ajwRetRGB(ajwF(0.015f, 0.16f, 0.03f, 0.05f), 5, ar, ag, ab, ajwAP(AJW_A_CORES));
  ajwB(AJW_A_TUDO, ajwF(0.1f, 0.06f, 0.86f, 0.38f), 8, 0.16f);
  ajwRetRGB(ajwF(0.13f, 0.32f, 0.14f, 0.07f), 10, ar, ag, ab, ajwAP(AJW_A_CORES));
  for (int i = 0; i < 7; i++) {
    GfxRect r = ajwF(0.1f + i * 0.125f, 0.52f, 0.11f, 0.36f);
    ajwB(AJW_A_TUDO, r, 6, 0.2f);
    if (i == 1) gfx_anel_fora(r, 6.0f / r.h, 3, 4, ar, ag, ab, ajwAP(AJW_A_CORES));
  }
  ajwRetRGB(ajwF(0.1f, 0.92f, 0.3f, 0.012f), 3, ar, ag, ab, ajwAP(AJW_A_CORES));
  ajwParte(AJW_A_CORES, ajwF(0.0f, 0.0f, 1.0f, 1.0f));
}

static void ajwCenaIconeGrande(const char *icone, int parte) {
  GfxRect r = ajwF(0.38f, 0.22f, 0.24f, 0.427f);
  ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.08f);
  ajwB(parte, ajwF(0.36f, 0.18f, 0.28f, 0.5f), 24, 0.14f);
  gfx_icone(r, icone, AJW_BRANCO, ajwAP(parte) * 0.9f);
}
static void ajwCenaPerfis(void) {
  ajwB(AJW_A_TUDO, ajwF(0, 0, 1, 1), 0, 0.08f);
  ajwB(AJW_A_TUDO, ajwF(0.36f, 0.14f, 0.28f, 0.05f), 4, 0.6f);
  for (int i = 0; i < 4; i++) {
    GfxRect c = ajwF(0.17f + i * 0.18f, 0.32f, 0.12f, 0.213f);
    ajwB(AJW_A_TUDO, c, c.h * 0.5f, i == 1 ? 0.42f : 0.26f);
    if (i == 1) gfx_anel_fora(c, 0.5f, 4, 3, AJW_BRANCO, ajwCA);
    ajwB(AJW_A_TUDO, ajwF(0.17f + i * 0.18f + 0.015f, 0.58f, 0.09f, 0.03f), 3, 0.45f);
  }
}
static void ajwCenaDescanso(void) {
  ajwRet(ajwF(0, 0, 1, 1), 0, 0.0f, ajwCA);
  ajwB(AJW_A_TUDO, ajwF(0.55f, 0.1f, 0.4f, 0.6f), 30, 0.07f);
  TxtLinha l = txt_linha(TXT_TITULO1, "20:41", 243, 242, 239, 255);
  txt_desenhar_alpha(l, ajwRc.x + ajwRc.w * 0.08f, ajwRc.y + ajwRc.h * 0.62f - l.h, 0.85f * ajwCA);
  ajwB(AJW_A_TUDO, ajwF(0.08f, 0.68f, 0.22f, 0.03f), 3, 0.35f);
}

static void ajwCenaDesenhar(int cena, int alvo, int op, int v, GfxRect R, float a) {
  float ar, ag, ab;
  ajwRc = R; ajwAlvo = alvo; ajwCA = a; ajwAlvoTem = 0;
  // Interruptor desligado: a parte que ele liga fica so esbocada. As opcoes que
  // mudam a propria miniatura (destaque, Continuar assistindo...) ja mostram.
  ajwDesl = ehInterruptor(op) && v == 1 && op != AJ_HERO && op != AJ_CW_LIGADO && op != AJ_LANDSCAPE &&
            op != AJ_RELOGIO && op != AJ_EXPANDIR && op != AJ_COL_EXPANDIR && op != AJ_COL_ENTRADA;
  ajwRet(R, 18, 0.07f, a);
  gfx_recorte(R.x, R.y, R.w, R.h);
  switch (cena) {
    case AJW_C_INICIO: ajwCenaInicio(op, v); break;
    case AJW_C_COLECOES: ajwCenaColecoes(op, v); break;
    case AJW_C_COLECAO: ajwCenaColecao(op, v); break;
    case AJW_C_TITULO: ajwCenaTitulo(op, v); break;
    case AJW_C_PLAYER: ajwCenaPlayer(op, v); break;
    case AJW_C_FONTES: ajwCenaFontes(); break;
    case AJW_C_TVVIVO: ajwCenaTvVivo(); break;
    case AJW_C_APARENCIA: ajwCenaAparencia(op, v); break;
    case AJW_C_PERFIS: ajwCenaPerfis(); break;
    case AJW_C_CONTA: ajwCenaIconeGrande("aj_user-round", AJW_A_TUDO); break;
    case AJW_C_SISTEMA: ajwCenaIconeGrande("aj_monitor-cog", AJW_A_TUDO); break;
    case AJW_C_SOBRE: ajwCenaIconeGrande("aj_info", AJW_A_TUDO); break;
    case AJW_C_ADDONS: ajwCenaIconeGrande("aj_puzzle", AJW_A_TUDO); break;
    case AJW_C_DESCANSO: ajwCenaDescanso(); break;
  }
  gfx_sem_recorte();
  if (ajwAlvoTem) {   // o contorno no acento em volta do que muda
    GfxRect c = ajwAlvoR;
    float m = 6.0f;
    c.x -= m; c.y -= m; c.w += 2 * m; c.h += 2 * m;
    if (c.x < R.x + 3) { c.w -= R.x + 3 - c.x; c.x = R.x + 3; }
    if (c.y < R.y + 3) { c.h -= R.y + 3 - c.y; c.y = R.y + 3; }
    if (c.x + c.w > R.x + R.w - 3) c.w = R.x + R.w - 3 - c.x;
    if (c.y + c.h > R.y + R.h - 3) c.h = R.y + R.h - 3 - c.y;
    ajwAcento(&ar, &ag, &ab);
    if (ajwDesl) gfx_anel(c, 10.0f / c.h, 3, 0.55f, 0.55f, 0.56f, 0.8f * a);
    else gfx_anel(c, 10.0f / c.h, 4, ar, ag, ab, a);
  }
  gfx_anel(R, 18.0f / R.h, 1.5f, 0.165f, 0.165f, 0.17f, a);
}

// Sem opcao (abas, botoes do alto, "Mais opcoes"): um icone grande.
static void ajwCenaIcone(GfxRect R, const char *icone, float a) {
  ajwRc = R; ajwAlvo = AJW_A_TUDO; ajwCA = a; ajwAlvoTem = 0; ajwDesl = 0;
  ajwRet(R, 18, 0.07f, a);
  ajwCenaIconeGrande(icone, AJW_A_TUDO);
  gfx_anel(R, 18.0f / R.h, 1.5f, 0.165f, 0.165f, 0.17f, a);
}
// A categoria em foco nas abas: a cena que melhor a representa.
static void ajwCenaSecao(int s, GfxRect R, float a) {
  static const struct { const char *t; int cena; } C[] = {
    { "Reprodução", AJW_C_PLAYER }, { "Idiomas e legendas", AJW_C_PLAYER }, { "Fontes e addons", AJW_C_FONTES },
    { "TV ao vivo", AJW_C_TVVIVO }, { "Tela inicial", AJW_C_INICIO }, { "Coleções", AJW_C_COLECOES },
    { "Aparência", AJW_C_APARENCIA }, { "Página do título", AJW_C_TITULO }, { "Cartazes e trailers", AJW_C_INICIO },
    { "Conta e serviços", AJW_C_CONTA }, { "Esta TV", AJW_C_SISTEMA }, { "Sobre e ajuda", AJW_C_SOBRE },
  };
  int cena = AJW_C_INICIO;
  for (size_t i = 0; i < sizeof C / sizeof *C; i++) if (ajwSecaoChamada(s, C[i].t)) cena = C[i].cena;
  if (ajwSecaoChamada(s, "Idiomas e legendas")) { ajwCenaDesenhar(AJW_C_PLAYER, AJW_A_LEGENDA, -1, 0, R, a); return; }
  ajwCenaDesenhar(cena, AJW_A_TUDO, -1, 0, R, a);
}
