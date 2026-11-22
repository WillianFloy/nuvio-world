// AJUSTES WORLD — a tela de Ajustes da TV no Nuvio World (21/11, layout
// aprovado pelo dono no quadro "Definitivo", base B):
//
//   Ajustes                         [Buscar ajuste] [Alterados · N] [Guia]
//   Reprodução  Idiomas  Fontes  …  Coleções  …  Esta TV  Sobre      (abas)
//   ┌ lista da categoria ─────────────┐   ┌ prévia ─────────────────┐
//   │ GRUPO                           │   │ cena esquemática da tela │
//   │ Opção               valor       │   │ onde a opção age, com a  │
//   │▐Opção em foco  Esta TV  ‹ v › ▌ │   │ parte que muda destacada │
//   │ Mais opções                 3 › │   └──────────────────────────┘
//   └─────────────────────────────────┘   onde muda · explicação
//   ‹ › muda o valor · OK lista de valores · Segurar OK volta ao padrão · Voltar sai
//
// Um menu so: o Painel e a Lista (AJ_LAYOUT_AJUSTES) nao valem aqui. Os dados,
// a gravacao, os avisos de risco, as dependencias, a busca local e as folhas
// (fileiras, ordens, vinculos, teclado) sao os de sempre: esta camada so troca
// o desenho e a navegacao. As prévias sao desenho novo (ajustes_ux_world_cenas.inc),
// nao as cenas do Painel.
//
// Regras puras (grade, segurar OK, rascunho das setas, rolagem): ajwmodelo.h,
// com teste de comportamento em tests/ajustes_world_modelo.c.

#include "ajwmodelo.h"

static int ajwAtivo(void) {
#ifdef NV_AJUSTES_CHEIA
  return !ajRetrato() && !telefoneui_ativo();
#else
  return 0;
#endif
}

// --- ESTADO ----------------------------------------------------------------------
enum { AJW_M_CATEGORIA = 0, AJW_M_BUSCA, AJW_M_ALTERADOS };
enum { AJW_T_BUSCA = 0, AJW_T_ALTERADOS, AJW_T_GUIA, AJW_T_N };
static int ajwModo;            // o que a lista mostra
static int ajwTopo = -1;       // botao do alto em foco (com focoIndice); -1 = abas
static AjwRascunho ajwRasc = { -1, 0, 0 };
static AjwSegurar ajwOk;
static int ajwOkReentra;       // o OK curto, reenviado ao soltar, nao arma de novo
static char ajwBusca[96];
static int ajwBuscaPend;       // o teclado da busca esta aberto
static AjusteBuscaResultado ajwRes[48];
static int ajwNRes, ajwResFoco;
static Uint32 ajwRestaurouEm;  // marca do "voltou ao padrao" na linha
// Animacao (molas leves, ajwAtualizar).
static float ajwRol, ajwRolAlvo;           // rolagem da lista
static float ajwAbaX, ajwAbaW, ajwAbasDesloc, ajwAbasDeslocAlvo;
static float ajwFocoY = -1, ajwFocoH;      // a faixa branca da linha em foco
static float ajwGradeRol, ajwGradeRolAlvo;
static int ajwUltimoOp = -2;
static float ajwPreviaA = 1.0f;            // troca de prévia: aparece rapido

static void ajwReiniciar(void) {
  ajwModo = AJW_M_CATEGORIA; ajwTopo = -1;
  ajw_rasc_limpar(&ajwRasc); ajw_ok_soltar(&ajwOk); ajwOkReentra = 0;
  ajwBuscaPend = 0; ajwNRes = ajwResFoco = 0;
  ajwRol = ajwRolAlvo = 0; ajwFocoY = -1; ajwGradeRol = ajwGradeRolAlvo = 0;
  ajwUltimoOp = -2; ajwPreviaA = 1.0f;
}

// --- GEOMETRIA (1920x1080 a 100%; o "Tamanho dos Ajustes" escala tudo) -------------
#define AJW_MARGEM      72.0f
#define AJW_TOPO        54.0f
#define AJW_CAB_H       66.0f
#define AJW_ABAS_Y     (AJW_TOPO + AJW_CAB_H + 28.0f)
#define AJW_ABAS_H      58.0f
#define AJW_CORPO_Y    (AJW_ABAS_Y + AJW_ABAS_H + 30.0f)
#define AJW_RODAPE_H    40.0f
#define AJW_LN_H        78.0f
#define AJW_LN_FOCO_H   92.0f
#define AJW_GRP_H       46.0f
#define AJW_RES_H      108.0f
#define AJW_VAO_COL     66.0f
static float ajwX0(void) { return (AJW_MARGEM + ajustes_rail_largura_fixa()) / ajustes_tamanho_ajustes(); }
static float ajwX1(void) { return NV_VTELA_W - AJW_MARGEM; }
static float ajwListaW(void) { float w = (ajwX1() - ajwX0() - AJW_VAO_COL) * 0.53f; return w > 1040.0f ? 1040.0f : w; }
static float ajwPreviaX(void) { return ajwX0() + ajwListaW() + AJW_VAO_COL; }
static float ajwPreviaW(void) { return ajwX1() - ajwPreviaX(); }
static float ajwCorpoBase(void) { return NV_VTELA_H - AJW_TOPO - AJW_RODAPE_H - 26.0f; }

// --- CORES ----------------------------------------------------------------------------
#define AJW_BRANCO 0.953f, 0.949f, 0.937f
static void ajwAcento(float *r, float *g, float *b) { ajustes_acento(r, g, b); }
static void ajwRet(GfxRect r, float raioPx, float c, float a) {
  if (r.w <= 0 || r.h <= 0 || a <= 0.001f) return;
  gfx_cor(r, r.h > 0 ? raioPx / r.h : 0, c, c, c * 1.02f, a);
}
static void ajwRetRGB(GfxRect r, float raioPx, float cr, float cg, float cb, float a) {
  if (r.w <= 0 || r.h <= 0 || a <= 0.001f) return;
  gfx_cor(r, r.h > 0 ? raioPx / r.h : 0, cr, cg, cb, a);
}
static TxtLinha ajwTxt(TxtEstilo e, const char *s, int cinza, float maxW) {
  return maxW > 0 ? txt_linha_corta(e, s, cinza, cinza, cinza, 255, maxW) : txt_linha(e, s, cinza, cinza, cinza, 255);
}
static TxtLinha ajwTxtB(TxtEstilo e, const char *s, float maxW) {   // claro (branco da UI)
  return maxW > 0 ? txt_linha_corta(e, s, 243, 242, 239, 255, maxW) : txt_linha(e, s, 243, 242, 239, 255);
}
static TxtLinha ajwTxtP(TxtEstilo e, const char *s, float maxW) {   // escuro, sobre o branco do foco
  return maxW > 0 ? txt_linha_corta(e, s, 12, 12, 14, 255, maxW) : txt_linha(e, s, 12, 12, 14, 255);
}

// --- NOMES CURTOS DAS ABAS ---------------------------------------------------------------
static const char *ajwAbaNome(int s) {
  const char *t = (s >= 0 && s < nSecoes) ? TELA[secIni[s]].titulo : "";
  static const char *const CURTO[][2] = {
    { "Idiomas e legendas", "Idiomas" }, { "Fontes e addons", "Fontes" },
    { "Página do título", "Título" }, { "Cartazes e trailers", "Cartazes" },
    { "Conta e serviços", "Conta" }, { "Sobre e ajuda", "Sobre" },
  };
  for (size_t i = 0; i < sizeof CURTO / sizeof *CURTO; i++) if (!strcmp(t, CURTO[i][0])) return CURTO[i][1];
  return t;
}

// --- O QUE CADA LINHA FAZ COM AS SETAS ------------------------------------------------
// Valor que muda na propria linha com ‹ ›: interruptores, escolhas e numeros comuns.
// O resto abre a pagina de valores (ou age, se e acao).
static int ajwPagina(int op) {   // opcoes que so se escolhem na pagina
  return op == AJ_GUIA_CATEGORIAS || ajIptvGerenciador(op) || op == AJ_IPTV_ACOES || op == AJ_IPTV_EXCLUIR ||
         op == AJ_IPTV_CONTA || op == AJ_TEX_MB || op == AJ_SALVOS_DEST || ajPaleta(op) || op == AJ_FIL_LIMITE ||
         op == AJ_ICONE_APP || op == AJ_LOGO_APP || op == AJ_LAYOUT_AJUSTES;
}
static int ajwNaLinha(int op) {
  if (op < 0 || op >= AJ_N || soLeitura(op) || OPCOES[op].tipo == OP_ACAO || ajwPagina(op)) return 0;
  return OPCOES[op].tipo == OP_ESCOLHA || OPCOES[op].tipo == OP_NUMERO;
}
static int ajwValorLinha(int op) { return ajwRasc.op == op ? ajwRasc.v : uxValorAtual(op); }

// O rascunho das setas vira gravacao: pelo caminho de sempre (avisos de risco,
// "mudou em outra tela", falha). Sem pergunta pendente, o seletor nao fica aberto.
static void ajwRascGravar(void) {
  int op = ajwRasc.op, v = ajwRasc.v;
  if (op < 0) return;
  ajw_rasc_limpar(&ajwRasc);
  if (v == uxValorAtual(op)) return;
  uxAbrirEditor(op);
  if (!uxEditor || uxOp != op) return;
  if (uxEditor == 2) { uxCancelar(); return; }
  uxPendente = v;
  uxAplicar();
  if (uxEditor && !uxAvisoRisco && !uxRestaurar) uxCancelar();
}
static int ajwLateral(int d) {
  int op = focoOp, lo, hi, passo = 1;
  if (op < 0 || inativa(op) || soLeitura(op)) return 1;
  if (ajwRasc.op >= 0 && ajwRasc.op != op) ajwRascGravar();
  if (ehInterruptor(op)) { uxAlternarDireto(op); return 1; }
  if (!ajwNaLinha(op)) return 1;
  if (OPCOES[op].tipo == OP_NUMERO) { lo = OPCOES[op].min; hi = OPCOES[op].max; passo = OPCOES[op].passo > 0 ? OPCOES[op].passo : 1; }
  else { lo = 0; hi = nValores(op) - 1; }
  ajw_rasc_passo(&ajwRasc, op, uxValorAtual(op), lo, hi, d * passo, SDL_GetTicks());
  if (OPCOES[op].tipo == OP_NUMERO && ajwRasc.op == op) ajwRasc.v = limita(op, ajwRasc.v);
  return 1;
}

// Segurar OK: o valor volta ao padrao (o mesmo caminho do "Restaurar padrao").
static int ajwPodeRestaurar(int op) {
  return op >= 0 && op < AJ_N && !inativa(op) && !soLeitura(op) && uxTemPadrao(op) && uxDiferente(op) &&
         op != AJ_GUIA_CATEGORIAS && !ajIptvGerenciador(op);
}
static void ajwRestaurar(int op) {
  if (!ajwPodeRestaurar(op)) return;
  ajw_rasc_limpar(&ajwRasc);
  uxAbrirEditor(op);
  if (uxEditor != 1 || uxOp != op) { uxCancelar(); return; }
  uxPendente = uxValorPadrao(op);
  uxAplicar();
  if (uxEditor && !uxAvisoRisco && !uxRestaurar) uxCancelar();
  else if (!uxEditor) { uxNotificar(i18n("Voltou ao padrão.")); ajwRestaurouEm = SDL_GetTicks(); }
}

// --- AS LINHAS DE CADA MODO ------------------------------------------------------------
static int ajwOpDaLinha(void) {
  if (ajwModo == AJW_M_BUSCA) return ajwNRes ? ajwRes[ajwResFoco].op : -1;
  if (ajwModo == AJW_M_ALTERADOS) return uxNDifs ? uxDifs[uxDifFoco] : -1;
  return focoOp;
}
// Nos modos de busca e de alterados a linha em foco e uma opcao de qualquer
// categoria: o foco de verdade (focoItem/focoOp) vai para ela, para o OK das
// acoes e as perguntas cairem no lugar certo.
static void ajwSincronizarFoco(void) {
  int op = ajwOpDaLinha();
  if (ajwModo != AJW_M_CATEGORIA && op >= 0 && op != focoOp) { focarOpcao(op); focoIndice = 0; }
}

static void ajwBuscar(void) {
  ajwNRes = ajwBusca[0] ? ajustes_buscar(ajwBusca, ajwRes, (int)(sizeof ajwRes / sizeof *ajwRes)) : 0;
  if (ajwNRes < 0) ajwNRes = 0;
  ajwResFoco = 0; ajwRolAlvo = 0;
}
static void ajwBuscaAbrir(void) {
  ajwBuscaPend = 1; stCampo = 0;
  teclado_contexto(i18n("Ajustes"));
  teclado_abrir_com("Buscar ajuste", "Nome do ajuste ou uma palavra", 60, NULL, ajwBusca[0] ? ajwBusca : NULL);
  teclado_aceitar_utf8(1);
  teclado_permitir_vazio(1);
}
// Chamado por ajustes_atualizar antes da leitura geral do teclado (que trataria
// um campo sem dono como o portal do Stalker).
static void ajwTecladoLer(void) {
  int r;
  if (!ajwBuscaPend || teclado_aberto()) return;
  r = teclado_resultado();
  if (r == TECLADO_NADA) return;
  ajwBuscaPend = 0;
  if (r != TECLADO_PRONTO) return;
  snprintf(ajwBusca, sizeof ajwBusca, "%s", teclado_texto());
  if (!ajwBusca[0]) { ajwModo = AJW_M_CATEGORIA; ajwTopo = AJW_T_BUSCA; focoIndice = 1; return; }
  ajwBuscar();
  ajwModo = AJW_M_BUSCA; ajwTopo = -1;
  focoIndice = ajwNRes ? 0 : 1;
  if (!ajwNRes) ajwTopo = AJW_T_BUSCA;
  ajwSincronizarFoco();
}

// --- TECLAS ---------------------------------------------------------------------------------
static int ajwSaiDaLista(SDL_Keycode k) {   // Voltar na lista
  (void)k;
  if (uxVeioBusca && uxRetornarOp < 0) { uxPediuBusca = 2; uxVeioBusca = 0; return 1; }
  if (uxRetornarOp >= 0) { int op = uxRetornarOp; uxRetornarOp = -1; focarOpcao(op); return 1; }
  if (ajwModo == AJW_M_BUSCA) { ajwModo = AJW_M_CATEGORIA; focoIndice = 1; ajwTopo = AJW_T_BUSCA; return 1; }
  if (ajwModo == AJW_M_ALTERADOS) { ajwModo = AJW_M_CATEGORIA; focoIndice = 1; ajwTopo = AJW_T_ALTERADOS; return 1; }
  focoIndice = 1; ajwTopo = -1; sairArmado = 0;
  return 1;
}
// OK curto numa linha. 0 = acao comum (o despacho de sempre em eventoTela).
static int ajwOkLinha(void) {
  int op;
  ajwSincronizarFoco();
  if (ajwModo == AJW_M_BUSCA) {   // "OK abre na categoria"
    op = ajwOpDaLinha();
    if (op < 0) return 1;
    ajwModo = AJW_M_CATEGORIA; focarOpcao(op); focoIndice = 0;
    return 1;
  }
  op = focoOp;
  if (op < 0) {
    if (focoItem >= 0 && focoItem < AJ_N_TELA && TELA[focoItem].tipo == IT_MAIS) maisAberto[secAtual] = !maisAberto[secAtual];
    return 1;
  }
  if (inativa(op)) {
    int req = uxRequisito(op);
    if (uxLigarRequisitos(op, 1)) return 1;
    if (req >= 0) { uxCancelar(); ajwModo = AJW_M_CATEGORIA; focarOpcao(req); uxRetornarOp = op; return 1; }
    uxAbrirEditor(op);
    return 1;
  }
  if (soLeitura(op)) return 1;
  if (uxAlternarDireto(op)) return 1;
  if (OPCOES[op].tipo != OP_ACAO) { uxAbrirEditor(op); ajwGradeRol = ajwGradeRolAlvo = 0; return 1; }
  return 0;
}

// A pagina de valores (uxEditor == 1, fora das perguntas).
static int ajwValorOk(int op, int v) {
  if (op == AJ_TEX_MB) return !texAcima(v);
  if (op == AJ_SALVOS_DEST) return ajustes_fonte_oferecida(v, trakt_ativo(), simkl_ativo());
  return 1;
}
static int ajwEventoPagina(SDL_Keycode k) {
  int op = uxOp, n = nValores(op);
  if (uxRestaurar || uxAvisoRisco || uxEditor == 2 || op == AJ_GUIA_CATEGORIAS) return uxEvento(k);
  if (uxVoltar(k)) { ajIptvVoltar(); return 1; }
  if (uxOk(k)) { uxAplicar(); return 1; }
  if (ajPaleta(op)) {
    if (k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN)
      uxPendente = ajReguaMover(op, uxPendente, k == SDLK_RIGHT ? 1 : k == SDLK_LEFT ? -1 : 0,
                                k == SDLK_DOWN ? 1 : k == SDLK_UP ? -1 : 0);
    return 1;
  }
  if (OPCOES[op].tipo == OP_NUMERO) {
    int d = (k == SDLK_RIGHT || k == SDLK_UP) ? 1 : (k == SDLK_LEFT || k == SDLK_DOWN) ? -1 : 0;
    if (!d) return 1;
    uxPendente = op == AJ_FIL_LIMITE ? fil_limite_passo(uxPendente, d)
                                     : limita(op, uxPendente + d * (OPCOES[op].passo > 0 ? OPCOES[op].passo : 1));
    return 1;
  }
  { unsigned char ok[512];
    int cols = ajw_grade_colunas(n), m = n < (int)sizeof ok ? n : (int)sizeof ok;
    for (int i = 0; i < m; i++) ok[i] = (unsigned char)ajwValorOk(op, i);
    if (k == SDLK_LEFT || k == SDLK_RIGHT) uxPendente = ajw_grade_mover(uxPendente, m, cols, k == SDLK_RIGHT ? 1 : -1, 0, ok);
    else if (k == SDLK_UP || k == SDLK_DOWN) uxPendente = ajw_grade_mover(uxPendente, m, cols, 0, k == SDLK_DOWN ? 1 : -1, ok);
  }
  return 1;
}

static int ajwEvento(SDL_Keycode k) {
  if (uxEditor) return ajwEventoPagina(k);
  if (iptvForm.tipo && uxVoltar(k)) { ajIptvVoltar(); return 1; }
  if (k != SDLK_LEFT && k != SDLK_RIGHT) ajwRascGravar();   // qualquer outra tecla grava o rascunho
  // O ALTO: Buscar, Alterados, Guia.
  if (focoIndice && ajwTopo >= 0) {
    if (uxVoltar(k)) { sair = 1; return 1; }
    if (k == SDLK_LEFT) { if (ajwTopo > 0) ajwTopo--; return 1; }
    if (k == SDLK_RIGHT) { if (ajwTopo + 1 < AJW_T_N) ajwTopo++; return 1; }
    if (k == SDLK_UP) { if (ajustes_menu_topo()) sair = 1; return 1; }
    if (k == SDLK_DOWN) {
      if (ajwModo == AJW_M_BUSCA && ajwNRes) { focoIndice = 0; ajwSincronizarFoco(); }
      else if (ajwModo == AJW_M_ALTERADOS && uxNDifs) { focoIndice = 0; ajwSincronizarFoco(); }
      else { ajwModo = AJW_M_CATEGORIA; focarSecao(secAtual); focoIndice = 1; }
      ajwTopo = -1;
      return 1;
    }
    if (uxOk(k)) {
      if (ajwTopo == AJW_T_BUSCA) { ajwBuscaAbrir(); return 1; }
      if (ajwTopo == AJW_T_ALTERADOS) {
        uxListarDiferencas(); uxDifFoco = 0; ajwModo = AJW_M_ALTERADOS; ajwRolAlvo = 0;
        if (uxNDifs) { focoIndice = 0; ajwTopo = -1; ajwSincronizarFoco(); }
        return 1;
      }
      guiaAbrir(0); guiaDoIndice = 1;
      return 1;
    }
    return 1;
  }
  // AS ABAS.
  if (focoIndice) {
    int s = secAtual;
    if (uxVoltar(k)) { sair = 1; return 1; }
    if (k == SDLK_UP) { ajwTopo = AJW_T_BUSCA; return 1; }
    if (k == SDLK_LEFT) { if (s > 0) { focarSecao(s - 1); focoIndice = 1; ajwRolAlvo = 0; } else sair = 1; return 1; }
    if (k == SDLK_RIGHT) { if (s + 1 < nSecoes) { focarSecao(s + 1); focoIndice = 1; ajwRolAlvo = 0; } return 1; }
    if (k == SDLK_DOWN || uxOk(k)) { ajwModo = AJW_M_CATEGORIA; focarSecao(s); focoIndice = 0; return 1; }
    return 1;
  }
  // A LISTA.
  if (uxVoltar(k)) return ajwSaiDaLista(k);
  if (k == SDLK_UP || k == SDLK_DOWN) {
    int d = k == SDLK_DOWN ? 1 : -1;
    uxRetornarOp = -1;
    if (ajwModo == AJW_M_BUSCA || ajwModo == AJW_M_ALTERADOS) {
      int *f = ajwModo == AJW_M_BUSCA ? &ajwResFoco : &uxDifFoco, n = ajwModo == AJW_M_BUSCA ? ajwNRes : uxNDifs;
      if (*f + d < 0) { focoIndice = 1; ajwTopo = ajwModo == AJW_M_BUSCA ? AJW_T_BUSCA : AJW_T_ALTERADOS; return 1; }
      if (*f + d < n) *f += d;
      ajwSincronizarFoco();
      return 1;
    }
    for (int i = focoItem + d; i > secIni[secAtual] && i < secFim(secAtual); i += d)
      if (focavel(i)) { focar(i); focoIndice = 0; return 1; }
    if (d < 0) { focoIndice = 1; ajwTopo = -1; }
    return 1;
  }
  if (k == SDLK_LEFT || k == SDLK_RIGHT) { ajwSincronizarFoco(); return ajwLateral(k == SDLK_RIGHT ? 1 : -1); }
  if (k == SDLK_PAGEUP || k == SDLK_PAGEDOWN) return 1;
  if (uxOk(k)) return ajwOkLinha();
  return 1;
}

// O evento cru (antes de eventoTela, que so ve KEYDOWN): o OK da lista age ao
// soltar quando a linha tem padrao para voltar; segurado, restaura.
static int ajwEventoBruto(const SDL_Event *e) {
  SDL_Keycode k;
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0;
  k = e->key.keysym.sym;
  if (k != SDLK_RETURN && k != SDLK_KP_ENTER) return 0;
  if (ajwOkReentra) return 0;
  if (teclado_aberto() || uxEditor || focoIndice || filAberta || frAberta || hcAberta || riscoFolha || guiaAberto || apoioAberto ||
      ajModalAberto() || (ajwModo == AJW_M_BUSCA)) { if (e->type == SDL_KEYUP) ajw_ok_soltar(&ajwOk); return 0; }
  if (e->type == SDL_KEYDOWN) {
    if (e->key.repeat) {
      if (ajw_ok_tique(&ajwOk, SDL_GetTicks())) ajwRestaurar(ajwOpDaLinha());
      return ajwOk.armado || ajwOk.usado;
    }
    ajwSincronizarFoco();
    return ajw_ok_baixo(&ajwOk, SDL_GetTicks(), ajwPodeRestaurar(ajwOpDaLinha()));
  }
  { int r = ajw_ok_cima(&ajwOk, SDL_GetTicks());
    if (r == 2) { ajwRestaurar(ajwOpDaLinha()); return 1; }
    if (r == 1) {
      SDL_Event d = *e;
      d.type = SDL_KEYDOWN; d.key.repeat = 0; d.key.state = SDL_PRESSED;
      ajwOkReentra = 1;
      eventoTela(&d);
      ajwOkReentra = 0;
      return 1;
    }
    return 0; }
}

static void ajwAtualizar(float dt) {
  Uint32 agora = SDL_GetTicks();
  float k = ajustes_animacoes_reduzidas() ? 1.0f : 1.0f - expf(-dt * 16.0f);
  if (ajw_rasc_vencido(&ajwRasc, agora)) ajwRascGravar();
  if (ajw_ok_tique(&ajwOk, agora)) ajwRestaurar(ajwOpDaLinha());
  ajwRol += (ajwRolAlvo - ajwRol) * k;
  ajwGradeRol += (ajwGradeRolAlvo - ajwGradeRol) * k;
  ajwAbasDesloc += (ajwAbasDeslocAlvo - ajwAbasDesloc) * k;
  if (ajwPreviaA < 1.0f) { ajwPreviaA += dt * 7.0f; if (ajwPreviaA > 1.0f) ajwPreviaA = 1.0f; }
  if (ajwModo == AJW_M_ALTERADOS && uxDifFoco >= uxNDifs) uxDifFoco = uxNDifs ? uxNDifs - 1 : 0;
}

#include "ajustes_ux_world_cenas.inc"

// --- DESENHO ----------------------------------------------------------------------------------
static int ajwNAlterados(void) {
  int n = 0;
  for (int i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && ajOpcaoDisponivel(TELA[i].op) && uxDiferente(TELA[i].op) &&
        (TELA[i].op != AJ_ICONE_APP || apoiador_ativo())) n++;
  return n;
}
static float ajwBotao(const char *icone, const char *rot, float xDir, float y, int foco, int ponto) {
  TxtLinha t = foco ? ajwTxtP(TXT_BODY, rot, 0) : ajwTxt(TXT_BODY, rot, 212, 0);
  float ic = icone ? 27.0f : 0.0f, pt = ponto ? 22.0f : 0.0f;
  float w = 24.0f + ic + (ic > 0 ? 14.0f : 0) + pt + t.w + 26.0f;
  GfxRect r = { xDir - w, y, w, AJW_CAB_H };
  float cx = r.x + 24.0f;
  if (foco) ajwRetRGB(r, r.h * 0.5f, AJW_BRANCO, 1);
  else ajwRetRGB(r, r.h * 0.5f, 1, 1, 1, 0.08f);
  if (icone) {
    float c = foco ? 0.05f : 0.83f;
    gfx_icone((GfxRect){ cx, r.y + (r.h - ic) * 0.5f, ic, ic }, icone, c, c, c, 1);
    cx += ic + 14.0f;
  }
  if (ponto) { float ar, ag, ab; ajwAcento(&ar, &ag, &ab);
    ajwRetRGB((GfxRect){ cx, r.y + r.h * 0.5f - 6, 12, 12 }, 6, ar, ag, ab, 1); cx += pt; }
  txt_desenhar(t, cx, r.y + (r.h - t.h) * 0.5f);
  return w;
}
static void ajwCabecalho(void) {
  float x0 = ajwX0(), xd = ajwX1(), y = AJW_TOPO;
  int noTopo = focoIndice && ajwTopo >= 0 && !uxEditor;
  char t[96];
  { TxtLinha l = ajwTxtB(TXT_TITULO3, "Ajustes", 0); txt_desenhar(l, x0, y + (AJW_CAB_H - l.h) * 0.5f); }
  xd -= ajwBotao("aj_book-open", "Guia", xd, y, noTopo && ajwTopo == AJW_T_GUIA, 0) + 18.0f;
  snprintf(t, sizeof t, i18n("Alterados · %d"), ajwNAlterados());
  xd -= ajwBotao(NULL, t, xd, y, noTopo && ajwTopo == AJW_T_ALTERADOS, 1) + 18.0f;
  { const char *b = ajwBusca[0] && ajwModo == AJW_M_BUSCA ? ajwBusca : i18n("Buscar ajuste");
    int f = noTopo && ajwTopo == AJW_T_BUSCA;
    float w = 420.0f;
    GfxRect r = { xd - w, y, w, AJW_CAB_H };
    TxtLinha l = f ? ajwTxtP(TXT_BODY, b, w - 90) : ajwTxt(TXT_BODY, b, ajwModo == AJW_M_BUSCA ? 230 : 163, w - 90);
    float c = f ? 0.05f : 0.64f;
    if (f) ajwRetRGB(r, r.h * 0.5f, AJW_BRANCO, 1); else ajwRetRGB(r, r.h * 0.5f, 1, 1, 1, 0.08f);
    gfx_icone((GfxRect){ r.x + 24, r.y + (r.h - 27) * 0.5f, 27, 27 }, "aj_search", c, c, c, 1);
    txt_desenhar(l, r.x + 24 + 27 + 14, r.y + (r.h - l.h) * 0.5f); }
}

// A fileira de abas (ou, nos outros modos, o titulo do que a lista mostra).
static void ajwAbas(void) {
  float x0 = ajwX0(), w = ajwX1() - x0, y = AJW_ABAS_Y, base = y + AJW_ABAS_H;
  float pos[AJ_MAX_SECOES + 1], lar[AJ_MAX_SECOES], total = 0;
  int s, foco = focoIndice && ajwTopo < 0 && !uxEditor;
  float ar, ag, ab;
  ajwAcento(&ar, &ag, &ab);
  ajwRet((GfxRect){ x0, base - 1.5f, w, 1.5f }, 0, 0.15f, 1);
  if (uxEditor) {
    char b[240];
    TxtLinha a = ajwTxt(TXT_V3_LN, uxCaminho(uxOp), 138, 0), sep = ajwTxt(TXT_V3_LN, "›", 138, 0);
    snprintf(b, sizeof b, "%s", i18n(rotuloOpcao(uxOp)));
    TxtLinha t = ajwTxtB(TXT_W_ABA_B, b, w - a.w - sep.w - 40);
    txt_desenhar(a, x0, base - 22 - a.h);
    txt_desenhar(sep, x0 + a.w + 12, base - 22 - sep.h);
    txt_desenhar(t, x0 + a.w + sep.w + 24, base - 22 - t.h + 2);
    return;
  }
  if (ajwModo == AJW_M_BUSCA || ajwModo == AJW_M_ALTERADOS) {
    char b[200];
    if (ajwModo == AJW_M_ALTERADOS) snprintf(b, sizeof b, "%s", i18n("Diferentes do padrão"));
    else if (ajwNRes == 1) snprintf(b, sizeof b, i18n("1 resultado para “%s”"), ajwBusca);
    else snprintf(b, sizeof b, i18n("%d resultados para “%s”"), ajwNRes, ajwBusca);
    TxtLinha t = ajwTxtB(TXT_W_ABA_B, b, w * 0.6f);
    TxtLinha d = ajwTxt(TXT_V2_24, ajwModo == AJW_M_BUSCA ? "procura no nome, na explicação e em palavras parecidas"
                                                          : "Segure OK numa linha para voltar ao padrão", 115, w - t.w - 40);
    txt_desenhar(t, x0, base - 20 - t.h);
    txt_desenhar(d, x0 + t.w + 24, base - 22 - d.h);
    return;
  }
  for (s = 0; s < nSecoes && s < AJ_MAX_SECOES; s++) {
    TxtLinha l = txt_linha(s == secAtual ? TXT_V3_LN_B : TXT_V3_LN, ajwAbaNome(s), 0, 0, 0, 0);
    lar[s] = (float)l.w; pos[s] = total; total += lar[s] + 36.0f;
  }
  total -= 36.0f;
  if (nSecoes > 0) ajwAbasDeslocAlvo = ajw_abas_desloc(total, w, pos[secAtual], pos[secAtual] + lar[secAtual], 80.0f, ajwAbasDeslocAlvo);
  { int rolou = total > w;
    if (rolou) gfx_esvanecer_x(x0, x0 + (ajwAbasDesloc > 2 ? 60 : 0), x0 + w - 60, x0 + w);
    gfx_recorte(x0, y - 10, w, AJW_ABAS_H + 12);
    for (s = 0; s < nSecoes && s < AJ_MAX_SECOES; s++) {
      int at = s == secAtual;
      float x = x0 + pos[s] - ajwAbasDesloc;
      TxtLinha l = at ? txt_linha(TXT_V3_LN_B, ajwAbaNome(s), 243, 242, 239, 255) : ajwTxt(TXT_V3_LN, ajwAbaNome(s), 138, 0);
      if (x + lar[s] < x0 - 4 || x > x0 + w + 4) continue;
      if (at && foco) {   // aba em foco: a pilula clara
        GfxRect r = { x - 20, y - 2, lar[s] + 40, AJW_ABAS_H - 14 };
        ajwRetRGB(r, r.h * 0.5f, AJW_BRANCO, 1);
        l = txt_linha(TXT_V3_LN_B, ajwAbaNome(s), 12, 12, 14, 255);
        txt_desenhar(l, x, r.y + (r.h - l.h) * 0.5f);
      } else txt_desenhar(l, x, y + (AJW_ABAS_H - 14 - l.h) * 0.5f - 2);
      if (at) ajwRetRGB((GfxRect){ x, base - 5, lar[s], 5 }, 2.5f, ar, ag, ab, 1);
    }
    gfx_sem_recorte();
    if (rolou) gfx_sem_esvanecer_x(); }
}

// Uma linha da lista. `rot` e `caminho` (pode ser NULL) ja vem do item.
static float ajwLinha(int op, int item, float x, float y, float w, int foco, const char *caminho, float aMasc) {
  float h = caminho ? AJW_RES_H : (foco ? AJW_LN_FOCO_H : AJW_LN_H);
  GfxRect r = { x, y, w, h - 6.0f };
  int mais = op < 0 && item >= 0 && TELA[item].tipo == IT_MAIS;
  int apagada = op >= 0 && inativa(op);
  char val[200] = "";
  float ar, ag, ab, xd = r.x + r.w - 30.0f, yc = r.y + (caminho ? 36.0f : r.h * 0.5f);
  ajwAcento(&ar, &ag, &ab);
  if (aMasc <= 0.01f) return h;
  if (foco) {
    if (apagada) ajwRet(r, 16, 0.20f, aMasc);
    else ajwRetRGB(r, 16, AJW_BRANCO, aMasc);
  }
  if (mais) {
    char rot[120]; int n = maisContar(secAtual, NULL, 0);
    snprintf(rot, sizeof rot, "%s", i18n(maisAberto[secAtual] ? "Menos opções" : "Mais opções"));
    TxtLinha l = foco ? ajwTxtP(TXT_W_LN_B, rot, w - 140) : ajwTxt(TXT_W_LN, rot, 163, w - 140);
    char c[24]; snprintf(c, sizeof c, "%d", n);
    TxtLinha lc = foco ? ajwTxtP(TXT_V2_28, c, 0) : ajwTxt(TXT_V2_28, c, 140, 0);
    float ic = foco ? 0.05f : 0.55f;
    ajwSeta(maisAberto[secAtual] ? 3 : 2, xd - 26, yc - 13, 26, ic, aMasc);
    txt_desenhar_alpha(lc, xd - 26 - 12 - lc.w, yc - lc.h * 0.5f, aMasc);
    txt_desenhar_alpha(l, r.x + 30, yc - l.h * 0.5f, aMasc);
    return h;
  }
  if (op < 0) return h;
  if (sairArmado == op && foco) snprintf(val, sizeof val, "%s", i18n("OK de novo para confirmar"));
  else if (ajwRasc.op == op) uxValorTexto(op, ajwRasc.v, val, sizeof val);
  else if (OPCOES[op].tipo != OP_ACAO || textoValor(op)[0]) snprintf(val, sizeof val, "%s", textoValor(op));
  { int setas = foco && !apagada && (ajwNaLinha(op) || ehInterruptor(op));
    int abre = foco && !apagada && !setas && (OPCOES[op].tipo != OP_LEITURA);
    float largVal = w * 0.42f;
    TxtLinha lv = foco ? (apagada ? ajwTxt(TXT_V2_28, val, 170, largVal) : ajwTxtP(TXT_W_ABA_B, val, largVal))
                       : ajwTxt(TXT_V2_28, val, apagada ? 92 : 163, largVal);
    float c = apagada ? 0.6f : 0.05f;
    if (setas) {
      gfx_icone((GfxRect){ xd - 24, yc - 12, 24, 24 }, "aj_chevron-right", c, c, c, aMasc);
      xd -= 24 + 12;
    } else if (abre) {
      gfx_icone((GfxRect){ xd - 24, yc - 12, 24, 24 }, "aj_chevron-right", c, c, c, 0.8f * aMasc);
      xd -= 24 + 12;
    }
    txt_desenhar_alpha(lv, xd - lv.w, yc - lv.h * 0.5f, aMasc);
    xd -= lv.w;
    if (setas) {
      ajwSeta(1, xd - 12 - 24, yc - 12, 24, c, aMasc);
      xd -= 12 + 24;
    }
    if (!foco && !apagada && uxDiferente(op)) {   // o ponto: diferente do padrao
      ajwRetRGB((GfxRect){ xd - 12 - 11, yc - 5.5f, 11, 11 }, 5.5f, ar, ag, ab, aMasc);
      xd -= 12 + 11;
    }
    xd -= 24.0f;
    { float lx = r.x + 30, lw = xd - lx;
      TxtLinha l = foco ? (apagada ? ajwTxt(TXT_W_LN_B, rotuloOpcao(op), 200, lw) : ajwTxtP(TXT_W_LN_B, rotuloOpcao(op), lw))
                        : ajwTxt(TXT_W_LN, rotuloOpcao(op), apagada ? 110 : 226, lw);
      txt_desenhar_alpha(l, lx, yc - l.h * 0.5f, aMasc);
      if (foco && !apagada && !caminho) {   // escopo: so na linha em foco
        const char *sel = uxAvancada(op) ? "Avançado" : !strcmp(uxEscopo(op), "Conta/perfil") ? "Este perfil" : "Esta TV";
        TxtLinha ls = ajwTxt(TXT_V2_KICK, sel, 64, 0);
        float sx = lx + l.w + 16, sw = ls.w + 20;
        if (sx + sw < xd) {
          ajwRet((GfxRect){ sx, yc - 15, sw, 30 }, 8, 0.86f, aMasc);
          txt_desenhar_alpha(ls, sx + 10, yc - ls.h * 0.5f, aMasc);
        }
      }
      if (caminho) {
        TxtLinha lc = foco ? txt_linha_corta(TXT_V3_BEN, caminho, 82, 82, 84, 255, w - 60) : ajwTxt(TXT_V3_BEN, caminho, 115, w - 60);
        txt_desenhar_alpha(lc, lx, r.y + 62.0f, aMasc);
      } } }
  return h;
}

// O caminho de uma opcao: "Seção › Bloco".
static void ajwCaminho(int op, char *b, size_t n) {
  const char *bl = uxBloco(op);
  if (bl[0]) snprintf(b, n, "%s › %s", i18n(uxCaminho(op)), i18n(bl));
  else snprintf(b, n, "%s", i18n(uxCaminho(op)));
}

// So linha inteira aparece: meia linha cortada pela borda vira letra picotada.
// Na rolagem, quem entra ou sai some depressa (o pedaco que falta, em alfa).
static float ajwMascara(float y0, float y1, float topo, float base) {
  float h = y1 - y0 + 1, fora = 0;
  if (y0 < topo - 2) fora = topo - y0;
  if (y1 > base + 2) fora = fmaxf(fora, y1 - base);
  return fora <= 0 ? 1.0f : fmaxf(0.0f, 1.0f - 3.0f * fora / h);
}

static void ajwLista(void) {
  float x = ajwX0(), w = ajwListaW(), topo = AJW_CORPO_Y, base = ajwCorpoBase();
  float y, focoY0 = 0, focoY1 = 0, total;
  int noFoco = !focoIndice;
  gfx_recorte(x - 4, topo - 2, w + 8, base - topo + 4);
  if (ajwModo == AJW_M_BUSCA || ajwModo == AJW_M_ALTERADOS) {
    int n = ajwModo == AJW_M_BUSCA ? ajwNRes : uxNDifs, f = ajwModo == AJW_M_BUSCA ? ajwResFoco : uxDifFoco;
    total = n * AJW_RES_H;
    ajwRolAlvo = ajw_rolagem(total, base - topo, f * AJW_RES_H, (f + 1) * AJW_RES_H, AJW_RES_H * 0.6f, ajwRolAlvo);
    y = topo - ajwRol;
    if (!n) {
      TxtLinha t = ajwTxt(TXT_W_LN, ajwModo == AJW_M_BUSCA ? "Nenhum ajuste encontrado" : "Tudo está no padrão", 163, w);
      txt_desenhar(t, x + 30, topo + 24);
    }
    for (int i = 0; i < n; i++, y += AJW_RES_H) {
      int op = ajwModo == AJW_M_BUSCA ? ajwRes[i].op : uxDifs[i];
      char c[220];
      if (y > base || y + AJW_RES_H < topo) continue;
      if (ajwModo == AJW_M_BUSCA) snprintf(c, sizeof c, "%s", ajwRes[i].caminho);
      else ajwCaminho(op, c, sizeof c);
      ajwLinha(op, -1, x, y, w, noFoco && i == f, c, ajwMascara(y, y + AJW_RES_H - 6, topo, base));
    }
    gfx_sem_recorte();
    return;
  }
  // Categoria: grupos (ROT), linhas e "Mais opções".
  { int s = secAtual, i, primeiro = 1;
    float yy = 0;
    for (i = secIni[s] + 1; i < secFim(s); i++) {   // medir
      if (!visivel(i) && TELA[i].tipo != IT_MAIS) continue;
      if (TELA[i].tipo == IT_ROT) { yy += primeiro ? AJW_GRP_H - 10 : AJW_GRP_H + 8; primeiro = 0; continue; }
      if (TELA[i].tipo == IT_MAIS && !focavel(i)) continue;
      if (TELA[i].tipo != IT_OPC && TELA[i].tipo != IT_MAIS) continue;
      primeiro = 0;
      { float h = (i == focoItem && noFoco) ? AJW_LN_FOCO_H : AJW_LN_H;
        if (i == focoItem) { focoY0 = yy; focoY1 = yy + h; }
        yy += h; }
    }
    total = yy;
    if (!noFoco) ajwRolAlvo = 0;   // com o foco nas abas, a categoria do comeco
    else if (focoItem > secIni[s] && focoItem < secFim(s)) {
      // O rotulo do bloco acima da linha em foco entra junto quando cabe.
      float y0 = focoY0;
      if (focoItem - 1 > secIni[s] && TELA[focoItem - 1].tipo == IT_ROT) y0 -= AJW_GRP_H;
      if (focoItem == primeiroDaSecao(s)) y0 = 0;
      ajwRolAlvo = ajw_rolagem(total, base - topo, y0, focoY1, 30.0f, ajwRolAlvo);
    }
    y = topo - ajwRol; primeiro = 1;
    for (i = secIni[s] + 1; i < secFim(s); i++) {
      if (!visivel(i) && TELA[i].tipo != IT_MAIS) continue;
      if (TELA[i].tipo == IT_ROT) {
        float gy = y + (primeiro ? 0 : 18);
        if (gy + 30 > topo && gy < base) {
          ajCapsRGB(TXT_V2_KICK, TELA[i].titulo, 2.4f, x + 30, gy + 6, ajwMascara(gy, gy + 30, topo, base), 128, 128, 130);
        }
        y += primeiro ? AJW_GRP_H - 10 : AJW_GRP_H + 8; primeiro = 0;
        continue;
      }
      if (TELA[i].tipo == IT_MAIS && !focavel(i)) continue;
      if (TELA[i].tipo != IT_OPC && TELA[i].tipo != IT_MAIS) continue;
      primeiro = 0;
      { int f = i == focoItem && noFoco;
        float h = f ? AJW_LN_FOCO_H : AJW_LN_H;
        if (y < base && y + h > topo) ajwLinha(TELA[i].tipo == IT_OPC ? TELA[i].op : -1, i, x, y, w, f, NULL, ajwMascara(y, y + h - 6, topo, base));
        y += h; }
    } }
  gfx_sem_recorte();
}

// A pagina de valores: a grade no lugar da lista.
static void ajwPaginaValores(void) {
  int op = uxOp, n = nValores(op), cols, linhas, i;
  float x = ajwX0(), w = ajwListaW(), topo = AJW_CORPO_Y, base = ajwCorpoBase(), cw, chh = 81.0f, vao = 12.0f;
  float ar, ag, ab;
  ajwAcento(&ar, &ag, &ab);
  if (OPCOES[op].tipo == OP_NUMERO) {
    char t[96];
    float frac, yc = topo + 150;
    uxValorTexto(op, uxPendente, t, sizeof t);
    TxtLinha l = ajwTxtB(TXT_V2_NUM, t, w - 160);
    ajwSeta(1, x + 10, yc - 22, 44, 0.95f, 0.8f);
    txt_desenhar(l, x + (w - l.w) * 0.5f, yc - l.h * 0.5f);
    gfx_icone((GfxRect){ x + w - 54, yc - 22, 44, 44 }, "aj_chevron-right", AJW_BRANCO, 0.8f);
    frac = OPCOES[op].max > OPCOES[op].min ? (float)(uxPendente - OPCOES[op].min) / (float)(OPCOES[op].max - OPCOES[op].min) : 0;
    if (op == AJ_FIL_LIMITE) frac = ajFracaoLimite((float)uxPendente);
    frac = frac < 0 ? 0 : frac > 1 ? 1 : frac;
    ajwRet((GfxRect){ x + 10, yc + 110, w - 20, 10 }, 5, 0.18f, 1);
    ajwRetRGB((GfxRect){ x + 10, yc + 110, (w - 20) * frac, 10 }, 5, ar, ag, ab, 1);
    ajwRetRGB((GfxRect){ x + 10 + (w - 20) * frac - 14, yc + 101, 28, 28 }, 14, AJW_BRANCO, 1);
    { char pd[160], b[220]; uxValorTexto(op, uxValorPadrao(op), pd, sizeof pd);
      snprintf(b, sizeof b, i18n("Padrão · %s"), pd);
      TxtLinha p = ajwTxt(TXT_V2_24, b, 138, w);
      if (uxTemPadrao(op)) txt_desenhar(p, x + 10, yc + 160); }
    return;
  }
  if (ajPaleta(op)) {   // as cores em fileiras, como a regua de sempre
    float cy = topo + 50;
    for (int f = 0; f < (op == AJ_FUNDO_DIN_COR ? 2 : 4); f++) {
      int m; const int *L = ajReguaFila(op, f, &m);
      float passo = fminf(96.0f, (w - 40) / (float)(m > 0 ? m : 1));
      for (int k = 0; k < m; k++) {
        int v = L[k], foco = v == uxPendente;
        float cx = x + 50 + k * passo;
        if (op == AJ_TEMA && v >= 0 && v < AJ_N_TEMAS_OPC && v < (int)(sizeof TEMA_ACENTO / sizeof *TEMA_ACENTO) && f < 2)
          aj2Bolinha(v, cx, cy, foco, v == uxOriginal, 1);
        else {   // dinamicas e texturas: o nome numa pilula
          char t[80];
          float pw = fminf(passo * 2.6f, (w - 60) / (float)(m > 0 ? m : 1) - 12), px = x + 10 + k * (pw + 12);
          GfxRect r = { px, cy - 30, pw, 60 };
          uxValorTexto(op, v, t, sizeof t);
          if (foco) ajwRetRGB(r, 30, AJW_BRANCO, 1); else ajwRet(r, 30, 0.12f, 1);
          if (v == uxOriginal && !foco) { float ar2, ag2, ab2; ajwAcento(&ar2, &ag2, &ab2); gfx_anel(r, 0.5f, 3, ar2, ag2, ab2, 1); }
          TxtLinha l = foco ? ajwTxtP(TXT_V2_24, t, r.w - 24) : ajwTxt(TXT_V2_24, t, 200, r.w - 24);
          txt_desenhar(l, r.x + (r.w - l.w) * 0.5f, cy - l.h * 0.5f);
        }
      }
      cy += f >= 1 ? 110 : 104;
    }
    return;
  }
  cols = ajw_grade_colunas(n);
  cw = (w - (cols - 1) * vao) / cols;
  linhas = (n + cols - 1) / cols;
  { int fl = uxPendente / cols;
    float total = linhas * (chh + vao);
    ajwGradeRolAlvo = ajw_rolagem(total, base - topo - 50, fl * (chh + vao), fl * (chh + vao) + chh, chh * 0.7f, ajwGradeRolAlvo); }
  gfx_recorte(x - 4, topo - 2, w + 8, base - topo - 46);
  for (i = 0; i < n; i++) {
    int c = i % cols, l = i / cols, foco = i == uxPendente && !uxRodape, gravado = i == uxOriginal, ok = ajwValorOk(op, i);
    GfxRect r = { x + c * (cw + vao), topo + l * (chh + vao) - ajwGradeRol, cw, chh };
    char t[200];
    float tx = r.x + 26;
    if (r.y > base || r.y + r.h < topo) continue;
    if(op==AJ_GUIA_CATEGORIAS && i<uxNCategorias) snprintf(t, sizeof t, "%s", uxCategorias[i].nome);
    else uxValorTexto(op, i, t, sizeof t);
    if (op == AJ_TEX_MB && !ok) snprintf(t, sizeof t, "%s", i18n(OPCOES[op].valores[i]));
    if (foco) ajwRetRGB(r, 16, AJW_BRANCO, 1);
    else ajwRetRGB(r, 16, 1, 1, 1, 0.06f);
    if (gravado && !foco) gfx_anel(r, 16.0f / r.h, 3, ar, ag, ab, 1);
    if (op == AJ_ICONE_APP) { iconeapp_desenhar(i, (GfxRect){ r.x + 14, r.y + 12, 57, 57 }, 1, foco ? 1.0f : 0.75f); tx = r.x + 86; }
    if (op == AJ_LOGO_APP) { logoapp_miniatura(i, (GfxRect){ r.x + 14, r.y + 12, 57, 57 }, 1, foco ? 1.0f : 0.75f); tx = r.x + 86; }
    { int fv = marcaDoValor(op, OPCOES[op].valores && i < OPCOES[op].n ? OPCOES[op].valores[i] : NULL);
      if (fv >= 0) { float k = foco ? 0.05f : 0.9f; marca_formato((FormatoMarca)fv, tx, r.y + 20, 40, k, k, k, 1); }
      else {
        TxtLinha lt = foco ? ajwTxtP(TXT_W_ABA_B, t, r.x + r.w - tx - 56) : ajwTxt(TXT_CALLOUT, t, ok ? 226 : 92, r.x + r.w - tx - 56);
        txt_desenhar(lt, tx, r.y + (r.h - lt.h) * 0.5f);
      } }
    if (op == AJ_GUIA_CATEGORIAS && i < uxNCategorias) {
      GfxRect box = { r.x + r.w - 48, r.y + (r.h - 26) * 0.5f, 26, 26 };
      gfx_anel_fora(box, 0.18f, 0, 2, foco ? 0.1f : 0.65f, foco ? 0.1f : 0.67f, foco ? 0.1f : 0.72f, 0.85f);
      if (uxCategorias[i].oculta) { gfx_cor(box, 0.18f, ar, ag, ab, 1); ajIconeT("aj_check", box.x, box.y, 26, 1); }
    } else if (gravado) {
      float k = foco ? 0.05f : 0.95f;
      gfx_icone((GfxRect){ r.x + r.w - 48, r.y + (r.h - 26) * 0.5f, 26, 26 }, "aj_check", k, k, k, 1);
    }
  }
  gfx_sem_recorte();
  if (op != AJ_GUIA_CATEGORIAS) {
    TxtLinha d = ajwTxt(TXT_V2_24, "Contorno = valor salvo agora. A prévia segue o foco antes de você confirmar.", 115, w);
    txt_desenhar(d, x + 4, base - 34);
  }
}

// A coluna da prévia.
static void ajwPrevia(void) {
  float x = ajwPreviaX(), w = ajwPreviaW(), y = AJW_CORPO_Y, base = ajwCorpoBase();
  float h = w * 9.0f / 16.0f;
  int op = -1, sec = secAtual, v;
  char onde[200] = "", desc[700] = "", aviso[300] = "";
  if (h > (base - y) * 0.62f) { h = (base - y) * 0.62f; }
  if (uxEditor) op = uxOp;
  else if (!focoIndice) op = ajwOpDaLinha();
  if (op != ajwUltimoOp) { ajwUltimoOp = op; ajwPreviaA = 0.35f; }
  GfxRect R = { x, y, w, h };
  // Sem opcao: a categoria (abas) ou o botao do alto.
  if (op < 0) {
    if (focoIndice && ajwTopo == AJW_T_BUSCA) {
      ajwCenaIcone(R, "aj_search", ajwPreviaA);
      snprintf(onde, sizeof onde, "%s", i18n("Buscar ajuste"));
      snprintf(desc, sizeof desc, "%s", i18n("Procura em todas as categorias pelo nome, pela explicação e por palavras parecidas. OK abre o teclado."));
    } else if (focoIndice && ajwTopo == AJW_T_ALTERADOS) {
      ajwCenaIcone(R, "aj_rotate-ccw-clock", ajwPreviaA);
      snprintf(onde, sizeof onde, "%s", i18n("Diferentes do padrão"));
      snprintf(desc, sizeof desc, "%s", i18n("Só os ajustes que você mudou. Segure OK numa linha para voltar ao padrão."));
    } else if (focoIndice && ajwTopo == AJW_T_GUIA) {
      ajwCenaIcone(R, "aj_book-open", ajwPreviaA);
      snprintf(onde, sizeof onde, "%s", i18n("Guia de uso"));
      snprintf(desc, sizeof desc, "%s", i18n("O que cada parte do Nuvio faz, com os atalhos do controle."));
    } else if (focoItem >= 0 && focoItem < AJ_N_TELA && TELA[focoItem].tipo == IT_MAIS && !focoIndice) {
      char r[400];
      maisContar(secAtual, r, sizeof r);
      ajwCenaIcone(R, "aj_sliders-horizontal", ajwPreviaA);
      snprintf(onde, sizeof onde, "%s", i18n("Opções avançadas"));
      snprintf(desc, sizeof desc, "%s", r);
    } else if (sec >= 0 && sec < nSecoes) {
      ajwCenaSecao(sec, R, ajwPreviaA);
      snprintf(onde, sizeof onde, "%s", i18n(TELA[secIni[sec]].titulo));
      snprintf(desc, sizeof desc, "%s", i18n(TELA[secIni[sec]].sub ? TELA[secIni[sec]].sub : ""));
    }
  } else {
    int alvo, cena = ajwCenaDe(op, &alvo);
    v = uxEditor && uxOp == op ? uxPendente : ajwValorLinha(op);
    if (op == AJ_MENU_TOPO) alvo = v == 1 ? AJW_A_TOPO : AJW_A_RAIL;
    ajwCenaDesenhar(cena, alvo, op, v, R, ajwPreviaA);
    snprintf(onde, sizeof onde, "%s", i18n(ajwOndeMuda(cena, alvo)));
    snprintf(desc, sizeof desc, "%s", uxResumo(op));
    if (inativa(op)) ajAvisoTexto(op, aviso, sizeof aviso);
  }
  y += h + 26;
  if (onde[0]) {
    TxtLinha l = ajwTxt(TXT_CAPTION, onde, 212, w);
    txt_desenhar(l, x, y); y += l.h + 14;
  }
  if (aviso[0]) {
    float ar, ag, ab; ajwAcento(&ar, &ag, &ab);
    y += txt_bloco(TXT_DET_META, aviso, (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), x, y, w, 36, 1, 3) + 12;
  }
  if (desc[0] && y < base - 40) txt_bloco_corta(TXT_DET_META, desc, 163, 163, 165, x, y, w, 37, 1, (int)((base - y) / 37));
}

typedef struct { const char *tecla, *acao; } AjwDica;
static void ajwRodape(void) {
  AjwDica d[5]; int n = 0;
  float x = ajwX0(), y = NV_VTELA_H - AJW_TOPO - AJW_RODAPE_H * 0.5f;
  int op = ajwOpDaLinha();
  if (uxEditor) {
    if (OPCOES[uxOp].tipo == OP_NUMERO) d[n++] = (AjwDica){ "‹ ›", "ajusta" };
    d[n++] = (AjwDica){ "OK", uxOp == AJ_GUIA_CATEGORIAS ? "marca" : "escolhe" };
    d[n++] = (AjwDica){ "Voltar", "sai sem mudar" };
  } else if (focoIndice && ajwTopo >= 0) {
    d[n++] = (AjwDica){ "‹ ›", "anda" };
    d[n++] = (AjwDica){ "OK", "abre" };
    d[n++] = (AjwDica){ "Voltar", "sai" };
  } else if (focoIndice) {
    d[n++] = (AjwDica){ "‹ ›", "troca de categoria" };
    d[n++] = (AjwDica){ "OK", "entra" };
    d[n++] = (AjwDica){ "Voltar", "sai" };
  } else if (ajwModo == AJW_M_BUSCA) {
    d[n++] = (AjwDica){ "‹ ›", "muda aqui mesmo" };
    d[n++] = (AjwDica){ "OK", "abre na categoria" };
    d[n++] = (AjwDica){ "Voltar", "limpa a busca" };
  } else {
    if (op >= 0 && !inativa(op) && (ajwNaLinha(op) || ehInterruptor(op))) d[n++] = (AjwDica){ "‹ ›", "muda o valor" };
    if (op >= 0 && !inativa(op) && OPCOES[op].tipo == OP_ESCOLHA && !ehInterruptor(op)) d[n++] = (AjwDica){ "OK", "lista de valores" };
    else if (op >= 0 && !inativa(op) && OPCOES[op].tipo == OP_ACAO) d[n++] = (AjwDica){ "OK", "abre" };
    else if (op >= 0 && inativa(op)) d[n++] = (AjwDica){ "OK", "liga o que falta" };
    if (op >= 0 && ajwPodeRestaurar(op)) d[n++] = (AjwDica){ "Segurar OK", "volta ao padrão" };
    d[n++] = (AjwDica){ "Voltar", "sai" };
  }
  for (int i = 0; i < n; i++) {
    TxtLinha t = ajwTxt(TXT_V3_PILL, d[i].tecla, 212, 0), a = ajwTxt(TXT_V2_24, d[i].acao, 120, 0);
    txt_desenhar(t, x, y - t.h * 0.5f); x += t.w + 10;
    txt_desenhar(a, x, y - a.h * 0.5f); x += a.w + 38;
  }
  if (uxAviso[0]) {   // o aviso do ultimo salvamento, na direita do rodape
    TxtLinha t = ajwTxtB(TXT_V2_24, uxAviso, 700);
    float xd = ajwX1() - t.w;
    if (xd > x) {
      gfx_icone((GfxRect){ xd - 34, y - 12, 24, 24 }, "aj_check", AJW_BRANCO, 0.75f);
      txt_desenhar_alpha(t, xd, y - t.h * 0.5f, 0.8f);
    }
  }
}

// As perguntas (restaurar, aviso de memoria, o que falta ligar): um cartao no centro.
static void ajwPergunta(void) {
  char de[160], para[160], texto[600], tit[200];
  float W = 980, H = 470, x, y, ar, ag, ab;
  int confirmar = uxConfirmar, dep = uxEditor == 2 && !uxRestaurar && !uxAvisoRisco;
  if (!uxEditor || (!uxRestaurar && !uxAvisoRisco && !dep)) return;
  ajwAcento(&ar, &ag, &ab);
  gfx_cor((GfxRect){ 0, 0, NV_VTELA_W, NV_VTELA_H }, 0, 0, 0, 0, 0.72f);
  x = (NV_VTELA_W - W) * 0.5f; y = (NV_VTELA_H - H) * 0.5f;
  ajwRet((GfxRect){ x, y, W, H }, 28, 0.09f, 1);
  x += 56; y += 50;
  if (dep) {
    ajAvisoTexto(uxOp, texto, sizeof texto);
    snprintf(tit, sizeof tit, "%s", i18n(rotuloOpcao(uxOp)));
    TxtLinha t = ajwTxtB(TXT_V2_INSP, tit, W - 112); txt_desenhar(t, x, y); y += t.h + 20;
    y += txt_bloco(TXT_DET_META, texto, 200, 200, 202, x, y, W - 112, 37, 1, 4) + 30;
    { int req = uxRequisito(uxOp);
      TxtLinha b = ajwTxtP(TXT_W_ABA_B, req >= 0 ? "Ver o ajuste necessário" : "Fechar", 0);
      GfxRect r = { x, y, b.w + 56, 72 };
      ajwRetRGB(r, 36, AJW_BRANCO, 1); txt_desenhar(b, r.x + 28, r.y + (r.h - b.h) * 0.5f); }
    return;
  }
  if (uxAvisoRisco) {
    int fil = uxAvisoRisco == SEG_AVISO_FILEIRAS;
    snprintf(tit, sizeof tit, "%s", i18n(fil ? "Mais fileiras na Home" : "Mais itens por fileira"));
    snprintf(texto, sizeof texto, "%s", i18n(fil ? "Mais fileiras usam mais memória e rede; em TVs com 1 GB a Home pode ficar lenta ou fechar. Se o app fechar, ele volta sozinho ao valor anterior."
                                                 : "Mais itens por fileira usam mais memória; em TVs com 1 GB a Home pode ficar lenta ou fechar. Se o app fechar, ele volta sozinho ao valor anterior."));
    uxValorTexto(uxOp, uxOriginal, de, sizeof de); uxValorTexto(uxOp, uxPendente, para, sizeof para);
  } else {
    char pd[160];
    uxValorTexto(uxOp, uxValorPadrao(uxOp), pd, sizeof pd);
    snprintf(tit, sizeof tit, "%s", i18n("Restaurar este ajuste?"));
    snprintf(texto, sizeof texto, i18n("O valor voltará para “%s”. %s"), pd,
      !strcmp(uxEscopoValor(uxOp, uxValorPadrao(uxOp)), "Conta/perfil") ? i18n("Esta preferência participa dos ajustes do perfil. Nenhum serviço será desconectado.") : i18n("A mudança vale só nesta TV. Nenhum serviço será desconectado."));
    uxValorTexto(uxOp, uxValorAtual(uxOp), de, sizeof de); snprintf(para, sizeof para, "%s", pd);
  }
  { TxtLinha k = ajwTxt(TXT_V2_24, rotuloOpcao(uxOp), 138, W - 112); txt_desenhar(k, x, y); y += k.h + 10; }
  { TxtLinha t = ajwTxtB(TXT_V2_INSP, tit, W - 112); txt_desenhar(t, x, y); y += t.h + 20; }
  y += txt_bloco(TXT_DET_META, texto, 200, 200, 202, x, y, W - 112, 37, 1, 4) + 22;
  { char dp[360]; snprintf(dp, sizeof dp, "%s  →  %s", de, para);
    TxtLinha l = ajwTxtB(TXT_W_ABA_B, dp, W - 112); txt_desenhar(l, x, y); y += l.h + 30; }
  { const char *sim = uxAvisoRisco ? "Continuar" : "Restaurar";
    TxtLinha a = confirmar ? ajwTxtP(TXT_W_ABA_B, sim, 0) : ajwTxtB(TXT_W_ABA_B, sim, 0);
    TxtLinha b = !confirmar ? ajwTxtP(TXT_W_ABA_B, "Cancelar", 0) : ajwTxtB(TXT_W_ABA_B, "Cancelar", 0);
    GfxRect ra = { x, y, a.w + 56, 72 }, rb = { x + a.w + 56 + 16, y, b.w + 56, 72 };
    if (confirmar) ajwRetRGB(ra, 36, AJW_BRANCO, 1); else ajwRet(ra, 36, 0.16f, 1);
    if (!confirmar) ajwRetRGB(rb, 36, AJW_BRANCO, 1); else ajwRet(rb, 36, 0.16f, 1);
    txt_desenhar(a, ra.x + 28, ra.y + (ra.h - a.h) * 0.5f);
    txt_desenhar(b, rb.x + 28, rb.y + (rb.h - b.h) * 0.5f); }
}

static void ajwDesenhar(void) {
  ponteiro_camada();   // sem alvos de ponteiro nesta tela (o clique e o OK)
  ajPaginaIlha(ajTelaR(), 0, 1);
  if (filAberta) return;   // a folha de fileiras ocupa a tela
  ajwCabecalho();
  ajwAbas();
  if (uxEditor == 1 && !uxRestaurar && !uxAvisoRisco) ajwPaginaValores();
  else ajwLista();
  ajwPrevia();
  ajwRodape();
  ajwPergunta();
}

#ifdef AJUSTES_TESTE
// Capturas e teste de comportamento (tests/ajustes_shot.c com NUVIO_SHOT_WORLD).
void ajustes_teste_ux_captura(int cenario);
int ajustes_teste_world(int cena, const char **nome) {
  static const char *const N[] = {
    "abas", "lista-gif", "lista-cartaz", "busca", "valores", "alterados", "legenda", "numero",
    "cor", "restaurar", "topo", "titulo-notas", "menu-topo", "colecao-vitrine",
  };
  if (cena < 0 || cena >= (int)(sizeof N / sizeof *N)) return 0;
  *nome = N[cena];
  ajustes_teste_ux_captura(-1);
  guiaFechar(); sair = 0;
  ajwReiniciar();
  switch (cena) {
    case 0: focarOpcao(AJ_COL_GIF); focoIndice = 1; break;
    case 1: focarOpcao(AJ_COL_GIF); valor[AJ_COL_EXPANDIR] = 1; break;
    case 2: focarOpcao(AJ_EXPANDIR); break;
    case 3: snprintf(ajwBusca, sizeof ajwBusca, "trailer"); ajwBuscar(); ajwModo = AJW_M_BUSCA; focoIndice = 0; ajwSincronizarFoco(); break;
    case 4: focarOpcao(AJ_COLECAO_EXIBICAO); uxAbrirEditor(AJ_COLECAO_EXIBICAO); uxPendente = 2; break;
    case 5: valor[AJ_TEMA] = 1; valor[AJ_RELOGIO] = 1; uxListarDiferencas(); uxDifFoco = 0; ajwModo = AJW_M_ALTERADOS; focoIndice = 0; ajwSincronizarFoco(); break;
    case 6: focarOpcao(AJ_LEG1_TAMANHO); ajwRasc.op = AJ_LEG1_TAMANHO; ajwRasc.v = 160; ajwRasc.ate = SDL_GetTicks() + 600000u; break;
    case 7: focarOpcao(AJ_LARGURA_DP); uxAbrirEditor(AJ_LARGURA_DP); uxPendente = 150; break;
    case 8: focarOpcao(AJ_TEMA); uxAbrirEditor(AJ_TEMA); break;
    case 9: valor[AJ_RELOGIO] = 1; focarOpcao(AJ_RELOGIO); uxAbrirEditor(AJ_RELOGIO); uxRestaurar = 1; uxConfirmar = 1; break;
    case 10: focoIndice = 1; ajwTopo = AJW_T_BUSCA; break;
    case 11: focarOpcao(AJ_NT_IMDB); break;
    case 12: focarOpcao(AJ_MENU_TOPO); ajwRasc.op = AJ_MENU_TOPO; ajwRasc.v = 1; ajwRasc.ate = SDL_GetTicks() + 600000u; break;
    case 13: focarOpcao(AJ_COLECAO_EXIBICAO); break;
  }
  return 1;
}
// Estado para o teste de comportamento.
int ajustes_teste_world_estado(int *op, int *modo, int *indice, int *editor, int *topo) {
  *op = focoOp; *modo = ajwModo; *indice = focoIndice; *editor = uxEditor; *topo = ajwTopo;
  return secAtual;
}
static int ajwTesteOp(const char *n) {
  static const struct { const char *n; int op; } T[] = {
    { "relogio", AJ_RELOGIO }, { "colgif", AJ_COL_GIF }, { "colexib", AJ_COLECAO_EXIBICAO },
    { "leg1tam", AJ_LEG1_TAMANHO }, { "expandir", AJ_EXPANDIR }, { "idioma", AJ_IDIOMA },
    { "guia", AJ_GUIA },
  };
  for (size_t i = 0; i < sizeof T / sizeof *T; i++) if (!strcmp(T[i].n, n)) return T[i].op;
  return -1;
}
int ajustes_teste_world_op(const char *n) { return ajwTesteOp(n); }
int ajustes_teste_world_valor(const char *n) { int op = ajwTesteOp(n); return op >= 0 ? uxValorAtual(op) : -999; }
int ajustes_teste_world_padrao(const char *n) { int op = ajwTesteOp(n); return op >= 0 ? uxValorPadrao(op) : -999; }
void ajustes_teste_world_definir(const char *n, int v) { int op = ajwTesteOp(n); if (op >= 0) valor[op] = v; }
int ajustes_teste_world_rascunho(void) { return ajwRasc.op >= 0 ? ajwRasc.v : -999; }
void ajustes_teste_world_vencer(void) {   // o relogio anda: o rascunho e o "segurar" vencem
  if (ajwRasc.op >= 0) ajwRasc.ate = SDL_GetTicks() - 1;
  if (ajwOk.armado) ajwOk.desde = SDL_GetTicks() - AJW_SEGURAR_MS - 1;
  ajwAtualizar(0.016f);
}
void ajustes_teste_world_focar(const char *n) {
  int op = ajwTesteOp(n);
  if (op >= 0) { ajwModo = AJW_M_CATEGORIA; focarOpcao(op); }
}
int ajustes_teste_world_nres(void) { return ajwNRes; }
void ajustes_teste_world_buscar(const char *q) {
  snprintf(ajwBusca, sizeof ajwBusca, "%s", q); ajwBuscar(); ajwModo = AJW_M_BUSCA; focoIndice = 0; ajwTopo = -1; ajwSincronizarFoco();
}
int ajustes_teste_world_guia(void) { return guiaAberto; }
int ajustes_teste_world_sair(void) { return sair; }
#endif
