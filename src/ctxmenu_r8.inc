// O MENU DE SEGURAR OK DAS PRANCHAS R8 (Fase 3, plano 25/11): C FOLHA (padrao)
// e A DESDOBRAR (Ajustes > Home > Menu de opcoes). Incluido por ctxmenu.c: a
// LOGICA continua a dele (montar, aplicar, as escritas, a pagina de estilos, a
// confirmacao); aqui so o DESENHO e o relogio, copiados das pranchas
// aprovadas (tools/r8/menus.py e adapt.py: R8-MC e R8-MA).
//
// Vale para o menu do cartaz aberto pela Home e pela Biblioteca, para o
// destaque e para a pasta de colecao (so a pagina de estilos). O painel de
// Salvos, a linha social e o menu de lista seguem o desenho de antes (eles
// sao da Fase 4).
//
// FUNDO: enquanto o menu R8 esta na tela, app.c congela a Home num FBO
// (ctx_r8_fundo) e a desenha recuada e escurecida numa passada so
// (gfx_snap_desenhar_xf). O cartaz em foco volta por cima, vivo
// (home_cartao_foco_por_cima). No Desdobrar as vizinhas da mesma fileira sao
// faixas da copia deslocadas (gfx_snap_desenhar_faixa). Sem a copia (video no
// ar, FBO indisponivel, copia preta) o menu fica sobre a Home viva com o veu.
#include "r8tempo.h"
#include "r8estilo.h"

// A tela das pranchas: os pixels reais do alvo (1920 x 1080 na TV).
#define R8M_TW  ((float)NV_LAYOUT_REAL_W)
#define R8M_TH  ((float)NV_LAYOUT_REAL_H)
// A Folha e uma GAVETA encostada na borda direita (tools/r8/menus.py, C_*).
#define R8M_FOLHA_W     640.0f
#define R8M_FOLHA_P      48.0f     // margem interna: titulo, meta, icones, sinopse
#define R8M_FOLHA_LX     32.0f     // a linha comeca 16 px antes do texto
#define R8M_ARTE_H      360.0f
#define R8M_ESCURO      280.0f     // degrade da Home ate a borda da gaveta
#define R8M_ESCURO_A      0.90f
#define R8M_ACAO_Y0     436.0f
#define R8M_ACAO_PASSO   64.0f
#define R8M_ACAO_H       56.0f
#define R8M_PW          470.0f     // o Desdobrar se estende isto
#define R8M_A_PASSO      66.0f
#define R8M_A_H          56.0f

static int        r8On, r8Forma, r8AbertoAnt, r8PagAnt = -1, r8EstOpAnt, r8OpFeita, r8IntFeita;
static R8Abertura r8Ab;
static R8Mola     r8Foco;
static R8Troca    r8Pag;
static double     r8Aperto = -1e9, r8Confirmou = -1e9, r8Pendente = -1e9;
static int        r8FundoOk;        // app pintou/copiou a Home parada neste quadro
static unsigned   r8FundoVer;       // muda quando o estilo da fileira muda (repintar a copia)
static int        r8Reduzido;
static int        r8Prendeu;        // home_prender_foco ligado por este menu
// A ABERTURA SO COMECA COM O FUNDO PRONTO (26/11, TCL: a gaveta aparecia ja a
// 1/3 do caminho e a Home escurecia depois). O relogio da curva comecava no
// OK, mas o quadro da pintura da copia e o seguinte (que paga o custo dela na
// GPU, 45-60 ms) passavam antes de algo aparecer. Agora a sessao nova espera:
// a Home assenta, a copia e pintada, um quadro so de copia passa (o custo da
// pintura fica nele) e so entao a curva comeca, do zero. Gaveta, veu e recuo
// saem do mesmo valor. Teto de R8M_ESPERA_MAX ms para nunca prender o menu.
#define R8M_ESPERA_MAX 400u
static int        r8Espera;         // sessao aberta, curva ainda nao comecou
static Uint32     r8EsperaDesde;
static int        r8FundoEst = -1;  // o que o quadro anterior fez (R8F_Q_*)
static Uint32     r8InicioMs;       // quando a curva comecou (testes)
static CtxR8Geo   r8Geo;            // o que o ultimo quadro desenhou (testes)
int ctx_r8_geo(CtxR8Geo *g) { if (g) *g = r8Geo; return r8Geo.visivel; }

static double r8Seg(Uint32 ms) { return (double)ms / 1000.0; }
static int r8Cartaz(GfxRect *F);
// O RELOGIO DO MENU e o `agora` que o app passa a ctx_atualizar/ctx_desenhar
// (o mesmo do resto da interface); abrir e o OK usam o ultimo visto.
static Uint32 r8Relogio;
static Uint32 r8Agora(void) { return r8Relogio ? r8Relogio : SDL_GetTicks(); }
// Cada abertura do zero (nao a reabertura no meio do fechar) e uma sessao: a
// copia parada da Home e por sessao (fundoparado_sessao).
static unsigned r8Sessao;

// ---------------------------------------------------------------- estado
static int r8Elegivel(void) {
  return !doPainel && !doSocial && !doLista && !modoInline();
}
static void r8Abrir(void) {
  double t = r8Seg(r8Agora());
  float f;
  r8On = r8Elegivel();
  if (!r8On) return;
  r8Forma = ajustes_menu_segurar();
  r8Reduzido = ajustes_animacoes_reduzidas();
  if (r8Ab.estado == 0 || r8ab_fechada(&r8Ab, t)) {
    r8Sessao++;
    r8ab_iniciar(&r8Ab, r8Reduzido ? R8_REDUZIDO_DUR : R8_MENU_ABRIR,
                 r8Reduzido ? R8_REDUZIDO_DUR : R8_MENU_FECHAR);
    r8Ab.dConteudoSair = R8_MENU_CONTEUDO_SAIR;
  }
  r8ab_abrir(&r8Ab, t);
  f = (float)(pagina == 1 ? (estFoco < 0 ? 0 : estFoco) : foco);
  r8mola_iniciar(&r8Foco, &f, 1);
  r8troca_iniciar(&r8Pag, pagina);
  r8PagAnt = pagina;
  r8EstOpAnt = 0; r8Confirmou = r8Pendente = r8Aperto = -1e9;
  r8AbertoAnt = 1;
  // O foco da Home fica preso ao titulo do menu ate ele fechar de vez (a lista
  // de estilos remonta a fileira a cada seta; ver home_prender_foco).
  { HomeItem hi;
    // (Aqui temCartaz ainda nao foi ligado: ctx_abrir o liga depois de abrirComum.)
    if (!deOutraTela && !soFileira && !doLista && home_item_focado(&hi) && hi.indice == idx) { home_prender_foco(idx); r8Prendeu = 1; } }
}
static void r8Atualizar(Uint32 agora) {
  double t;
  float alvo;
  r8Relogio = agora;
  t = r8Seg(agora);
  if (!r8On) return;
  if (r8AbertoAnt && !aberto) r8ab_fechar(&r8Ab, t);
  // Solta o foco so com o menu FECHADO DE VEZ: o estilo restaurado no Voltar
  // remonta a fileira mais uma vez, ainda com o menu saindo.
  if (r8Prendeu && !aberto && r8ab_fechada(&r8Ab, t)) { home_prender_foco(-1); r8Prendeu = 0; }
  if (!r8AbertoAnt && aberto) r8ab_abrir(&r8Ab, t);
  r8AbertoAnt = aberto;
  if (pagina != r8PagAnt) {
    r8troca_ir(&r8Pag, pagina, t);
    alvo = (float)(pagina == 1 ? (estFoco < 0 ? 0 : estFoco) : pagina == 2 ? confFoco : foco);
    r8mola_iniciar(&r8Foco, &alvo, 1);
    r8PagAnt = pagina;
  }
  alvo = (float)(pagina == 1 ? (estFoco < 0 ? 0 : estFoco) : pagina == 2 ? confFoco : foco);
  if (r8Reduzido) r8mola_iniciar(&r8Foco, &alvo, 1);
  else r8mola_ir(&r8Foco, &alvo, t);
  // A escrita: o fio de pendente corre enquanto ela esta no ar; confirmada, a
  // linha mostra o feito (marca e o texto) por 1,2 s.
  if (estadoOperacao != r8EstOpAnt) {
    if (estadoOperacao == CTX_PENDENTE) r8Pendente = t;
    if (estadoOperacao == CTX_CONFIRMADA) { r8Confirmou = t; r8OpFeita = operacao; r8IntFeita = intencao; }
    r8EstOpAnt = estadoOperacao;
  }
}
// 1 enquanto o menu R8 esta na tela (aberto ou fechando).
static int r8Visivel(Uint32 agora) {
  return r8On && !r8ab_fechada(&r8Ab, r8Seg(agora)) && r8Ab.estado != 0;
}

// ---------------------------------------------------------------- texto
// Maiusculas do titulo (a prancha usa text-transform): ASCII e Latin-1 (os dois
// bytes C3 xx); o resto passa igual.
static void r8Maiusc(const char *s, char *d, size_t n) {
  size_t k = 0;
  while (*s && k + 2 < n) {
    unsigned char c = (unsigned char)*s;
    if (c >= 'a' && c <= 'z') { d[k++] = (char)(c - 32); s++; }
    else if (c == 0xC3 && (unsigned char)s[1] >= 0xA0 && (unsigned char)s[1] <= 0xBE && (unsigned char)s[1] != 0xB7) {
      d[k++] = (char)0xC3; d[k++] = (char)((unsigned char)s[1] - 0x20); s += 2;
    } else d[k++] = *s++;
  }
  d[k] = 0;
}

// A sinopse quebrada UMA vez por texto e largura.
#define R8M_SIN_MAX 8
static struct { char chave[96]; float w; int n; char l[R8M_SIN_MAX][260]; } r8Sin;
static void r8Quebrar(const char *s, float w, TxtEstilo e) {
  char chave[96];
  const char *p = s;
  snprintf(chave, sizeof chave, "%.80s|%zu", s, strlen(s));
  if (r8Sin.w == w && !strcmp(r8Sin.chave, chave)) return;
  memset(&r8Sin, 0, sizeof r8Sin);
  snprintf(r8Sin.chave, sizeof r8Sin.chave, "%s", chave); r8Sin.w = w;
  while (*p && r8Sin.n < R8M_SIN_MAX) {
    char linha[260] = "";
    while (*p == ' ') p++;
    while (*p) {
      char tent[260];
      size_t k = txt_token_tam(p), nl = strlen(linha);
      if (nl + k + 2 >= sizeof tent) break;
      snprintf(tent, sizeof tent, "%s%s%.*s", linha, nl ? " " : "", (int)k, p);
      if (nl && (float)txt_largura(e, tent) > w) break;
      snprintf(linha, sizeof linha, "%s", tent);
      p += k;
      while (*p == ' ') p++;
    }
    snprintf(r8Sin.l[r8Sin.n++], sizeof r8Sin.l[0], "%s", linha);
  }
}

// ---------------------------------------------------------------- pecas
static const char *r8Icone(int acao, const CatItem *ci) {
  switch (acao) {
    case OP_LISTA: return "r8_bookmark";
    case OP_ASSISTIDO: return historicoDe(ci) == 1 ? "r8_eyeoff" : "r8_eye";
    case OP_TIRAR_CONTINUAR: case OP_DISPENSAR: return "r8_x";
    case OP_RECOMENDAR: return "r8_users";
    case OP_DETALHES: return "r8_info";
    case OP_ESTILO: return "r8_rows";
    case OP_CATEGORIA: return "r8_folder";
    default:
      if (acao >= OP_EXTRA && acao < OP_EXTRA + nExtras && extras[acao - OP_EXTRA].icone)
        return extras[acao - OP_EXTRA].icone;
      return "r8_info";
  }
}
// A linha da prancha (row_label_html): icone de 26 a 18 px da borda, 14 de vao,
// o rotulo em 23/600, centrados na altura.
// O icone fica `r8LinPad` para dentro da linha: 16 na Folha (alinha com o
// titulo), 18 no Desdobrar (as pranchas).
static float r8LinPad = 18.0f;
static void r8Linha(GfxRect r, const char *icone, const char *rot, float a) {
  TxtLinha t;
  if (a <= 0.003f) return;
  gfx_icone((GfxRect){ r.x + r8LinPad, r.y + (r.h - 26.0f) * 0.5f, 26.0f, 26.0f }, icone, R8_OSSO, a);
  t = txt_linha_corta(TXT_R8_23S, rot, 244, 242, 238, 255, r.w - r8LinPad - 26.0f - 14.0f - r8LinPad);
  txt_desenhar_alpha(t, r.x + r8LinPad + 26.0f + 14.0f, r.y + (r.h - (float)t.h) * 0.5f, a);
}
// A pilula de foco (r8_foco_linha: raio 10, veu 10 %, contorno de 1,5 px a
// 50 %) apertada pelo OK (encolhe `aperto` no meio do toque de 140 ms).
static void r8Pilula(GfxRect r, float raio, float a, float aperto, double t) {
  float p = (float)sin(M_PI * r8_seg((float)(t - r8Aperto), 0.0f, 0.14f));
  float s = 1.0f - aperto * p;
  GfxRect q = { r.x + r.w * (1.0f - s) * 0.5f, r.y + r.h * (1.0f - s) * 0.5f, r.w * s, r.h * s };
  (void)raio;
  r8_foco_linha(q, a);
}
// O fio da escrita no ar: corre (assintota a 92 %) enquanto pendente; completa
// e some quando confirma.
static float r8FioPendente(double t) {
  if (estadoOperacao == CTX_PENDENTE) return 0.92f * (1.0f - expf(-(float)(t - r8Pendente) / 0.35f));
  if (t - r8Confirmou < 0.16) return 1.0f;
  return 0.0f;
}
// 0..1: a linha da escrita confirmada mostra o feito (marca e texto) por 1,2 s.
static float r8Feito(double t) {
  float d = (float)(t - r8Confirmou);
  if (d < 0.0f || d > 1.36f) return 0.0f;
  return r8_seg(d, 0.0f, 0.16f) * (1.0f - r8_seg(d, 1.2f, 1.36f));
}
static int r8LinhaDaOp(int op) {
  int i;
  for (i = 0; i < nOps; i++)
    if ((op == CTX_OP_LISTA && ops[i].acao == OP_LISTA) ||
        (op == CTX_OP_HISTORICO && (ops[i].acao == OP_ASSISTIDO || (ops[i].acao >= OP_EXTRA && ops[i].acao - OP_EXTRA == extraJunta()))))
      return i;
  return -1;
}
static const char *r8TextoFeito(void) {
  if (r8OpFeita == CTX_OP_LISTA) return avisoOp ? avisoOp : (r8IntFeita ? "Salvo" : "Removido dos Salvos");
  return r8IntFeita ? "Marcado como assistido" : "Desmarcado como assistido";
}

// Arte com corte de capa e cantos arredondados (raio em px).
static void r8Arte(GfxRect r, const char *url, float raioPx, float a) {
  GLuint t;
  if (!url || !url[0] || a <= 0.003f || r.w < 2.0f || r.h < 2.0f) return;
  t = tex_obter_larg(url, r.w);
  if (!t) return;
  gfx_tex_aspect_atual = tex_aspecto(url);
  gfx_card_forcar_cover_atual = 1.0f;
  gfx_rect(r, t, GFX_CARD, 0, 0, 0, raioPx / r.h, 0, 0, 0, a);
  gfx_card_forcar_cover_atual = 0.0f;
  gfx_tex_aspect_atual = 0.0f;
}

// ---------------------------------------------------------------- o fundo
// Home congelada: recuo e veu da forma (prancha: Folha 0,975 e 50 %;
// Desdobrar 0,99 e 34 %). A prancha recua em torno de (50 %, 60 %) da tela com a
// fileira SEM o cartaz em foco; aqui a copia tem o cartaz dentro, entao o recuo
// e em torno do CENTRO DO CARTAZ: a copia dele encolhe para dentro do proprio
// lugar e o cartaz vivo, por cima, a cobre inteira (sem borda dupla). Sem
// cartaz, o ponto da prancha.
int ctx_r8_fundo(float *esc, float *ox, float *oy, float *veu, unsigned *versao) {
  Uint32 agora = r8Agora();
  float m;
  if (!r8Visivel(agora)) return 0;
  m = r8Reduzido ? (aberto ? 1.0f : 0.0f) : r8ab_valor(&r8Ab, r8Seg(agora));
  *esc = 1.0f - (r8Forma ? 0.01f : 0.025f) * m;
  { GfxRect F;
    if (r8Cartaz(&F)) { *ox = F.x + F.w * 0.5f; *oy = F.y + F.h * 0.5f; }
    else { *ox = R8M_TW * 0.5f; *oy = R8M_TH * 0.6f; } }
  *veu = (r8Forma ? 0.34f : 0.5f) * (r8Reduzido ? r8ab_linear(&r8Ab, r8Seg(agora)) : m);
  *versao = r8FundoVer;
  return 1;
}
void ctx_r8_fundo_pronto(int ok) { r8FundoOk = ok; }
void ctx_r8_fundo_estado(int q) { r8FundoEst = q; }
int  ctx_r8_inicio(Uint32 *ms) { if (r8On && !r8Espera && r8Ab.estado != 0) { if (ms) *ms = r8InicioMs; return 1; } return 0; }
// O QUANTO A GAVETA (Folha) ESTA NA TELA, 0..1 (o relogio sai com ela: dono,
// 26/11, opcao A). Mesmo valor da superficie (com a curva; nas reduzidas, a
// opacidade). O Desdobrar nao entra (Grupo C).
float ctx_r8_gaveta(void) {
  Uint32 agora = r8Agora();
  double t = r8Seg(agora);
  if (!r8On || r8Forma || r8Espera || !r8Visivel(agora)) return 0.0f;
  return r8Reduzido ? r8ab_linear(&r8Ab, t) : r8ab_valor(&r8Ab, t);
}
unsigned ctx_r8_sessao(void) { return r8Sessao; }

// O retangulo do cartaz neste quadro (a mola de foco da Home pode andar), EM
// PIXELS DA TELA: o menu R8 desenha nas medidas da prancha (1920 x 1080), fora
// da tela virtual do resto deste arquivo (ver r8Desenhar).
static int r8Cartaz(GfxRect *F) {
  float e = gfx_escala_ui();
  if (temCartaz && !cartazFixo) {
    HomeItem hi;
    if (home_item_focado(&hi) && hi.indice == idx && hi.rect.w > 8.0f && hi.rect.h > 8.0f) {
      cartazRect = ctxDaHome(hi.rect);
      *F = hi.rect;
      return 1;
    }
  }
  if (temCartaz) { *F = (GfxRect){ cartazRect.x * e, cartazRect.y * e, cartazRect.w * e, cartazRect.h * e }; return 1; }
  // A pasta de colecao (so estilos): o cartao focado da Home, quando ha.
  if (soFileira) {
    HomeItem hi;
    if (home_item_focado(&hi) && hi.rect.w > 8.0f && hi.rect.h > 8.0f &&
        hi.rect.w < 1920.0f * 0.5f && hi.rect.h < 1080.0f * 0.7f) { *F = hi.rect; return 1; }
  }
  return 0;
}
static const char *r8ArteCartaz(void) {
  if (temCartaz && cartazArte[0]) return cartazArte;
  if (soFileira) { HomeItem hi; if (home_item_focado(&hi) && hi.arte && hi.arte[0]) return hi.arte; }
  return NULL;
}
// O cartaz por cima do veu: o proprio cartao da Home (ou, fora dela, a arte).
static void r8CartazVivo(GfxRect F, float a) {
  // Da Home: o proprio cartao dela por cima, quando ainda e ele o focado. Se o
  // estilo trocado ao vivo nao mostra mais o titulo (Heroi), nada: a copia ja
  // mostra a fileira como ela esta. Antes a arte guardada era desenhada no
  // retangulo antigo e ficava flutuando sobre a fileira nova (TCL, 26/11).
  if (temCartaz && !cartazFixo) { (void)cartaoDaHome(); return; }
  if (temCartaz && cartazArte[0]) {
    r8Geo.cartazGuardado = 1;
    float raio = ajustes_raio_poster_px() / gfx_escala_ui();
    r8Arte(F, cartazArte, raio, a);
    r8_foco_conteudo((GfxRect){ F.x - 3.0f, F.y - 3.0f, F.w + 6.0f, F.h + 6.0f }, raio + 3.0f, a);
  }
}

// ---------------------------------------------------------------- conteudo
typedef struct { float x, y, w, passo, h, raio, inset, aperto; } R8Lista;

// As acoes (pagina 0): linhas, pilula, fio de pendente e o feito.
static void r8Acoes(const CatItem *ci, R8Lista L, float trA, float trDx, double t,
                    float (*cascata)(int), float cFoco) {
  int i, li = r8LinhaDaOp(estadoOperacao == CTX_PENDENTE ? operacao : r8OpFeita);
  float fp, feito = r8Feito(t), fio = r8FioPendente(t);
  r8mola_valor(&r8Foco, t, &fp);
  if (nOps > 0) {
    GfxRect pr = { L.x + trDx, L.y + L.passo * fp, L.w, L.h };
    r8Geo.pilula = pr; r8Geo.pilulaA = cFoco * trA;
    r8Pilula(pr, L.raio, cFoco * trA, L.aperto, t);
    if (fio > 0.0f && li >= 0)
      gfx_cor((GfxRect){ L.x + trDx + L.inset, L.y + L.passo * (float)li + L.h - 9.0f,
                         (L.w - 2.0f * L.inset) * fio, 3.0f }, 0.5f, R8_ACENTO, trA * cFoco);
  }
  for (i = 0; i < nOps; i++) {
    float c = cascata(i + 2) * trA;
    GfxRect r = { L.x + trDx, L.y + L.passo * (float)i + 8.0f * (1.0f - cascata(i + 2)), L.w, L.h };
    if (aberto && c > 0.5f && ponteiro_ativo())
      ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroCtxOpcao, NULL, i, 0);
    if (i == li && feito > 0.0f) {
      r8Linha(r, r8Icone(ops[i].acao, ci), ops[i].rot, c * (1.0f - feito));
      r8Linha(r, "r8_check", r8TextoFeito(), c * feito);
    } else r8Linha(r, r8Icone(ops[i].acao, ci), ops[i].rot, c);
  }
}

// A pagina de estilos (pagina 1) na mesma lista: a silhueta da forma no lugar
// do icone, o tamanho (P M G) na ponta da linha em foco e a ordem da colecao.
static void r8Estilos(R8Lista L, float trA, float trDx, double t, float (*cascata)(int), float cFoco,
                      float tetoY) {
  int i, n = nEstilos + (ordemLinha ? 1 : 0);
  float fp, passo = L.passo, h = L.h, rol = 0.0f, disp = tetoY - L.y;
  // LISTA MAIOR QUE A SUPERFICIE: as linhas ficam no passo da prancha (no
  // minimo 52) e a lista ROLA para o foco ficar no meio, recortada na area.
  if (n > 0 && passo * (float)n > disp) {
    float p2 = disp / (float)n;
    if (p2 >= 52.0f) { passo = p2; h = passo - (L.passo - L.h); }
    else { passo = 52.0f; h = 46.0f; }
  }
  r8mola_valor(&r8Foco, t, &fp);
  // So linhas INTEIRAS na janela que rola (dono, 26/11: nada pela metade na
  // beirada de baixo): a altura visivel desce ao multiplo do passo.
  if (passo * (float)n > disp) disp = floorf((disp + (passo - h)) / passo) * passo - (passo - h);
  if (passo * (float)n > disp) {
    // O foco fica perto do meio; parado (fp inteiro), a janela comeca numa
    // linha inteira e termina noutra.
    int vis = (int)((disp + (passo - h)) / passo + 0.5f);
    rol = (fp - (float)((vis - 1) / 2)) * passo;
    if (rol < 0.0f) rol = 0.0f;
    if (rol > passo * (float)(n - vis)) rol = passo * (float)(n - vis);
  }
  L.y -= rol;
  if (rol > 0.0f || passo * (float)n > disp) gfx_recorte(L.x - 8.0f, L.y + rol - 4.0f, L.w + 16.0f, disp + 8.0f);
  r8Pilula((GfxRect){ L.x + trDx, L.y + passo * fp, L.w, h }, L.raio, cFoco * trA, L.aperto, t);
  for (i = 0; i < n; i++) {
    float c = cascata(i + 2) * trA;
    GfxRect r = { L.x + trDx, L.y + passo * (float)i + 8.0f * (1.0f - cascata(i + 2)), L.w, h };
    if (c <= 0.003f) continue;
    if (i < nEstilos) {
      TxtLinha tl;
      if (aberto && c > 0.5f) ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroEstFoco, ponteiroEstOk, i, 0);
      estGlifo((GfxRect){ r.x + r8LinPad, r.y + (r.h - 26.0f) * 0.5f, 26.0f, 26.0f }, estTipo(i), 0.95f, c);
      tl = txt_linha_corta(TXT_R8_23S, estLin[i].rotulo, 244, 242, 238, 255, r.w - 160.0f);
      txt_desenhar_alpha(tl, r.x + r8LinPad + 40.0f, r.y + (r.h - (float)tl.h) * 0.5f, c * (i == estFoco ? 1.0f : 0.72f));
      // O tamanho: as letras na ponta; a escolhida em osso, as outras apagadas.
      if (estLin[i].n > 1) {
        int j;
        float x = r.x + r.w - 18.0f;
        for (j = estLin[i].n - 1; j >= 0; j--) {
          const char *pal = i18n(fil_estilo_tam_palavra(j));
          char letra[8]; int k = 0, esc = j == estTam[i];
          TxtLinha l;
          if (pal[0]) { letra[k++] = pal[0];
            while (k < 6 && ((unsigned char)pal[k] & 0xC0) == 0x80) { letra[k] = pal[k]; k++; } }
          letra[k] = 0;
          l = txt_linha(TXT_R8_19S, letra, 244, 242, 238, 255);
          x -= 30.0f;
          if (esc && i == estFoco) gfx_cor((GfxRect){ x, r.y + (r.h - 30.0f) * 0.5f, 30.0f, 30.0f }, 0.5f, 1, 1, 1, 0.16f * c);
          txt_desenhar_alpha(l, x + (30.0f - (float)l.w) * 0.5f, r.y + (r.h - (float)l.h) * 0.5f,
                             c * (esc ? 1.0f : 0.45f) * (i == estFoco ? 1.0f : 0.6f));
          x -= 4.0f;
        }
      }
    } else {
      char rot[96];
      snprintf(rot, sizeof rot, i18n("Ordem: %s"), i18n(fil_ordem_rotulo(ordemSel)));
      if (aberto && c > 0.5f) ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroEstFoco, ponteiroOrdemAtivar, nEstilos, 0);
      r8Linha(r, "r8_layers", rot, c * (i == estFoco ? 1.0f : 0.72f));
    }
  }
  gfx_sem_recorte();
}

// A confirmacao (pagina 2) dentro da superficie: o que acontece e os dois
// botoes (o primeiro faz, o segundo cancela; esquerda/direita escolhem).
static float r8Confirmacao(const CatItem *ci, float x, float y, float w, float a, double t) {
  char pergunta[240];
  const char *texto = itemASeguir(ci)
    ? i18n("A série sai da fileira. Ela volta quando você assistir um episódio novo.")
    : i18n("O ponto onde você parou é apagado. O título volta para a fileira se você assistir de novo.");
  const char *botao = "Tirar da fileira";
  float y0 = y, bx, fp;
  int i;
  snprintf(pergunta, sizeof pergunta, i18n("Tirar %s da fileira?"), ci->titulo);
  if (confExtra >= 0 && confExtra < nExtras) {
    const CtxExtra *x0 = &extras[confExtra];
    snprintf(pergunta, sizeof pergunta, i18n(x0->pergunta ? x0->pergunta : "%s"), ci->titulo);
    texto = i18n(x0->texto ? x0->texto : "");
    botao = x0->rot;
  }
  y += txt_bloco_corta(TXT_R8_28X, pergunta, 244, 242, 238, x, y, w, 36.0f, a, 2) + 14.0f;
  y += txt_bloco_corta(TXT_R8_20R, texto, 180, 177, 171, x, y, w, 29.0f, a, 4) + 28.0f;
  r8mola_valor(&r8Foco, t, &fp);
  bx = x - 18.0f;
  { float wb[2]; const char *rot[2] = { i18n(botao), i18n("Cancelar") };
    for (i = 0; i < 2; i++) wb[i] = (float)txt_largura(TXT_R8_23S, rot[i]) + 56.0f;
    r8Pilula((GfxRect){ bx + (fp > 0.5f ? wb[0] + 12.0f : 0.0f), y, fp > 0.5f ? wb[1] : wb[0], R8M_ACAO_H },
             R8_RAIO_PAINEL, a, 0.03f, t);
    for (i = 0; i < 2; i++) {
      TxtLinha l = txt_linha(TXT_R8_23S, rot[i], 244, 242, 238, 255);
      if (aberto && a > 0.5f && ponteiro_ativo())
        ponteiro_alvo(bx, y, wb[i], R8M_ACAO_H, ponteiroConfFoco, ponteiroConfOk, i, 0);
      txt_desenhar_alpha(l, bx + 28.0f, y + (R8M_ACAO_H - (float)l.h) * 0.5f, a);
      bx += wb[i] + 12.0f;
    } }
  return y + R8M_ACAO_H - y0;
}

// ---------------------------------------------------------------- C · Folha
static double r8T;
// Reduzir animacoes: sem cascata nem deslize; o conjunto so aparece e some
// na opacidade da superficie (aGeral).
// A Folha e uma gaveta: o conteudo anda com ela, sem cascata.
static float r8CascFolha(int i) { (void)i; return 1.0f; }
static float r8CascA(int i)     { return r8Reduzido ? (r8Ab.estado ? 1.0f : 0.0f) : r8ab_cascata(&r8Ab, r8T, (float)i, 0.12f, 0.05f, 0.20f); }

static void r8DesenharFolha(const CatItem *ci, double t, float m, float aGeral) {
  GfxRect F = { 0 }, S;
  int temF = r8Cartaz(&F), i;
  float sx, x, sa, cr, cg, cb, ca, tdx[3], tw[3];
  const char *arteLarga = NULL, *artePoster = r8ArteCartaz();
  CtxInfoR8 inf;
  CtxInfoEstado est;
  char tit[256];
  memset(&inf, 0, sizeof inf);
  if (ci) { est.salvo = tituloSalvo(ci); est.visto = historicoDe(ci); ctxinfo_r8(ci, &est, &inf); arteLarga = inf.arte; }
  if (!arteLarga) arteLarga = artePoster ? artePoster : (ci && ci->poster[0] ? ci->poster : NULL);
  // A GAVETA (dono, 26/11): o painel inteiro entra pela direita, de fora da
  // tela ate encostar na borda, e sai do mesmo jeito; sempre a direita. Nada
  // voa do cartaz: a arte mora no topo e anda junto. Opaca o tempo todo (so as
  // animacoes reduzidas trocam o deslize por opacidade, em aGeral).
  sx = R8M_TW - R8M_FOLHA_W;
  x = sx + R8M_FOLHA_W * (1.0f - m);
  S = (GfxRect){ x, 0.0f, R8M_FOLHA_W, R8M_TH };
  sa = (m > 0.001f ? 1.0f : 0.0f) * aGeral;
  r8LinPad = R8M_FOLHA_P - R8M_FOLHA_LX;
  r8Geo.superficie = S; r8Geo.superficieA = sa; r8Geo.forma = 0;
  if (!r8FundoOk) r8_veu(0.5f * m * aGeral);      // sem a copia parada, o veu vai aqui
  if (temF) r8CartazVivo(F, aGeral);
  // A Home escurece ate a beirada: o cartao que fica sob ela some no escuro
  // em vez de aparecer cortado.
  r8_escuro_lateral(x - R8M_ESCURO, x, R8M_ESCURO_A * m * aGeral);
  if (aberto && m > 0.5f && ponteiro_ativo()) {
    ponteiro_alvo(0, 0, R8M_TW, R8M_TH, NULL, pagina == 1 ? ponteiroEstFora : ponteiroCtxFora, 0, 0);
    ponteiro_alvo(S.x, S.y, S.w, S.h, NULL, NULL, 0, 0);
  }
  r8_superficie_cor(&cr, &cg, &cb, &ca);
  gfx_cor(S, 0.0f, cr, cg, cb, ca * sa);
  { GfxRect A = { x, 0.0f, R8M_FOLHA_W, R8M_ARTE_H };
    r8Arte(A, arteLarga, 0.0f, sa);
    // degrade da base da arte para a superficie (0 % a 55 % de baixo)
    gfx_rect((GfxRect){ A.x, A.y + A.h * 0.45f, A.w, A.h * 0.55f + 1.0f }, 0, GFX_VEU_BAIXO,
             0, 0, 0, 0, cr, cg, cb, sa); }
  // o fio da beirada (1 px, 8 %)
  gfx_cor((GfxRect){ x, 0.0f, 1.0f, R8M_TH }, 0.0f, 1, 1, 1, 0.08f * sa);

  // PAGINAS: acoes (0), estilos (1), confirmacao (2), com a troca cruzada.
  for (i = 0; i < 3; i++) tw[i] = r8troca_peso(&r8Pag, t, i, &tdx[i]);
  { const char *nome = ci ? ci->titulo : filTitulo;
    TxtLinha l;
    float tx = x + R8M_FOLHA_P, tWid = R8M_FOLHA_W - 2.0f * R8M_FOLHA_P;
    r8Maiusc(nome && nome[0] ? nome : i18n("Estilo da fileira"), tit, sizeof tit);
    l = txt_linha_corta(TXT_R8_40X, tit, 244, 242, 238, 255, tWid);
    txt_desenhar_alpha(l, tx, R8M_ARTE_H - 64.0f, sa);
    r8Geo.tituloY = R8M_ARTE_H - 64.0f; r8Geo.tituloA = sa;
    { float mx = tx, my = R8M_ARTE_H + 8.0f, mw = tWid;
      char meta[260] = "";
      int falha = estadoOperacao == CTX_FALHA;
      if (pagina == 1) snprintf(meta, sizeof meta, "%s", i18n("Estilo da fileira"));
      else if (falha) snprintf(meta, sizeof meta, "%s", i18n("Não foi possível atualizar. Tente novamente."));
      else {
        if (inf.notaImdb > 0) {
          char n[24];
          snprintf(n, sizeof n, idioma_ponto_decimal(ajustes_idioma()) ? "IMDb %d.%d" : "IMDb %d,%d",
                   inf.notaImdb / 10, inf.notaImdb % 10);
          l = txt_linha(TXT_R8_19S, n, 244, 242, 238, 255);
          txt_desenhar_alpha(l, mx, my, sa);
          mx += (float)l.w + 18.0f; mw -= (float)l.w + 18.0f;
        }
        snprintf(meta, sizeof meta, "%s%s%s", inf.meta, inf.meta[0] && inf.classif[0] ? " \xc2\xb7 " : "", inf.classif);
      }
      if (meta[0] && mw > 40.0f) {
        l = falha ? txt_linha_corta(TXT_R8_19S, meta, 240, 190, 130, 255, mw)
                  : txt_linha_corta(TXT_R8_19S, meta, 180, 177, 171, 255, mw);
        txt_desenhar_alpha(l, mx, my, sa);
      } } }
  { R8Lista L = { x + R8M_FOLHA_LX, R8M_ACAO_Y0, R8M_FOLHA_W - 2.0f * R8M_FOLHA_LX,
                  R8M_ACAO_PASSO, R8M_ACAO_H, R8_RAIO_LINHA, R8M_FOLHA_P - R8M_FOLHA_LX, 0.03f };
    float teto = R8M_TH - R8M_FOLHA_P, tx = x + R8M_FOLHA_P, tWid = R8M_FOLHA_W - 2.0f * R8M_FOLHA_P;
    if (tw[0] > 0.0f && ci) {
      r8Acoes(ci, L, tw[0] * sa, tdx[0], t, r8CascFolha, 1.0f);
      // A SINOPSE (e onde a pessoa parou) embaixo das acoes, ate o fim da folha.
      { float y = L.y + L.passo * (float)nOps + 28.0f, c = tw[0] * sa;
        if (inf.prog[0] && y + 30.0f < teto) {
          TxtLinha p = txt_linha_corta(TXT_R8_19S, inf.prog, 244, 242, 238, 255, tWid);
          txt_desenhar_alpha(p, tx + tdx[0], y, c);
          if (inf.progFrac > 0.0f) {
            gfx_cor((GfxRect){ tx + tdx[0], y + 30.0f, tWid, 4.0f }, 0.5f, 1, 1, 1, 0.14f * c);
            gfx_cor((GfxRect){ tx + tdx[0], y + 30.0f, tWid * inf.progFrac, 4.0f }, 0.5f, R8_ACENTO, c);
          }
          y += 52.0f;
        }
        if (inf.sinopse && inf.sinopse[0]) {
          int j;
          r8Quebrar(inf.sinopse, tWid, TXT_R8_20R);
          for (j = 0; j < r8Sin.n && y + 29.0f <= teto; j++, y += 29.0f) {
            char linha[270];
            int ultima = j + 1 < r8Sin.n && y + 58.0f > teto;
            snprintf(linha, sizeof linha, "%s%s", r8Sin.l[j], ultima ? "\xe2\x80\xa6" : "");
            txt_desenhar_alpha(txt_linha_corta(TXT_R8_20R, linha, 180, 177, 171, 255, tWid), tx + tdx[0], y, c);
          }
        } } }
    if (tw[1] > 0.0f) r8Estilos(L, tw[1] * sa, tdx[1], t, r8CascFolha, 1.0f, teto);
    if (tw[2] > 0.0f && ci)
      r8Confirmacao(ci, tx + tdx[2], L.y + 4.0f, tWid, tw[2] * sa, t); }
  infoDesenhada = 1; infoLadoUlt = -1;
  infoCaixa = (GfxRect){ sx, 0.0f, R8M_FOLHA_W, R8M_ARTE_H };
  menuCaixa = (GfxRect){ sx, 0.0f, R8M_FOLHA_W, R8M_TH };
}

// ---------------------------------------------------------------- A · Desdobrar
static void r8DesenharDesdobrar(const CatItem *ci, double t, float m, float aGeral) {
  GfxRect F, P;
  int temF = r8Cartaz(&F), lado, i, deitado, titulo;
  float px, need, top, passo, rowH, tw[3], tdx[3];
  r8LinPad = 18.0f;
  if (!temF) {
    // Sem cartaz (o destaque): o painel nasce no lugar do botao do destaque.
    F = (GfxRect){ 96.0f, 400.0f, 0.0f, 56.0f };
  }
  deitado = F.w > F.h * 1.2f;
  titulo = !deitado && ci && pagina != 1;
  lado = F.x + F.w + R8M_PW <= R8M_TW - 48.0f ? 1 : -1;
  passo = deitado ? 62.0f : R8M_A_PASSO;
  rowH = R8M_A_H;
  top = titulo ? 124.0f : pagina == 1 ? 60.0f : 16.0f;
  { int n = pagina == 1 ? nEstilos + (ordemLinha ? 1 : 0) : nOps;
    need = top + passo * (float)n + 12.0f;
    if (pagina == 2) need = 330.0f; }
  P.h = F.h > need ? F.h : need;
  if (deitado && P.h < F.h + 40.0f && pagina == 0) P.h = F.h + 40.0f;
  P.y = F.y;
  if (P.y + P.h > R8M_TH - 48.0f) P.y = R8M_TH - 48.0f - P.h;
  if (P.y < 48.0f) P.y = 48.0f;
  P.w = F.w + R8M_PW * m;
  P.x = lado > 0 ? F.x : F.x + F.w - P.w;
  px = lado > 0 ? F.x + F.w : F.x - R8M_PW;
  // Um cartao cortado pela borda (a fileira rolada) nao leva o painel para fora
  // da tela: o conjunto anda para dentro da margem de 48 px.
  { float minX = 48.0f, maxX = R8M_TW - 48.0f, ajuste = 0.0f;
    float esq = lado > 0 ? F.x + (temF ? 0.0f : 0.0f) : px, dirX = lado > 0 ? px + R8M_PW : F.x + F.w;
    if (esq < minX) ajuste = minX - esq;
    else if (dirX > maxX) ajuste = maxX - dirX;
    if (ajuste != 0.0f && !temCartaz) { P.x += ajuste; px += ajuste; F.x += ajuste; } }
  // Sem cartaz do titulo (destaque, pasta de colecao) o painel e so a parte
  // que se desdobra: o cartao da Home fica a vista, ao lado.
  if (!temCartaz) { P.w = R8M_PW * m; P.x = lado > 0 ? px : px + R8M_PW - P.w; }
  // As vizinhas da mesma fileira abrem espaco: faixas da copia parada.
  if (r8FundoOk && temCartaz && !r8Reduzido) {
    GfxRect esq = { 0.0f, F.y - 12.0f, F.x, F.h + 24.0f };
    GfxRect dir = { F.x + F.w, F.y - 12.0f, R8M_TW - (F.x + F.w), F.h + 24.0f };
    float veu = 0.55f * m;
    gfx_snap_desenhar_faixa(esq, lado > 0 ? -18.0f * m : -R8M_PW * m, veu);
    gfx_snap_desenhar_faixa(dir, lado > 0 ? R8M_PW * m : 18.0f * m, veu);
  }
  if (!r8FundoOk) r8_veu(0.34f * m * aGeral);
  if (aberto && m > 0.5f && ponteiro_ativo()) {
    ponteiro_alvo(0, 0, R8M_TW, R8M_TH, NULL, pagina == 1 ? ponteiroEstFora : ponteiroCtxFora, 0, 0);
    ponteiro_alvo(P.x, P.y, P.w, P.h, NULL, NULL, 0, 0);
  }
  r8Geo.superficie = P; r8Geo.superficieA = m > 0.01f ? aGeral : 0.0f; r8Geo.forma = 1;
  r8Geo.cartaz = F; r8Geo.deslVizinhas = R8M_PW * m;
  if (m > 0.01f) r8_material(P, R8_RAIO_PAINEL, aGeral);
  if (temF) r8CartazVivo(F, aGeral);
  for (i = 0; i < 3; i++) tw[i] = r8troca_peso(&r8Pag, t, i, &tdx[i]);
  if (titulo && tw[0] > 0.0f) {
    float c0 = r8CascA(0), c1 = r8CascA(1);
    CtxInfoR8 inf; CtxInfoEstado est;
    TxtLinha l = txt_linha_corta(TXT_R8_30B, ci->titulo, 244, 242, 238, 255, R8M_PW - 60.0f);
    txt_desenhar_alpha(l, px + 30.0f + tdx[0], P.y + 28.0f + 10.0f * (1.0f - c0), c0 * tw[0] * aGeral);
    r8Geo.tituloY = P.y + 28.0f + 10.0f * (1.0f - c0); r8Geo.tituloA = c0 * tw[0] * aGeral;
    est.salvo = tituloSalvo(ci); est.visto = historicoDe(ci); ctxinfo_r8(ci, &est, &inf);
    if (estadoOperacao == CTX_FALHA)
      l = txt_linha_corta(TXT_R8_19M, i18n("Não foi possível atualizar. Tente novamente."), 240, 190, 130, 255, R8M_PW - 60.0f);
    else l = txt_linha_corta(TXT_R8_19M, inf.meta, 180, 177, 171, 255, R8M_PW - 60.0f);
    txt_desenhar_alpha(l, px + 30.0f + tdx[0], P.y + 70.0f + 10.0f * (1.0f - c1), c1 * tw[0] * aGeral);
  }
  { R8Lista L = { px + 16.0f, P.y + top, R8M_PW - 32.0f, passo, rowH, R8_RAIO_PAINEL, 18.0f, 0.04f };
    if (tw[0] > 0.0f && ci) r8Acoes(ci, L, tw[0] * aGeral, tdx[0], t, r8CascA, r8CascA(1));
    if (tw[1] > 0.0f) {
      TxtLinha h = txt_linha_corta(TXT_R8_24B, i18n("Estilo da fileira"), 244, 242, 238, 255, R8M_PW - 60.0f);
      txt_desenhar_alpha(h, px + 30.0f + tdx[1], P.y + 18.0f, r8CascA(0) * tw[1] * aGeral);
      L.passo = 52.0f; L.h = 46.0f;
      r8Estilos(L, tw[1] * aGeral, tdx[1], t, r8CascA, r8CascA(1), P.y + P.h - 12.0f);
    }
    if (tw[2] > 0.0f && ci) r8Confirmacao(ci, px + 30.0f + tdx[2], P.y + 28.0f, R8M_PW - 60.0f, tw[2] * aGeral, t); }
  infoDesenhada = 1; infoLadoUlt = lado;
  infoCaixa = F;
  menuCaixa = (GfxRect){ px, P.y, R8M_PW, P.h };
}

// 1 = o desenho e deste arquivo (aberto ou fechando); 0 = o de antes.
// AS MEDIDAS SAO AS DA PRANCHA, EM PIXELS DA TELA 1920 x 1080 (r8DesenharReal
// sai da tela virtual do arquivo): o tamanho do texto da interface nao muda a
// Folha nem o Desdobrar, que sao desenho fixo aprovado.
static int r8DesenharReal(Uint32 agora);
static int r8Desenhar(Uint32 agora) {
  int r;
  if (!r8On) return 0;
  ESCALA_REAL_INI();
  r = r8DesenharReal(agora);
  ESCALA_REAL_FIM();
  return r;
}
static int r8DesenharReal(Uint32 agora) {
  double t = r8Seg(agora);
  const CatItem *ci;
  float m, aGeral = 1.0f;
  memset(&r8Geo, 0, sizeof r8Geo);
  if (!r8On) return 0;
  if (!r8Visivel(agora)) return 1;
  r8Geo.visivel = 1;
  r8T = t;
  ci = (doLista || soFileira) ? NULL : itemAtual();
  if (!ci && pagina != 1) return 1;
  if (r8Reduzido) { aGeral = r8ab_linear(&r8Ab, t); m = 1.0f; }
  else m = r8ab_valor(&r8Ab, t);
  if (r8Forma) r8DesenharDesdobrar(ci, t, m, aGeral);
  else r8DesenharFolha(ci, t, m, aGeral);
  return 1;
}
