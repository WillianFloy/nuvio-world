#include "herotexto.h"
#include "imdbnota.h"
#include "vertudo_pag.h"
#include "vertudo.h"
#include "expansao.h"
#include "vitgeom.h"
#include "colcache.h"
#include "marcastream.h"
#include "posterprov.h"
#include "idioma.h"
#include "badges.h"
#include "descoberta.h"
#include "catalogo.h"
#include "gfx.h"
#include "text.h"
#include "sinopse.h"
#include "tex_cache.h"
#include "perfiltv.h"
#include "layout.h"
#include "anim.h"
#include "revela.h"
#include "transicao.h"
#include "ajustes.h"
#include "diretor.h"
#include "addons.h"
#include "nuvem.h"
#include "fundo.h"
#include "focoprof.h"
#include "corviva.h"
#include "ctxlista.h"
#include "vistocont.h"
#include "cwordem.h"
#include "progresso.h"
#include <time.h>
#include "ctxmenu.h"
#include "menu.h"
#include "ponteiro.h"
#include "rolagemtoque.h"
#include "telefoneui.h"
#include "escala.h"
#include "anos.h"
#include "salvos.h"
#include "dados.h"
#include "home.h"
#include "perfisessao.h"
#include <pthread.h>
#ifdef NV_TOUCH_UI
#include "perfis.h"
#include <limits.h>
#endif
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

// MEDIDAS do web (catalogSeeAllScreen, .seeall-card): cartaz de 248 de largura
// e raio 12. A 1920 cabem 5 colunas com o gutter da tela dos dois lados.
// #359: com a rail fixa da Moderna o x0 vai a ~218 e a 5a coluna entrava
// ~42 px por baixo do painel da direita. Quantas colunas cabem sai de
// vtColunas (x0 de ajustes_conteudo_x ate o painel); recolhida e Padrao
// continuam com 5, como antes.
static int vtColunas(void);
static float vtCardLargura(void);
static float vtCardAltura(void);
#define VT_COLS      vtColunas()
#define VT_COLS_MAX  5
#define VT_CARD_BASE_W (248.0f / gfx_escala_txt())
#define VT_CARD_W   vtCardLargura()
#define VT_CARD_H   vtCardAltura()
// .seeall-grid tinha 16; na TV o dono pediu mais ar entre os cartazes (13/10).
#define VT_GAP_X    (vtTelefone() ? 16.0f : 28.0f / gfx_escala_txt())
#define VT_GAP_Y    (20.0f + 40.0f)       // gap + a linha de titulo sob o cartaz
// GRADE na TV: sem o nome sob o cartaz (o painel ja diz tudo), so o vao.
#define VT_GRADE_GAP_Y (vtTelefone() ? VT_GAP_Y : 44.0f)
#define VT_TOPO    (collection ? 332.0f : 244.0f)
// LARGURA UTIL DA GRADE: as 5 colunas de sempre (5 x 248 + 4 x 16). Toda
// exibicao cabe nela, e o painel de detalhe a direita nunca e invadido.
#define VT_CW      vtLarguraUtil()
// A linha da filmografia (exibicao "Linha do tempo"): cartao deitado de 236.
#define VT_LINHA_H 236.0f
#define VT_TL_GAP  28.0f   // vao entre os cartoes da linha = o do seletor ao 1o (253+52 -> 332)

// PAINEL DE DETALHE, a direita. Medidas do web (.seeall-detail), que as ancora
// explicitamente na tela real de 1920x1080: top 170, largura 336. A margem
// direita e a da ilha do relogio (14/10, "margem componente solto"): o painel
// termina na mesma vertical que ela, nao 40 px antes.
//
// FIXO e nao rolando junto: o proprio CSS registra que `position: sticky` nao
// existe no Chromium 53 da TV e que sem `fixed` o painel sumia assim que o dono
// descia. Aqui nao ha fluxo nenhum — ele so nao recebe scrollY.
#define VT_PAN_W   336.0f
#define VT_PAN_X   (NV_TELA_W - NV_ILHA_MARGEM_D - VT_PAN_W)
#define VT_PAN_VAO  24.0f   // vao minimo entre a ultima coluna e o painel
#define VT_PAN_Y   VT_TOPO
#define VT_PAN_ART_H (330.0f / gfx_escala_txt())
#define VT_PAN_ART_W (220.0f / gfx_escala_txt())
static int   aberta, foco, pedAbrir = -1;
static int pedRelogio;
static float anim, scrollY, velY;
// 2.0.43: ms desde a abertura da Collection (herotexto.h, ht_ent_*). >= HT_ENT_TOTAL_MS = assentado
// (ajuste desligado, animacoes reduzidas, efeitos minimos e telefone entram assentados).
static float vitEntradaT = 1.0e6f;
float vertudo_entrada_ms(void) { return vitEntradaT; }
// 2.0.45: TRANSFORMACAO HOME -> COLLECTION (transicao.h). Com cartao de origem, um relogio adaptativo (vitTr)
// conduz tudo: o cartao cresce ate o retangulo do destaque da Vitrine e vira o fundo dela (o banner da pasta,
// a mesma textura do hero da Home), a Home recua, o cabecalho se forma durante o movimento e as fileiras entram
// quando o conteudo esta pronto (portao vitTrPronto, teto de 1000 ms). Sem cartao: a revelacao da 2.0.43.
static int vooTem;               // 1 enquanto a transformacao esta em cena
static int vooSaindo;            // Voltar no meio: tudo apaga com a Vitrine (sem voo inverso, decisao B da 2.0.44)
static int vooPedido;            // vertudo_colecao_voo -> vertudo_colecao
static GfxRect vooOrigem;        // onde o cartao estava na Home (espaco da Vitrine)
static float vooRaioPx;          // raio dos cantos dele (px)
static TrRelogio vitTr;
static GfxRect vooDestino;       // onde o destaque da Vitrine repousa (home_vitrine_arte_rect)
static int vooDestCobre;         // o destaque cobre a tela inteira (Moderna/Dinamica com banner)
static int vooDesenhado;         // o cartao ja foi desenhado neste quadro (dentro da Vitrine, sob os textos)
static int vooLogoTem;           // a logo da pasta sai do hero da Home e vai ao cabecalho (mesma imagem)
static GfxRect vooLogoDe;
static char vooLogoTex[NV_TEX_URL_MAX];
// World (19/11): sem logo, o NOME em texto faz o mesmo caminho (hero da Home -> cabecalho). Antes ele so
// apagava com a Home e o cabecalho acendia pequeno depois: nada ligava os dois (o "so mudou a velocidade").
static int vooNomeTem;
static GfxRect vooNomeDe;
static int vitBannerAtivo;       // D1: o banner da pasta e a camada de destaque ate o titulo entrar
static Uint32 vitBannerSolta;    // quando a transformacao acabou (o titulo entra quando a arte dele chega)
static Uint32 vitRevelaEm;       // quando a revelacao comecou (cartaz que chega depois entra com fade)
// World (20/11): CADA FILEIRA ENTRA QUANDO A DELA ESTA PRONTA. A cascata de 40 ms
// por fileira era um relogio fixo: a 2a fileira entrava vazia (so o titulo) se
// os cartazes dela ainda vinham. Agora, depois da revelacao, a fileira r acende
// no instante em que os primeiros cartazes visiveis dela chegaram (cheios ou
// previa de 128 px) -- nunca antes da anterior + 40 ms, para manter a ordem.
#define VIT_LIN_ESPERA_MAX 6
static Uint32 vitLinProntaEm[VIT_LIN_ESPERA_MAX];
// Cartazes das 3 primeiras fileiras revelados pela transformacao: o que ainda nao chegou fica FORA (nunca um
// cartao vazio) e entra sozinho com fade quando chega; passado VIT_CART_SEGURA_MS volta o esqueleto de sempre.
#define VIT_CART_FIL 3
#define VIT_CART_COL 42
#define VIT_CART_SEGURA_MS 3000u
static unsigned char vitCartEsc[VIT_CART_FIL][VIT_CART_COL];
static Uint32 vitCartChegou[VIT_CART_FIL][VIT_CART_COL];
static Uint32 vitAgora;          // relogio do quadro (vertudoCorpo)
// D2: pre-carga por permanencia (vertudo_prever): caminhos dos cartazes a pedir, um por quadro com o cache ocioso.
#define VIT_PREVER_MAX 30
static char vitPrever[VIT_PREVER_MAX][NV_TEX_URL_MAX];
static unsigned char vitPreverTent[VIT_PREVER_MAX];
static int vitPreverN, vitPreverI;
static float vitPreverW;
int home_pasta_logo(const ColFolder *f, GfxRect *r, const char **texUrl);   // home.c (2.0.45)
int home_pasta_nome(const ColFolder *f, GfxRect *r);   // home.c (World, 19/11)
static float vooP(void) { return tr_cartao(vitTr.p); }
static GfxRect vooRect(void) {
  float g = vooP(), q = 1.0f - g, e = tr_escala(vitTr.p);
  GfxRect d = vooDestino;
  float cx = d.x + d.w * 0.5f, cy = d.y + d.h * 0.5f;
  d.w *= e; d.h *= e; d.x = cx - d.w * 0.5f; d.y = cy - d.h * 0.5f;
  return (GfxRect){ vooOrigem.x * q + d.x * g, vooOrigem.y * q + d.y * g, vooOrigem.w * q + d.w * g, vooOrigem.h * q + d.h * g };
}
int vertudo_voo_ativo(float *p, GfxRect *rect) {
  if (!vooTem) return 0;
  if (p) *p = vooP();
  if (rect) *rect = vooRect();
  return 1;
}
int vertudo_tr_estado(float *p, float *r, float *msRevela, float *ms) {
  if (p) *p = vitTr.p;
  if (r) *r = vitTr.r;
  if (msRevela) *msRevela = vitTr.msRevela;
  if (ms) *ms = vitTr.ms;
  return vooTem ? vitTr.fase : TR_PARADO;
}
// A Home por baixo deixa de ser desenhada quando nada dela aparece: o cartao cobre a tela (Moderna/Dinamica)
// ou, com o destaque em faixa (Padrao), o fundo da Vitrine ja esta opaco e o cartao chegou a faixa.
int vertudo_cobre_tela(void) {
  if (!aberta) return 0;
  if (!vooTem) return 1;
  if (vooSaindo) return 0;
  if (vitTr.p >= TR_P_SAIDA) return 1;
  if (!vooDestCobre) return 0;
  GfxRect r = vooRect();
  return r.x <= 0.5f && r.y <= 0.5f && r.x + r.w >= NV_TELA_W - 0.5f && r.y + r.h >= NV_TELA_H - 0.5f;
}
int vertudo_home_oculta_origem(void) { return vooTem && !vooSaindo; }
float vertudo_home_recuo(float *textoVisivel) {
  float rec = 0.0f, tx = 1.0f;
  if (vooTem) {
    rec = tr_home_recuo(vitTr.p); tx = tr_home_texto(vitTr.p);
    if (vooSaindo) { rec *= anim; tx = 1.0f - (1.0f - tx) * anim; }
  }
  if (textoVisivel) *textoVisivel = tx;
  return rec;
}
// World 22/11 (dono: "a abertura da collection no layout padrao ta estranha"): no Padrao o
// banner cobre so a metade de cima; as fileiras da Home apagavam com o recuo e a metade de
// baixo ficava preta ate o conteudo chegar. Agora as fileiras da Home ficam ate a REVELACAO
// e cruzam com as da Colecao (o fundo escuro acende junto, tr_fundo_padrao).
static float tr_fundo_padrao(float r) { return tr_suave(r / 0.45f); }
float vertudo_home_fileiras(float recuo) {
  if (vooTem && !vooDestCobre && !vooSaindo) return 1.0f - tr_fundo_padrao(vitTr.r);
  return 1.0f - recuo;
}
#ifdef NV_TOUCH_UI
static int toqueLivre;
static ToqueRolagem toqueAbas;
static int toqueNasAbas;
#endif
static char  titulo[96];
static const ColFolder *collection;
static const char *wordmarkServico(void);
static int grupo(const char *g);
// World (19/11): o titulo da pasta (logo ou nome) esta VOANDO para o cabecalho: a Home nao desenha a copia
// dela (a copia da Home recua com a escala em torno do cartao e saia dupla, deslocada, no 1o quadro).
// As MESMAS condicoes em que o cabecalho desenha o voo (logoMove / ramo do nome em texto): esconder a copia
// da Home sem a Collection desenhar a dela faria o titulo sumir.
static int vooLogoVoa(void) { return vooLogoTem && collection && grupo("Streaming") && collection->logo[0]; }
static int vooNomeVoa(void) { return vooNomeTem && collection && !wordmarkServico()[0]; }
int vertudo_voo_titulo_ativo(void) { return vooTem && !vooSaindo && (vooLogoVoa() || vooNomeVoa()); }
static void vitCancelar(void);
static void vitManualLiberarSlot(void);
static int source, tabFocus, tabCursor, timeline, ranked;
static float tabRol;
static int vtTelefone(void) {
  return telefoneui_ativo();
}
static float vtEscala(void) { return vtTelefone() ? gfx_escala_ui() : gfx_escala_txt(); }
/* Keep measurements valid outside drawing too (focus, gestures and menus). */
#undef NV_TELA_W
#undef NV_TELA_H
#define NV_TELA_W (NV_LAYOUT_REAL_W / vtEscala())
#define NV_TELA_H (NV_LAYOUT_REAL_H / vtEscala())
static int vtTimelineRetrato(void) { return timeline && vtTelefone() && NV_TELA_H > NV_TELA_W; }
static float vtInicio(void) {
  return vtTelefone() ? 48.0f + ajustes_rail_largura_fixa() / vtEscala() : ajustes_conteudo_x();
}
// World 22/11 (TCL, "Coleção Ação" embaixo do avatar e da busca): com o MENU NO TOPO a faixa
// do cabecalho da colecao desce para baixo da barra (o mesmo que a Home Padrao faz com o
// destaque, menuTeto). Sem menu no topo, NV_ILHA_Y de sempre.
static float vtIlhaY(void) {
  MenuTopoGeo g;
  float base;
  if (vtTelefone() || !ajustes_menu_topo()) return NV_ILHA_Y;
  menu_topo_geo(&g);
  if (g.h <= 0.0f) return NV_ILHA_Y;
  base = (g.y + g.h) / vtEscala() + 14.0f;
  return base > NV_ILHA_Y ? base : NV_ILHA_Y;
}
static float vtFim(void) { return vtTelefone() ? NV_TELA_W - 48.0f : VT_PAN_X - VT_PAN_VAO; }
static float vtCabecalhoLargura(float x, float original) {
  if (!vtTelefone()) return original;
  float fim = NV_TELA_W - 48.0f;
  if (vtTimelineRetrato()) fim -= 180.0f;
  return fminf(original, fim - x);
}
static GfxRect vtTimelineCard(float cy) {
  float x = fminf(vtFim(), vtInicio() + (vtTimelineRetrato() ? 96.0f : 158.0f));
  float w = fmaxf(0.0f, vtFim() - x);
  if (vtTelefone()) w = fminf(1000.0f, w);
  return (GfxRect){x, cy, w, VT_CARD_H};
}
static GfxRect vtTimelineArte(GfxRect card) {
  float w = fminf(376.0f / gfx_escala_txt(), fminf(card.w * 0.36f,
                    (card.h - 24.0f) * 376.0f / 212.0f));
  return (GfxRect){card.x + 12.0f, card.y + 12.0f, w, w * 212.0f / 376.0f};
}
// Colunas que cabem entre o inicio do conteudo (a rail fixa entra por
// ajustes_conteudo_x, a fonte unica do recuo) e o painel, com VT_PAN_VAO de
// folga. Tira colunas, nao encolhe o cartaz (regra de ajustes_area_conteudo).
static float vtLarguraUtil(void) {
  float w = 5.0f * VT_CARD_W + 4.0f * VT_GAP_X;
  // #521: with the fixed rail only four columns fit left of the panel; the wide
  // layouts must shrink to what is left instead of running under it.
  return vtTelefone() ? w : fminf(w, vtFim() - vtInicio());
}
static int vtColunas(void) {
  if (timeline) return 1;
  if (vtTelefone()) {
    if (NV_TELA_H > NV_TELA_W) return 3;
    int n = (int)((vtFim() - vtInicio() + VT_GAP_X) / (VT_CARD_BASE_W + VT_GAP_X));
    return n < 1 ? 1 : n;
  }
  // #521/#205: contam as colunas que cabem em px REAIS (o cartaz sai da escala
  // de texto); contar na camada ampliada perderia uma coluna por quadro.
  float livre = (NV_LAYOUT_REAL_W - NV_CHROME_EDGE - VT_PAN_W - VT_PAN_VAO) / gfx_escala_txt() - ajustes_conteudo_x() + VT_GAP_X;
  int n = (int)(livre / (VT_CARD_BASE_W + VT_GAP_X) + 0.001f);
  return n < 1 ? 1 : n > VT_COLS_MAX ? VT_COLS_MAX : n;
}
static float vtCardLargura(void) {
  if (!vtTelefone() || timeline) return VT_CARD_BASE_W;
  return fmaxf(1.0f, (vtFim() - vtInicio() - (VT_COLS - 1) * VT_GAP_X) / VT_COLS);
}
static float vtCardAltura(void) { return timeline ? VT_LINHA_H : VT_CARD_W * 1.5f; }
static float tabAnim[COL_SOURCE_MAX];
static int order[VT_MAX], orderN=-1;
static char catalogId[96];

// --- EXIBICAO DA COLECAO ------------------------------------------------------
//
// Pedido do dono (06/10): "nas colecoes vamos colocar tipos diferentes onde a
// pessoa pode escolher, igual temos o dos diretores que e um timeline". A
// filmografia em linha do tempo era a UNICA variacao, e so os Diretores a
// tinham. Agora toda colecao escolhe a sua, no botao "Exibição" ao lado das
// abas, e a escolha fica gravada por perfil e por pasta.
//
// AUTOMATICO (layAuto): Diretores = linha do tempo; todas as outras pastas
// usam Grade. O ajuste global ou a escolha da pasta pode usar outra exibicao.
//
// TODAS AS EXIBICOES SAO A MESMA TELA: uma tabela de celulas (posicao e caixa
// de cada titulo, relativas ao inicio da grade) montada uma vez por mudanca, e
// a navegacao anda nela por LINHAS (celLin) e pelo centro mais proximo. Nenhuma
// conta de "colunas" sobrou no desenho nem no D-pad.
//
// CUSTO NA TV: nenhuma passada de blur, nenhuma textura nova por quadro. As
// artes saem do mesmo tex_cache por largura; Mural pede a arte deitada, que o
// cache ja tem das fileiras da home.
static const char *LAY_NOME[VT_LAY_N] = {
  "Padrão", "Grade", "Linha do tempo", "Ranking", "Saga", "Décadas", "Mural", "Vitrine",
  "Fluxo de capas"
};
static const char *LAY_ICONE[VT_LAY_N] = {
  "aj_sliders-horizontal", "aj_layout-dashboard", "aj_rows-3", "aj_list-ordered", "aj_layers", "aj_calendar", "aj_images",
  "aj_monitor-play", "aj_film"
};
// A ordem do botao "Exibição": a horizontal logo depois da vertical.
// "Fluxo de capas" (VT_LAY_FLUXO) foi aposentado: o id 8 continua reservado, mas
// nao e oferecido e uma escolha gravada com ele volta a "Padrão".
static const unsigned char LAY_CICLO[] = {
  VT_LAY_GRADE, VT_LAY_LINHA, VT_LAY_RANKING, VT_LAY_SAGA,
  VT_LAY_DECADAS, VT_LAY_MURAL, VT_LAY_VITRINE,
  VT_LAY_AUTO   // "Padrão": a pasta volta a seguir o ajuste do perfil (#535)
};
static int layEscolha;            // gravada (VT_LAY_AUTO = nunca escolheu)
// Animacao do foco (anel e zoom) entrando no cartaz novo e saindo do anterior.
static int vitFocoR = -1, vitFocoC = -1, vitFocoAntR = -1, vitFocoAntC = -1;
static float vitFocoF, vitFocoAntF;
// 2.0.40: AS MOLAS DA FILEIRA E DO FOCO NO TEMPO-BASE (NV_MOTION_MS, layout.h).
// "Navegação geral rápida" ligada mantem a razao de antes (1/0,85) sobre a
// desligada, e a "Velocidade da navegação" continua multiplicando. A rolagem
// vertical NAO muda aqui (etapa propria).
static float vitW2(void) {
  return NV_MOTION_W2 * ajustes_navegacao_velocidade() * (ajustes_navegacao_horizontal_rapida() ? 1.0f / 0.85f : 1.0f);
}
static float vitW1(void) {
  return NV_MOTION_W1 * ajustes_navegacao_velocidade() * (ajustes_navegacao_horizontal_rapida() ? 1.0f / 0.85f : 1.0f);
}
static float vitLinAlfa[COL_SOURCE_MAX];   // a fileira em foco acende, as outras esmaecem (animado)
static float vitLinEntra[COL_SOURCE_MAX];  // 0..1: a fileira sobe e acende quando os cartazes chegam
// TROCA DE COLLECTION COM A TELA ABERTA (Netflix -> Disney): o conteudo da atual
// sai (apaga e desce um pouco) sobre o fundo escuro, so entao a nova e montada e
// o destaque e as fileiras dela entram. `vitTrocaPend` e a pasta que espera.
static const ColFolder *vitTrocaPend;
static float vitTrocaSai;   // 0..1 da saida
static int vitTrocando;
static int semVitrineAuto;        // aberta por vertudo_abrir (ver layAuto)
// A pagina de uma fileira e temporaria: nunca altera a exibicao da colecao.
static int vitTudo, vitTudoFiltro, vitTudoGrupo, vitTudoEscolha, vitTudoSemAuto;
static int vitTudoFonteAntes, vitTudoLinha, vitTudoN, vitTudoBruto = -1, vitTudoMaisBruto = -1;
static unsigned vitTudoRev;
static SDL_Keycode vitTudoTecla;
static short vitTudoIdx[VT_MAX];
_Static_assert(VT_MAX <= 32767, "indices da fileira cabem em short");
static const ColFolder *vitTudoOrigem;
static ColFolder vitTudoPasta;
static ColSource vitTudoFonte;
static char vitTudoBase[NV_ADDON_URL_MAX], vitTudoNome[96];
static int vitVoltarTudo(void);
static int lay = VT_LAY_GRADE;    // a que esta na tela
static float pillAnim;            // foco no botao "Exibição"
// O seletor nasce recolhido (so a exibicao aberta); OK abre o trilho inteiro e
// `exibCursor` anda nele (indice em LAY_CICLO). Escolher, Voltar ou sair recolhe.
static int exibAberta, exibCursor;
// ESTILO DAS ABAS (#205). Menu suspenso: lista aberta e o item sob o cursor.
// Barra central: onde esta o realce que desliza. Paginas: o deslize do carrossel
// (+1 entrando da direita, -1 da esquerda, 0 em repouso).
static int   ddAberto, ddCursor;
static float ddAnim, barraX, barraW = -1.0f, barraAlvoX, barraAlvoW, pgSlide;
// CAPSULA DO FOCO QUE DESLIZA entre as abas de sempre (chips/topo): o mesmo
// modelo da barra central. capA e o alfa (foco na fileira de abas ou nao),
// capX/capW a posicao animada, capAlvoX/capAlvoW a aba com o cursor.
static float capX, capW = -1.0f, capAlvoX, capAlvoW, capA;
static void abasReiniciar(void);
static float celX[VT_MAX], celY[VT_MAX], celW[VT_MAX], celH[VT_MAX];
static short celLin[VT_MAX], celCol[VT_MAX];
static int   celN = -1, celLay = -1;
static int celTelefone = -1;
static float celAreaW, celTelaH;
static float celFim;              // altura do conteudo inteiro
// DECADAS: o cabecalho de cada secao (ano da decada, quantos titulos, y).
#define VT_DEC_MAX 40
#define VT_DEC_H   96.0f
static int   decN, decAno[VT_DEC_MAX], decQtd[VT_DEC_MAX];
static float decY[VT_DEC_MAX];
// SAGA: a trilha de capitulos acima de cada linha de cartoes.
#define VT_SAGA_TRILHA 44.0f
static float x0Atual;
// SAGA: quem ja foi visto, conferido na montagem e a cada 1,5 s (a volta do
// player marca um capitulo). Nunca por quadro: cada consulta copia o CatItem.
static unsigned char celVisto[VT_MAX];
static int sagaVistos;
// "Voce viu N de M" do cabecalho: titulos distintos (tt/tmdb/episodio) da lista.
static int vcVistos, vcTotal;
// Geometria do ultimo cabecalho desenhado (sobretitulo, nome) e pontos do anel:
// os testes de captura conferem o arranjo sem ler pixel.
static GfxRect cabEb, cabTt, cabSw;
static int cabPontos;
static float cabFim, vitHeroTopo;   // Vitrine: pe do cabecalho e topo do bloco do destaque
static float cabCy = 218.0f;        // centro da linha do anel e do seletor
static Uint32 vistosEm;             // ajustes_conteudo_x() do quadro (desenho)

// LINHA DO TEMPO ANIMADA (13/10): o foco de cada ano (0..1, mola) e o y do
// preenchimento da linha (coordenada da grade; < 0 = posicionar sem animar).
// Floats por quadro, nenhuma textura; a mola ja respeita a animacao reduzida.
static float tlFoco[VT_MAX], tlProg = -1.0f;
#define VT_TL_PONTO (VT_LINHA_H * 0.5f)   // centro do ponto do ano: o centro do cartao

// LINHA DO TEMPO HORIZONTAL (Cover Flow, 13/10; 14/10 sem inclinacao): o cartaz
// em foco no centro, grande; os vizinhos RETOS atras dele, menores, escuros e
// sobrepostos (profundidade so por escala, recobrimento e veu). A faixa vai de
// ponta a ponta e a ficha do titulo fica ABAIXO da linha do tempo. `fluxoPos` e
// o foco em mola (< 0 = posicionar sem animar).
static float fluxoPos = -1.0f;
static float fluxoFim(void) { return vtTelefone() ? vtFim() : NV_TELA_W - 48.0f; }
static float fluxoAltura(void) {
  if (vtTelefone()) return 300.0f / gfx_escala_txt();
  float h = NV_TELA_H - VT_TOPO;
  // Reserve the year label, title/logo, metadata and synopsis below the covers.
  return fminf((fluxoFim() - vtInicio()) * .35f * 1.5f,
               fminf(h * .60f, h * .98f - 76.0f - 158.0f));
}
#define VT_FLX_H fluxoAltura()
#define VT_FLX_W (VT_FLX_H / 1.5f)
static float fluxoTopo(void) { return VT_TOPO + (vtTelefone() ? 40.0f : (NV_TELA_H - VT_TOPO) * .01f); }
static float fluxoCentro(float x0) { return (x0 + fluxoFim()) * 0.5f; }
static float fluxoLinhaY(void) { return fluxoTopo() + VT_FLX_H + (vtTelefone() ? 64.0f : (NV_TELA_H - VT_TOPO) * .01f); }
static float fluxoInfoY(void) { return fluxoLinhaY() + 76.0f; }
// Caixa do titulo i e quanto ele escurece (0 no foco). O 6o vizinho cai na ponta.
static GfxRect fluxoRect(int i, float x0, float *sombra) {
  float W = VT_FLX_W, H = VT_FLX_H, d = i - fluxoPos, ad = fabsf(d), s = fminf(ad, 1.0f);
  float A = W * 0.78f, alem = fmaxf(0.0f, ad - 1.0f);
  float S = fmaxf(W * 0.3f, (fluxoFim() - x0) * 0.5f - A) / 5.0f * 0.97f;
  float off = ad < 1.0f ? d * A : copysignf(A + alem * S, d);
  float k = fmaxf(0.4f, 1.0f - 0.18f * s - 0.07f * alem), w = W * k, h = H * k;
  if (sombra) *sombra = fminf(0.85f, 0.5f * s + 0.1f * alem);
  return (GfxRect){ fluxoCentro(x0) + off - w * 0.5f, fluxoTopo() + (H - h) * 0.5f, w, h };
}

static int ordenaPorAno(void) {
  return lay == VT_LAY_LINHA || lay == VT_LAY_FLUXO || lay == VT_LAY_SAGA || lay == VT_LAY_DECADAS;
}
// MICRO-ANIMACOES (revela.h): a arte chegando por cartaz e a onda da grade.
// A onda e ARMADA quando uma grade nova e pedida (abrir, trocar de fonte) e
// dispara no primeiro quadro que tem itens — rolar ou voltar do detalhe nao
// a repete.
static RevelaArte revArte[VT_MAX];
static Uint32 ondaEm;
static int ondaArmada;
static void armarOnda(void) {
  ondaArmada = 1; ondaEm = 0;
  memset(revArte, 0, sizeof revArte);
}
// A fonte escolhida NAO TEM ENDERECO ainda: o addon dela nao esta instalado
// nesta TV, ou a sonda de manifesto ainda nao respondeu. Ver openSource.
static int semFonte;
static int grupo(const char *nome) {
  return collection && !strcmp(collection->group, nome);
}
static void corColecao(float *r,float *g,float *b) {
  col_cor(collection,r,g,b);
  // Serviços preservam sua marca. Famílias editoriais sem marca própria usam
  // cor apenas em seleção/estado, mantendo as superfícies neutras.
  if (grupo("Genres")) {*r=.075f;*g=.34f;*b=.285f;}
  else if (grupo("Themes")) {*r=.34f;*g=.13f;*b=.38f;}
  else if (grupo("Film Collections") || grupo("TV Collections"))
    {*r=.12f;*g=.27f;*b=.40f;}
}

// SERVICO DE STREAMING (pedido do dono, 13/10): sem o rotulo "STREAMING" no
// topo, o wordmark do servico no lugar do titulo e o degrade na cor da marca.
// O wordmark e o da pasta, ou o do TMDB; sem nenhum, fica o titulo em texto.
static const MarcaStream *marcaServico(void) {
  return grupo("Streaming") ? marca_stream(collection->title) : NULL;
}
static const char *wordmarkServico(void) {
  const char *u;
  if (!grupo("Streaming")) return "";
  if (collection->logo[0]) return collection->logo;
  u = marca_stream_logo(marcaServico());
  return u;
}
// O degrade do destaque na cor da marca, por cima da arte e sob o texto. Cor
// ja escura (marcastream.h): o branco do texto segue passando de 4,5:1. `h` e
// a altura: a faixa do cabecalho, ou a tela toda sob a arte inteira da Dinamica.
static void degradeMarca(float a, float h) {
  const MarcaStream *m = marcaServico();
  float c[3];
  CorvivaPaleta p;
  const char *w;
  if (m) { c[0] = m->r; c[1] = m->g; c[2] = m->b; }
  else {
    // Servico fora da tabela: a cor sai do wordmark (corviva), escurecida.
    if (!grupo("Streaming") || !*(w = wordmarkServico()) || !corviva_paleta(w, &p) || !p.ok) return;
    c[0] = p.grad[2][0] * 0.55f; c[1] = p.grad[2][1] * 0.55f; c[2] = p.grad[2][2] * 0.55f;
  }
  // A BASE DA FAIXA ESVANECE EM ALFA (uPar.x = onde o esvanecer comeca, em
  // fracao da altura): o degradê e a cor da marca sobre o Frost, e sem isso a
  // borda inferior era um degrau duro no meio da tela (Netflix na TCL: vermelho
  // em cima, teal embaixo, corte seco a ~48% da altura).
  { float faixas = fminf(240.0f, h * 0.5f);
    gfx_rect((GfxRect){ 0.0f, 0.0f, NV_TELA_W, h }, 0, GFX_FUNDO_DIN, 0.15f,
             1.0f - faixas / h, 0, 0, c[0], c[1], c[2], 0.78f * a); }
}

// As abas de fonte pertencem ao mesmo fluxo de selecao da folha de fontes.
// Mantem o acento reconhecivel, mas o mistura ao fundo para nao virar uma
// faixa azul/branca solta sobre a arte. A excecao para tons quase brancos
// preserva a leitura da tinta escura nos temas Branco e Grafite.
// Desde os acentos de 03/10 e o proprio acento (ver botao_cor_foco).
static void corFocoFonte(float *r, float *g, float *b) {
  ajustes_acento(r, g, b);
}

static void focoAbaFonte(GfxRect r, float f, float a) {
  float ar, ag, ab, sr, sg, sb;
  GfxRect luz;
  if (f <= 0.01f) return;
  ajustes_acento(&ar, &ag, &ab);
  luz = (GfxRect){r.x - r.h * 0.45f, r.y - r.h * 0.45f,
                  r.w + r.h * 0.90f, r.h * 2.0f};
  gfx_rect(luz, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
           ar, ag, ab, 0.16f * f * a);
  corFocoFonte(&sr, &sg, &sb);
  gfx_cor(r, NV_RAIO_PILL, sr, sg, sb, f * a);
  gfx_rect(r, 0, GFX_BRILHO_TOPO, NV_RAIO_PILL, 0.24f, 0, 0.5f,
           1, 1, 1, 0.10f * f * a);
}
static const char *rotuloGrupo(void) {
  if (grupo("Directors")) return "FILMOGRAFIA";
  if (grupo("Awards")) return "PRÊMIOS E CÂNONE";
  if (grupo("Genres")) return "GÊNERO";
  if (grupo("Themes")) return "TEMA";
  if (grupo("Streaming")) return "STREAMING";
  if (ranked) return "RANKING";
  return "COLEÇÃO";
}
static const char *legendaGrupo(void) {
  if (lay == VT_LAY_LINHA || lay == VT_LAY_FLUXO) return grupo("Directors") ? "Filmografia em ordem cronológica"
                                                    : "Em ordem cronológica";
  if (lay == VT_LAY_SAGA) return "Saga em ordem de lançamento";
  if (lay == VT_LAY_DECADAS) return "Organizado por década";
  if (grupo("Awards")) return ranked ? "Ranking na ordem original" : "Lista de prêmios configurada pelo usuário";
  if (grupo("Genres")) return "Títulos do gênero no seu catálogo";
  if (grupo("Themes")) return "Uma curadoria por tema e atmosfera";
  if (grupo("Streaming")) return "Catálogo organizado por serviço";
  return ranked ? "Ordem original do ranking" : "Seleção do seu catálogo";
}

// 9999 sem ano: na ordem cronologica o titulo sem data vai para o fim.
static int yearOf(const CatItem *it) {
  int a = nv_ano_meta(it->meta);
  return a ? a : 9999;
}
static int nItens(void);
static int itemDaLista(int i, CatItem *out) {
  if (vitTudo && vitTudoFiltro) {
    if (i < 0 || i >= nItens()) return 0;
    i = vitTudoIdx[i];
  }
  return desc_vertudo_item(i, out);
}
static int viewItem(int i,CatItem *out) {return itemDaLista(ordenaPorAno()&&i<orderN?order[i]:i,out);}

// O foco pertence ao titulo, nao ao indice: ocultar um filme pode trocar a
// lista sem mudar seu tamanho. Chaves pequenas evitam guardar outro CatItem.
typedef struct { char id[64], tipo[8]; long tmdb; } VtChave;
static VtChave focoChave;
static unsigned gradeRevisao, gradePonteiroRevisao;
static int gradeRevisaoValida, gradePonteiroValido, focoLembrado = -1;
static void guardarChave(VtChave *chave, const CatItem *it) {
  memset(chave, 0, sizeof *chave);
  if (!it) return;
  snprintf(chave->id, sizeof chave->id, "%s", it->imdb);
  snprintf(chave->tipo, sizeof chave->tipo, "%s", it->tipo);
  chave->tmdb = it->tmdb;
}
static int mesmaChave(const VtChave *chave, const CatItem *it) {
  if (strcmp(chave->tipo, it->tipo)) return 0;
  if (chave->id[0]) return !strcmp(chave->id, it->imdb);
  return chave->tmdb > 0 && chave->tmdb == it->tmdb;
}
static void lembrarFoco(void) {
  CatItem it;
  if (foco == focoLembrado) return;
  guardarChave(&focoChave, viewItem(foco, &it) ? &it : NULL);
  focoLembrado = foco;
}
static void limparFocoCatalogo(void) {
  memset(&focoChave, 0, sizeof focoChave);
  gradeRevisaoValida = 0;
  gradePonteiroValido = 0;
  focoLembrado = -1;
}
static int atualizarGrade(void);

// --- A ESCOLHA, por perfil e por pasta ---------------------------------------
// Arquivo proprio ("<id da colecao>\t<exibicao>" por linha), gravado por
// dados_gravar como o resto: no Tizen e o que leva a escolha para o IndexedDB.
// Nunca vai para a conta — e escolha deste aparelho, como as fileiras.
#define VT_ESC_MAX 160
static const char *arquivoEscolha(void) {
  static char nome[48];
  snprintf(nome, sizeof nome, "colecao-exibicao-p%d.txt", salvos_perfil_atual());
  return nome;
}
static void chavePasta(const ColFolder *f, char *dst, size_t n) {
  snprintf(dst, n, "%s/%s", f->groupId[0] ? f->groupId : f->group,
           f->id[0] ? f->id : f->title);
  for (char *p = dst; *p; p++) if (*p == '\t' || *p == '\n') *p = ' ';
}
// Le o arquivo inteiro: `chaves`/`vals` recebem ate VT_ESC_MAX linhas.
static int lerEscolhas(char (*chaves)[160], int *vals) {
  char cam[600], buf[300];
  int n = 0;
  FILE *f;
  if (!dados_caminho(cam, sizeof cam, arquivoEscolha())) return 0;
  if (!(f = fopen(cam, "r"))) return 0;
  while (n < VT_ESC_MAX && fgets(buf, sizeof buf, f)) {
    char *tab = strrchr(buf, '\t');
    if (!tab || buf[0] == '#') continue;
    *tab = 0;
    snprintf(chaves[n], 160, "%s", buf);
    vals[n] = atoi(tab + 1);
    if (vals[n] > VT_LAY_AUTO && vals[n] < VT_LAY_N && vals[n] != VT_LAY_FLUXO) n++;
  }
  fclose(f);
  return n;
}
static int escolhaGravada(const ColFolder *pasta) {
  static char chaves[VT_ESC_MAX][160];
  int vals[VT_ESC_MAX], n, i;
  char k[160];
  if (!pasta) return VT_LAY_AUTO;
  chavePasta(pasta, k, sizeof k);
  n = lerEscolhas(chaves, vals);
  for (i = 0; i < n; i++) if (!strcmp(chaves[i], k)) return vals[i];
  return VT_LAY_AUTO;
}
static void gravarEscolha(const ColFolder *pasta, int v) {
  static char chaves[VT_ESC_MAX][160];
  int vals[VT_ESC_MAX], n, i, achou = 0;
  char k[160], *txt;
  size_t cap = (size_t)(VT_ESC_MAX + 2) * 172, used = 0;
  if (!pasta) return;
  chavePasta(pasta, k, sizeof k);
  n = lerEscolhas(chaves, vals);
  for (i = 0; i < n; i++) if (!strcmp(chaves[i], k)) {
    achou = 1;
    if (v == VT_LAY_AUTO) {
      memmove(chaves + i, chaves + i + 1, sizeof chaves[0] * (size_t)(n - i - 1));
      memmove(vals + i, vals + i + 1, sizeof vals[0] * (size_t)(n - i - 1));
      n--; i--; continue;
    }
    vals[i] = v;
  }
  // Cheio: a mais antiga sai (a lista e um historico, nao um cadastro).
  if (!achou && v != VT_LAY_AUTO) {
    if (n == VT_ESC_MAX) { memmove(chaves, chaves + 1, sizeof chaves[0] * (VT_ESC_MAX - 1));
                           memmove(vals, vals + 1, sizeof vals[0] * (VT_ESC_MAX - 1)); n--; }
    snprintf(chaves[n], 160, "%s", k); vals[n++] = v;
  }
  if (!(txt = malloc(cap))) return;
  used = (size_t)snprintf(txt, cap, "# Exibicao de cada colecao neste perfil (vertudo.c).\n");
  for (i = 0; i < n && used < cap; i++)
    used += (size_t)snprintf(txt + used, cap - used, "%s\t%d\n", chaves[i], vals[i]);
  if (used < cap) dados_gravar(arquivoEscolha(), txt);
  free(txt);
}

// AUTOMATICO, pelo conteudo da pasta. Ver o cabecalho de VtLayout.
static int layAuto(void) {
  if (!collection) return VT_LAY_GRADE;
  // A Collection do Original escolhe entre fileiras e grade. FOLLOW_LAYOUT
  // acompanha o layout da Home; ROWS tambem usa a representacao por fileiras.
  if (!semVitrineAuto) {
    // FOLLOW_LAYOUT segue a preferencia de exibicao do proprio Nuvio.
    // ROWS força uma fileira por subcolecao; TABBED_GRID usa a grade com abas.
    if (collection->viewMode == COL_VIEW_ROWS) return VT_LAY_VITRINE;
    if (collection->viewMode == COL_VIEW_TABBED_GRID) return VT_LAY_GRADE;
    int padrao = ajustes_colecao_exibicao();
    if (padrao > VT_LAY_AUTO && padrao < VT_LAY_N) return padrao;
  }
  if (grupo("Directors")) return VT_LAY_LINHA;
  // Pastas antigas sem viewMode conservam o padrao que o World ja usava.
  return VT_LAY_GRADE;
}
static void resolverLayout(void) {
  int novo = layEscolha > VT_LAY_AUTO && layEscolha < VT_LAY_N ? layEscolha : layAuto();
  if (novo != lay) {
    if (lay == VT_LAY_VITRINE) vitCancelar();
    lay = novo; celLay = -1; orderN = -1;
  }
  timeline = lay == VT_LAY_LINHA;
}
// O ENDERECO DA FONTE, que pode nao existir ainda.
//
// Uma pasta que veio da CONTA guarda o addonId (o "id" do manifesto) e, muitas
// vezes, nenhuma URL: quem resolve e o mapa de addons, e ele so conhece o id
// depois que a sonda leu o manifesto daquele addon. Uma pasta que veio do
// PACOTE ja traz a base escrita.
static const char *baseDaFonte(const ColSource *s) {
#if defined(NV_TPK40) || defined(NV_UM_FIO)
  static char base[NV_ADDON_URL_MAX]; // no compiler TLS on Tizen 4/5 (rede.h)
#else
  static _Thread_local char base[NV_ADDON_URL_MAX];
#endif
  // A URL so autoriza a fonte se corresponder a um addon atualmente ativo.
  // Quando ha addonId, ele vence e resolve para a base da instalacao atual.
  if (!col_fonte_base_ativa(s, base, sizeof base)) base[0] = 0;
  return base;
}
static int fonteDisponivel(const ColSource *s, const char *base) {
  if (!s) return 0;
  if (!strcmp(s->prov, "tmdb")) return desc_chave_tmdb()[0] != 0;
  if (!strcmp(s->prov, "trakt")) return nuvem_trakt_cliente()[0] != 0;
  if (s->prov[0]) return 0;
  return base && base[0] && desc_catalogo_resolver(base, s->type, s->catId, NULL);
}
static const char *mensagemSemFonte(const ColSource *s) {
  if (s && !strcmp(s->prov, "tmdb"))
    return i18n("O TMDB não está configurado nesta TV.");
  if (s && !strcmp(s->prov, "trakt"))
    return i18n("O Trakt não está configurado nesta TV.");
  AddonIdentidadeEstado estado = addons_identidade_estado(s ? s->addonId : "");
  if (estado == ADD_IDENTIDADE_PENDENTE || estado == ADD_IDENTIDADE_PRONTA)
    return i18n("Carregando…");
  if (estado == ADD_IDENTIDADE_FALHOU)
    return i18n("Não deu para carregar");
  return i18n("O addon desta coleção não está instalado nesta TV.");
}

static void vitIniciar(void);
static float vitPasso(void);
static void carregarFonte(void);
static int vitPaginaFonte(void);
static void vitPedirMais(void);
static void vitFocarLinha(int linha);
static void openSource(void) {
  const ColSource *s=&collection->sources[source];
  snprintf(catalogId,sizeof catalogId,"%s",s->catId);
  ranked=strstr(s->catId,"top100")||strstr(s->catId,"top250")||strstr(s->catId,"top10");
  foco=0;scrollY=velY=0;orderN=-1;celLay=-1;armarOnda();
  limparFocoCatalogo();
  resolverLayout();
  // VITRINE: cada aba vira uma fileira, com pedidos independentes e limitados.
  if (lay == VT_LAY_VITRINE) { vitIniciar(); return; }
  carregarFonte();
}
// Pede ao carregador da descoberta o catalogo de `source` (sem mexer na tela).
static void carregarFonte(void) {
  const ColSource *s=&collection->sources[source];
  const char *base=vitTudo ? vitTudoBase : baseDaFonte(s);
  snprintf(catalogId,sizeof catalogId,"%s",s->catId);
#ifdef NV_TOUCH_UI
  toqueLivre = 0;
  toquerol_limpar(&toqueAbas);
#endif
  // TMDB e Trakt sao fontes nativas da Collection, nao catalogos de addon.
  // O conteudo e buscado pelos resolvedores diretos que ja atendem o Ver Tudo.
  if (s->prov[0]) {
    snprintf(catalogId, sizeof catalogId, "%s", s->title);
    semFonte = !fonteDisponivel(s, base);
    if (semFonte) { desc_vertudo_fechar(); return; }
    if (lay == VT_LAY_VITRINE) desc_vertudo_fonte_desde(s, vitPaginaFonte());
    else desc_vertudo_fonte(s);
    return;
  }
  // BASE VAZIA NAO VIRA PEDIDO.
  //
  // desc_vertudo_filtro so recusa ponteiro NULO, e `base` e um vetor dentro da
  // ColSource — nunca nulo, as vezes vazio. Com ele vazio a URL montada virava
  // "/catalog/movie/<id>.json", um caminho RELATIVO: no Tizen o XHR resolve
  // isso contra a origem do proprio widget, a resposta e o index.html e nada
  // decodifica. O resultado na tela era a colecao abrindo sem um unico cartaz e
  // sem explicacao (issue #10). Agora a tela DIZ o que falta.
  semFonte = !base[0];
  if (semFonte) {
    // Limpa a pagina global anterior: fonte sem endereco nao pode continuar
    // exibindo os resultados da fonte que estava aberta antes.
    desc_vertudo_filtro_desde("", s->type, s->catId, s->genre, vitPaginaFonte());
    printf("[vertudo] fonte sem endereco: addonId=\"%s\" cat=%s/%s\n",
           s->addonId, s->type, s->catId);
    fflush(stdout);
    return;
  }
  desc_vertudo_filtro_desde(base,s->type,s->catId,s->genre,vitPaginaFonte());
}

static int cursorInicial(void);
void vertudo_colecao(const ColFolder *folder) {
  int i;
  pedRelogio = 0;
  if(!folder||!folder->nSources)return;
  // Ja aberta na Vitrine e na TV: primeiro a saida da atual (vertudo_atualizar).
  if (!vitTrocando && aberta && anim > 0.9f && lay == VT_LAY_VITRINE && !vtTelefone()) {
    if (!vitTrocaPend) vitTrocaSai = 0.0f;
    vitTrocaPend = folder;
    return;
  }
  vitTrocaPend = NULL; vitTrocaSai = 0.0f;
  vitTudo = 0; vitTudoTecla = 0;
  vitCancelar();
  memset(tabAnim, 0, sizeof tabAnim);
  tabRol = 0.0f;   // a faixa renasce no logo, sem herdar a rolagem da pasta anterior
  collection=folder;source=tabCursor=0;tabFocus=folder->nSources>1;aberta=1;pedAbrir=-1;
  pillAnim=0;exibAberta=0;vcTotal=0;abasReiniciar();layEscolha=escolhaGravada(folder);semVitrineAuto=0;
  // COMECAR NA PRIMEIRA FONTE QUE TEM ENDERECO, e nao teimosamente na fonte 0.
  // Numa pasta com varias abas e comum que so parte dos addons esteja instalada
  // aqui; abrir na aba morta faz a pasta inteira parecer vazia.
  for (i = 0; i < folder->nSources; i++)
    if (folder->sources[i].prov[0] || baseDaFonte(&folder->sources[i])[0])
      { source = tabCursor = i; break; }
  snprintf(titulo,sizeof titulo,"%s",folder->title);openSource();
  // O cursor do cabecalho nasce na posicao certa (no Menu suspenso e o botao
  // do catalogo, nao o indice da fonte aberta), senao o OK nasce em "Exibição"
  // quando a primeira fonte com endereco e a quinta ou mais (#205).
  tabCursor = cursorInicial();
  if(lay==VT_LAY_VITRINE)tabFocus=0;   // sem barra de abas: o foco nasce nas fileiras
  { const int anima = ajustes_col_entrada() && !ajustes_animacoes_reduzidas() && !gfx_efeitos_minimos() && !vtTelefone();
    vooTem = anima && vooPedido;   // sem cartao de origem: so a revelacao da 2.0.43 (relogio vitEntradaT)
    vooSaindo = 0; vitBannerAtivo = 0; vitRevelaEm = 0; vooLogoTem = 0; vooNomeTem = 0;
    memset(vitLinProntaEm, 0, sizeof vitLinProntaEm);
    memset(vitCartEsc, 0, sizeof vitCartEsc); memset(vitCartChegou, 0, sizeof vitCartChegou);
    if (vooTem) {
      const float k = 1.0f / vtEscala();   // a origem chega em coordenadas da TELA; a Vitrine desenha na escala dela
      float esc = gfx_escala();
      GfxRect lg; const char *lgTex = NULL;
      tr_iniciar(&vitTr);
      vitEntradaT = 1.0e6f;   // a revelacao da 2.0.43 fica de fora: quem manda e vitTr
      vitBannerAtivo = 1; vitBannerSolta = 0;
      vooOrigem.x *= k; vooOrigem.y *= k; vooOrigem.w *= k; vooOrigem.h *= k; vooRaioPx *= k;
      gfx_escala_sair(vtEscala()); home_vitrine_arte_rect(&vooDestino); gfx_escala_sair(esc);
      if (home_pasta_logo(folder, &lg, &lgTex) && lgTex) {
        vooLogoTem = 1; vooLogoDe = (GfxRect){ lg.x * k, lg.y * k, lg.w * k, lg.h * k };
        snprintf(vooLogoTex, sizeof vooLogoTex, "%s", lgTex);
      } else if (home_pasta_nome(folder, &lg)) {
        vooNomeTem = 1; vooNomeDe = (GfxRect){ lg.x * k, lg.y * k, lg.w * k, lg.h * k };
      }
      vitPreverN = 0;   // a pre-carga por permanencia acabou: daqui em diante e demanda
    } else { tr_parar(&vitTr); vitEntradaT = !anima ? 1.0e6f : HT_VOO_SEM_CARTAO_MS; }
  }
  vooPedido = 0;
}
void vertudo_colecao_voo(const ColFolder *folder, GfxRect origem, float raioPx) {
  vooPedido = !aberta && folder && folder->nSources > 0;   // com a Vitrine ja aberta e troca de Collection, sem cartao
  vooOrigem = origem; vooRaioPx = raioPx;
  vertudo_colecao(folder);
  vooPedido = 0;
}

void vertudo_abrir(const char *base, const char *tipo, const char *catId,
                   const char *tit) {
  vitTrocaPend = NULL;
  vitTudo = 0; vitTudoTecla = 0;
  vitCancelar();
  pedRelogio = 0;
  aberta = 1; foco = 0; scrollY = 0.0f; velY = 0.0f; pedAbrir = -1;
#ifdef NV_TOUCH_UI
  toqueLivre = 0;
  toquerol_limpar(&toqueAbas);
#endif
  tabRol = 0.0f;
  memset(tabAnim, 0, sizeof tabAnim);
  snprintf(titulo, sizeof titulo, "%s", tit ? tit : "");
  collection=col_por_catalogo(base,tipo,catId);
  limparFocoCatalogo();
  source=tabCursor=tabFocus=0;orderN=-1;celLay=-1;pillAnim=0;exibAberta=0;vcTotal=0;abasReiniciar();
  layEscolha=escolhaGravada(collection);semVitrineAuto=1;
  if(layEscolha==VT_LAY_VITRINE)layEscolha=VT_LAY_AUTO;   // "Ver tudo" e uma fileira so
  if(collection) {
    for(int i=0;i<collection->nSources;i++)if(!strcmp(collection->sources[i].catId,catId)&&!strcmp(collection->sources[i].type,tipo)){source=tabCursor=i;break;}
    snprintf(titulo,sizeof titulo,"%s",collection->title);
  }
  snprintf(catalogId,sizeof catalogId,"%s",catId);
  ranked=strstr(catId,"top100")||strstr(catId,"top250")||strstr(catId,"top10");
  armarOnda();
  // Sem pasta (o "Ver tudo" de uma fileira de catalogo) a tela e a grade de
  // sempre: a escolha de exibicao e coisa de colecao.
  resolverLayout();
  desc_vertudo_abrir(base, tipo, catId);
}

int vertudo_aberta(void) { return aberta; }
int vertudo_foco(void) { return foco; }
void vertudo_fechar_seco(void) { vooTem = 0; vooSaindo = 0; vitBannerAtivo = 0; tr_parar(&vitTr); vitTrocaPend = NULL; vitTudo = 0; vitTudoTecla = 0; vitCancelar(); aberta = 0; anim = 0.0f; pedRelogio = 0; }
int vertudo_pediu_abrir(void) { int v = pedAbrir; pedAbrir = -1; return v; }
// ESQUERDA na coluna 0 da grade (ou na primeira aba da colecao): a barra
// lateral abre POR CIMA da lista (dono, 03/10). Antes a coluna 0 voltava para
// o ultimo da linha de cima. app.c le no mesmo evento.
static int pedMenu;
int vertudo_pediu_menu(void) { int v = pedMenu; pedMenu = 0; return v; }
int vertudo_pediu_relogio(void) { int v = pedRelogio; pedRelogio = 0; return v; }

static int nItens(void) {
  int n = desc_vertudo_n();
  if (!vitTudo || !vitTudoFiltro) return n;
  unsigned rev = desc_vertudo_revisao();
  if (n == vitTudoBruto && rev == vitTudoRev) return vitTudoN;
  vitTudoN = 0; vitTudoBruto = n; vitTudoRev = rev;
  if (n > VT_MAX) n = VT_MAX;
  for (int i = 0; i < n; i++) {
    CatItem it;
    if (!desc_vertudo_item(i, &it)) continue;
    int grupo = vitTudoFiltro == 1 ? !strcmp(it.tipo, "series") : nv_ano_meta(it.meta) / 10 * 10;
    if (grupo == vitTudoGrupo) vitTudoIdx[vitTudoN++] = (short)i;
  }
  return vitTudoN;
}
// Uma pagina sem membros ainda nao e uma fileira vazia. Continua pelo cursor
// normal, mas nao repete automaticamente se o leitor deixar de crescer.
static void vitTudoAvancar(int n) {
  int bruto = desc_vertudo_n_bruto();
  if (vitTudo && vitTudoFiltro && bruto < VT_MAX && bruto != vitTudoMaisBruto &&
      vertudo_quer_mais(0, n, foco, VT_MAX, desc_vertudo_carregando(),
                        desc_vertudo_fim(), desc_vertudo_erro())) {
    vitTudoMaisBruto = bruto;
    desc_vertudo_mais();
  }
}

// O preenchimento automatico de vistos e limitado. Se nenhuma dessas paginas
// trouxe um cartaz visivel, so a pessoa pode pedir o proximo bloco de paginas.
static int vitTemMaisVistos(void);
static void vitFocarMais(void);
static int temMaisVistos(void) {
  if (lay == VT_LAY_VITRINE) return vitTemMaisVistos();
  return ajustes_ocultar_filmes_vistos() &&
         !semFonte && (lay == VT_LAY_VITRINE ? vitTemMaisVistos() : !nItens()) && desc_vertudo_n_bruto() > 0 &&
         !desc_vertudo_fim() && !desc_vertudo_erro() && !desc_vertudo_carregando();
}
static void ponteiroMaisVistos(int a, int b) {
  (void)b;
  if (temMaisVistos()) {
    tabFocus = 0;
    if (lay == VT_LAY_VITRINE) vitFocarLinha(a);
  }
}
static void carregarMaisVistos(int a, int b) {
  (void)b;
  if (!aberta || !temMaisVistos()) return;
  tabFocus = 0;
  if (lay == VT_LAY_VITRINE) { vitFocarLinha(a); vitPedirMais(); }
  else desc_vertudo_mais();
}

// O OK NUM CARTAO: abre a pagina do titulo (o toque curto) ou, SEGURANDO, o menu
// do cartaz (ctxlista.h). O item da grade NAO esta no catalogo global — ele veio
// de uma pagina que so esta tela leu. Entra por cat_acrescentar para que a tela
// de titulo possa abri-lo por indice, que e como todo o app trabalha.
static CtxHold hold;

static void abrirItem(const CatItem *src);
static void abrirFocado(void) {
  CatItem it;
  if (viewItem(foco, &it)) abrirItem(&it);
}

// Retangulo e arte do cartao em foco, na tela virtual: o menu nasce ao lado e
// o cartaz volta por cima do veu, como na home. Le a tabela de celulas, a mesma
// do desenho, entao vale para toda exibicao.
static GfxRect celulaRect(int i, const CatItem *it, const char **arte) {
  float x0 = vtInicio();
  float cx = x0 + celX[i], cy = VT_TOPO + celY[i] - scrollY;
  if (lay == VT_LAY_LINHA) {
    *arte = posterprov_card_formato(it->origem, it->imdb, it->tmdb, it->tipo,
                                   it->backdrop[0] ? it->backdrop : it->poster, 1);
    return vtTimelineArte(vtTimelineCard(cy));
  }
  { const char *pp = posterprov_card_addon(it->origem, it->imdb, it->tmdb, it->tipo, it->poster);
    *arte = pp[0] ? pp : it->backdrop; }
  if (lay == VT_LAY_FLUXO) return fluxoRect(i, x0, NULL);
  return (GfxRect){ cx, cy, celW[i], celH[i] };
}

// 1 se o menu abriu. Titulo fora do catalogo e sem como entrar nele (id TMDB
// que nenhum item conhece) devolve 0 — o KEYUP vira toque, como sempre foi.
static int menuNoFocado(void) {
  CatItem it;
  GfxRect r;
  const char *arte = "";
  int idx;
  if (!viewItem(foco, &it)) return 0;
  if (!strncmp(it.imdb, "tmdb:", 5)) idx = ctxlista_indice(it.imdb, it.tmdb);
  else {
    idx = it.imdb[0] ? cat_indice_por_imdb(it.imdb) : -1;
    if (idx < 0 && it.imdb[0]) idx = cat_acrescentar(&it);
  }
  r = celulaRect(foco, &it, &arte);
  return ctxlista_abrir(idx, r, arte);
}

static int celulaAceitaMenu(void) {
  CatItem it;
  if (collection && tabFocus) return 0;
  if (lay == VT_LAY_VITRINE) return 0;   // a Vitrine trata o OK sozinha (vitEvento)
  if (nItens() < 1 || desc_vertudo_erro()) return 0;
  return viewItem(foco, &it);
}
static int vistoItem(const CatItem *it) { return cat_visto(it) || desc_filme_visto(it); }
static void contarVistos(int n) {
  static VcEntrada ent[VT_MAX];
  static char ids[VT_MAX][64], tipos[VT_MAX][8];
  int ne = 0;
  sagaVistos = 0;
  for (int i = 0; i < n && i < VT_MAX; i++) {
    CatItem it;
    int ok = viewItem(i, &it);
    celVisto[i] = ok && vistoItem(&it);
    sagaVistos += celVisto[i];
    if (!ok) continue;
    snprintf(ids[ne], sizeof ids[ne], "%s", it.imdb);
    snprintf(tipos[ne], sizeof tipos[ne], "%s", it.tipo);
    ent[ne] = (VcEntrada){ ids[ne], it.tmdb, tipos[ne], celVisto[i] };
    ne++;
  }
  vcTotal = vistocont_contar(ent, ne, &vcVistos);
}
// A lista de pessoas (Directors) mostra o contador e as marcas da timeline.
static int listaDePessoa(void) { return collection && !strcasecmp(collection->group, "Directors"); }

// --- A TABELA DE CELULAS ------------------------------------------------------
static void cel(int i, float x, float y, float w, float h, int lin, int col) {
  celX[i] = x; celY[i] = y; celW[i] = w; celH[i] = h;
  celLin[i] = (short)lin; celCol[i] = (short)col;
  if (y + h > celFim) celFim = y + h;
}
// A ordem cronologica (Linha do tempo, Saga, Decadas). Insercao estavel: dois
// titulos do mesmo ano ficam na ordem que o catalogo deu.
static void ordenar(int n) {
  static int years[VT_MAX];
  int old = orderN > 0 && foco < orderN ? order[foco] : (orderN < 0 ? -1 : foco);
  for (int i = 0; i < n; i++) { CatItem it; order[i] = i;
    years[i] = itemDaLista(i, &it) ? yearOf(&it) : 9999; }
  for (int i = 1; i < n; i++) { int v = order[i], j = i;
    while (j > 0 && years[order[j - 1]] > years[v]) { order[j] = order[j - 1]; j--; }
    order[j] = v; }
  orderN = n;
  if (old >= 0) for (int i = 0; i < n; i++) if (order[i] == old) { foco = i; break; }
}
static int decadaDe(int i) {
  CatItem it;
  int y = viewItem(i, &it) ? yearOf(&it) : 9999;
  return y == 9999 ? 0 : y / 10 * 10;
}
// MONTA a tabela da exibicao atual. Barato (uma passada pelos itens), e so
// roda quando o numero de itens ou a exibicao mudam — nunca por quadro.
static void montar(int n) {
  int i;
  if (n > VT_MAX) n = VT_MAX;
  celFim = 0.0f; decN = 0;
  if (ordenaPorAno() && n != orderN) ordenar(n);
  switch (lay) {
    case VT_LAY_FLUXO:
      // Uma linha so: esquerda/direita andam no tempo, cima vai as abas.
      fluxoPos = -1.0f;
      for (i = 0; i < n; i++) cel(i, i * (VT_FLX_W + VT_GAP_X), 0, VT_FLX_W, VT_FLX_H, 0, i);
      break;
    case VT_LAY_LINHA: {
      GfxRect card = vtTimelineCard(0);
      tlProg = -1.0f;
      for (i = 0; i < n; i++) cel(i, card.x - vtInicio(), i * (VT_LINHA_H + VT_TL_GAP), card.w, card.h, i, 0);
      break; }
    case VT_LAY_RANKING: {
      // Quatro por linha: o numeral mora no vao a ESQUERDA do cartaz, como o
      // Top 10 da Dinamica, e o cartaz cobre a ponta direita dele.
      const float faixa = 100.0f, w = 218.0f / gfx_escala_txt(), h = w * 1.5f;
      int cols = 4;
      float passo = VT_CW / 4.0f + 4.0f;
      if (vtTelefone()) {
        float livre = vtFim() - vtInicio();
        cols = (int)((livre + VT_GAP_X) / (faixa + w + VT_GAP_X));
        if (cols < 1) cols = 1;
        if (cols > 4) cols = 4;
        passo = (livre + VT_GAP_X) / cols;
      }
      for (i = 0; i < n; i++)
        cel(i, (i % cols) * passo + faixa, (i / cols) * (h + VT_GAP_Y), w, h, i / cols, i % cols);
      break; }
    case VT_LAY_SAGA: {
      // Tres capitulos deitados por linha, com a trilha de progresso por cima.
      float w = 424.0f / gfx_escala_txt(), h = 238.0f / gfx_escala_txt();
      int cols = 3;
      if (vtTelefone()) {
        float livre = vtFim() - vtInicio();
        w = fminf(w, livre); h = w * 238.0f / 424.0f;
        cols = (int)((livre + VT_GAP_X) / (w + VT_GAP_X));
        if (cols < 1) cols = 1;
        if (cols > 3) cols = 3;
      }
      float passoY = VT_SAGA_TRILHA + h + 108.0f;
      for (i = 0; i < n; i++)
        cel(i, (i % cols) * (w + VT_GAP_X), VT_SAGA_TRILHA + (i / cols) * passoY, w, h, i / cols, i % cols);
      break; }
    case VT_LAY_DECADAS: {
      // Uma secao por decada: o cabecalho e depois seis cartazes por linha.
      int cols = 6;
      float w = (VT_CW - 5.0f * VT_GAP_X) / 6.0f;
      if (vtTelefone()) {
        cols = VT_COLS < 6 ? VT_COLS : 6;
        w = (vtFim() - vtInicio() - (cols - 1) * VT_GAP_X) / cols;
      }
      float h = w * 1.5f, passoY = h + 56.0f;
      float y = 0.0f;
      int lin = -1, col = 0, dec = -1;
      for (i = 0; i < n; i++) {
        int d = decadaDe(i);
        if (i == 0 || d != dec) {
          if (i) y += passoY;              // fecha a ultima linha da secao anterior
          dec = d; col = 0; lin++;
          if (decN < VT_DEC_MAX) { decAno[decN] = d; decQtd[decN] = 0; decY[decN] = y; decN++; }
          y += VT_DEC_H;
        } else if (col == cols) { col = 0; lin++; y += passoY; }
        if (decN) decQtd[decN - 1]++;
        cel(i, col * (w + VT_GAP_X), y, w, h, lin, col);
        col++;
      }
      break; }
    case VT_LAY_MURAL: {
      // Blocos de cinco: um grande (2x2) e quatro pequenos, o grande trocando
      // de lado a cada bloco. Linha de cima: o grande e os dois pequenos do
      // alto; linha de baixo: os outros dois.
      // Solve the five-tile block from available width: big = two rows + gap.
      float g = VT_GAP_X,
            sh = (vtFim() - vtInicio() - (2.0f + 16.0f / 9.0f) * g) / (64.0f / 9.0f),
            sw = sh * 16.0f / 9.0f,
            bh = 2.0f * sh + g, bw = bh * 16.0f / 9.0f, folga = 28.0f;
      for (i = 0; i < n; i++) {
        int b = i / 5, k = i % 5;
        float y = b * (bh + g + folga);
        if (!(b & 1)) {
          if (k == 0) cel(i, 0, y, bw, bh, 2 * b, 0);
          else { int q = k - 1; cel(i, bw + g + (q & 1) * (sw + g), y + (q >> 1) * (sh + g), sw, sh,
                                   2 * b + (q >> 1), 1 + (q & 1)); }
        } else {
          if (k == 2) cel(i, 2 * (sw + g), y, bw, bh, 2 * b, 2);
          else { int q = k < 2 ? k : k - 1;
                 cel(i, (q & 1) * (sw + g), y + (q >> 1) * (sh + g), sw, sh, 2 * b + (q >> 1), q & 1); }
        }
      }
      break; }
    default: {  // GRADE: preserva o recuo da rail fixa (#359).
      int cols = VT_COLS;
      for (i = 0; i < n; i++)
        cel(i, (i % cols) * (VT_CARD_W + VT_GAP_X), (i / cols) * (VT_CARD_H + VT_GRADE_GAP_Y),
            VT_CARD_W, VT_CARD_H, i / cols, i % cols);
      break; }
  }
  celN = n; celLay = lay;
  celTelefone = vtTelefone(); celAreaW = vtFim() - vtInicio(); celTelaH = NV_TELA_H;
  if (foco >= n) foco = n ? n - 1 : 0;
  if (lay == VT_LAY_SAGA || listaDePessoa()) { contarVistos(n); vistosEm = SDL_GetTicks(); }
}
static int precisaMontar(int n);
static int atualizarGrade(void) {
  unsigned revisao = desc_vertudo_revisao();
  int n = nItens(), mudou = gradeRevisaoValida && gradeRevisao != revisao;
  if (n > VT_MAX) n = VT_MAX;
  if (mudou) {
    orderN = -1; celLay = -1;
    ctxhold_cancelar(&hold);
  }
  if (precisaMontar(n)) { montar(n); focoLembrado = -1; }
  if (mudou && (focoChave.id[0] || focoChave.tmdb > 0)) {
    for (int i = 0; i < n; i++) {
      CatItem it;
      if (viewItem(i, &it) && mesmaChave(&focoChave, &it)) { foco = i; break; }
    }
  }
  gradeRevisao = revisao; gradeRevisaoValida = 1;
  lembrarFoco();
  return n;
}
// O vizinho na linha `lin` (acima ou abaixo) com o centro mais perto do
// centro do item em foco. As linhas sao contiguas em indice em toda exibicao.
static int precisaMontar(int n) {
  return celLay != lay || celN != n || celTelefone != vtTelefone() ||
    ((vtTelefone() || lay == VT_LAY_MURAL || lay == VT_LAY_FLUXO || lay == VT_LAY_LINHA) &&
     (celAreaW != vtFim() - vtInicio() || celTelaH != NV_TELA_H));
}
static int vizinhoNaLinha(int n, int lin) {
  float cx = celX[foco] + celW[foco] * 0.5f, melhor = 1e9f;
  int achou = -1;
  for (int i = 0; i < n; i++) {
    float d;
    if (celLin[i] != lin) { if (celLin[i] > lin) break; continue; }
    d = fabsf(celX[i] + celW[i] * 0.5f - cx);
    if (d < melhor) { melhor = d; achou = i; }
  }
  return achou;
}
// O botao "Exibição" mora na faixa das abas: cursor = nAbas.
static int nAbas(void) {
  return collection && collection->nSources > 1 && lay != VT_LAY_VITRINE ? collection->nSources : 0;
}

// --- ESTILO DAS ABAS (#205, Ajustes > Home > "Estilo das abas") -----------------
//
// Pedido do dono: "outras opcoes tb, mais elegantes, tanto um dropdown (Movies
// ou TV Shows e ai as opcoes), a barra centralizada no topo, ou paginas com
// efeito carrossel". Cada estilo e so desenho + o que o D-pad faz no cabecalho;
// o modelo e o de sempre (tabFocus, tabCursor, source), entao a grade, o painel
// e o resto da pagina nao sabem qual estilo esta ligado.
//   Chips   cursor = a aba; uma fileira de pilulas (o de sempre).
//   Menu    cursor = [Filmes] [Séries] [catalogo v]; OK no catalogo abre a lista.
//   Barra   cursor = a aba; pilula de vidro no centro do alto, o realce desliza.
//   Paginas cursor = a pagina; Esquerda/Direita trocam de pagina na hora.
// Em todos o cursor == cabN() e o botao "Exibição".
enum { VT_AB_CHIPS, VT_AB_MENU, VT_AB_BARRA, VT_AB_PAGINAS, VT_AB_TOPO };
static int estiloAbas(void) {
  int e = ajustes_abas_estilo();
  return nAbas() && e > VT_AB_CHIPS && e <= VT_AB_TOPO ? e : VT_AB_CHIPS;
}
static int abaSerie(int i) {
  const ColSource *s = &collection->sources[i];
  return s->prov[0] ? !strcmp(s->midia, "TV") : !strcmp(s->type, "series");
}
// So ha primeiro nivel (Filmes/Séries) quando a colecao tem os dois tipos.
static int doisTipos(void) {
  int f = 0, s = 0;
  for (int i = 0; collection && i < collection->nSources; i++) { if (abaSerie(i)) s = 1; else f = 1; }
  return f && s;
}
// Os catalogos de um tipo (serie 0/1), em ordem; devolve quantos.
static int abasDoTipo(int serie, int *idx) {
  int n = 0;
  for (int i = 0; collection && i < collection->nSources; i++) if (abaSerie(i) == serie) idx[n++] = i;
  return n;
}
// Quantas posicoes de cursor ha antes do botao "Exibição".
static int cabN(void) {
  if (estiloAbas() == VT_AB_MENU) return doisTipos() ? 3 : 1;
  return nAbas();
}
static int cursorInicial(void) {
  if (estiloAbas() == VT_AB_MENU) return cabN() - 1;   // o botao do catalogo
  return nAbas() ? source : 0;
}
static void abasReiniciar(void) {
  ddAberto = ddCursor = 0; ddAnim = pgSlide = 0.0f; barraW = -1.0f;
  capW = -1.0f; capA = 0.0f;
}
// A capsula do foco nas abas de sempre: persegue a aba com o cursor com a
// mola do resto do foco (NV_MOLA_FOCO), em vez de acender e apagar no lugar
// (o "tudo meio duro" da Netflix na TCL). Nos outros estilos ela fica fora.
static void abasCapsulaAtualizar(float dt) {
  int est = estiloAbas();
  float alvoA = collection && tabFocus && tabCursor < cabN() &&
                (est == VT_AB_CHIPS || est == VT_AB_TOPO) ? 1.0f : 0.0f;
  capA = anim_mola(capA, alvoA, dt, alvoA > capA ? NV_MOLA_FOCO : NV_MOLA_DESFOCO);
  if (capW < 0.0f) { capX = capAlvoX; capW = capAlvoW; }
  else { capX = anim_mola(capX, capAlvoX, dt, NV_MOLA_FOCO * 0.6f);
         capW = anim_mola(capW, capAlvoW, dt, NV_MOLA_FOCO * 0.6f); }
}
static void alternarEstiloAbas(void) {
  ajustes_definir_abas_estilo((ajustes_abas_estilo() + 1) % (VT_AB_TOPO + 1));
  abasReiniciar(); tabRol = 0;
  tabFocus = 1; tabCursor = cabN() + 1;
}
// COLLECTION NA TV SEM O BOTAO "EXIBICAO" (dono, 15/11): a exibicao mudou para a
// primeira aba do painel do CH+ (salvospainel.c, vertudo_opcao_*). O botao some
// do cabecalho e da navegacao; no telefone ele continua.
static int chipOculto(void) { return collection && !vtTelefone(); }
#define VT_DD_RH 48.0f   // linha do menu suspenso
// JANELA DO MENU SUSPENSO (#205): com 16 catalogos a lista parava alem do pe
// da tela e o foco sumia ao descer. A altura fica no espaco util e a janela
// segue o cursor; devolve em *vis quantas linhas cabem.
static int ddJanela(int n, int cursor, float py, int *vis) {
  int v = (int)((NV_TELA_H - py - 24.0f - 12.0f) / VT_DD_RH), base;
  if (v < 1) v = 1;
  if (v > n) v = n;
  *vis = v;
  base = v < n ? cursor - v + 1 : 0;
  if (base < 0) base = 0;
  if (base > n - v) base = n - v;
  return base;
}
// Paginas: trocar de pagina e abrir a fonte; o deslize e so da pagina nova.
static void irPagina(int novo) {
  pgSlide = anim_politica_reduzida ? 0.0f : (novo > source ? 1.0f : -1.0f);
  source = novo; openSource();
}
// Menu suspenso: OK num botao de tipo escolhe o primeiro catalogo dele (se o
// tipo ja e o da fonte aberta, mantem) e abre a lista; OK no catalogo abre.
static void menuOk(void) {
  int cat = cabN() - 1, idx[COL_SOURCE_MAX], n;
  if (tabCursor < cat) {
    n = abasDoTipo(tabCursor, idx);
    if (!n) return;
    if (abaSerie(source) != tabCursor) { source = idx[0]; openSource(); }
    tabCursor = cat;
  }
  n = abasDoTipo(abaSerie(source), idx);
  if (n < 2) { tabFocus = 0; if (desc_vertudo_erro()) desc_vertudo_mais(); return; }
  ddAberto = 1; ddCursor = 0;
  for (int i = 0; i < n; i++) if (idx[i] == source) ddCursor = i;
}
static void ddEvento(SDL_Keycode k) {
  int idx[COL_SOURCE_MAX], n = abasDoTipo(abaSerie(source), idx);
  if (k == SDLK_UP) { if (ddCursor > 0) ddCursor--; }
  else if (k == SDLK_DOWN) { if (ddCursor < n - 1) ddCursor++; }
  else if (k == SDLK_LEFT || k == SDLK_RIGHT) ddAberto = 0;
  else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    ddAberto = 0;
    if (ddCursor < n && (idx[ddCursor] != source || desc_vertudo_erro())) { source = idx[ddCursor]; openSource(); }
    tabFocus = 0;
  }
}

// --- VITRINE ------------------------------------------------------------------
//
// Pedido de um usuario (Reddit): "can the collection view be like a home screen
// with a hero poster? Tabs are not the best view". A Vitrine e a Home em
// miniatura: o destaque do titulo em foco no alto (a mesma arte, logo, meta e
// sinopse do destaque da Home) e, abaixo, UMA FILEIRA POR ABA da colecao, com
// os cartazes das fileiras da Home. Sem abas, as fileiras saem do proprio
// catalogo: por tipo (filmes e series misturados) ou por decada.
//
// DADOS. Ate tres fontes leem suas primeiras paginas em paralelo; cada uma
// publica na propria fileira assim que responde. A grade conserva seu leitor
// e sua paginacao separados. O que sobra e uma copia de ate
// VIT_LIN_MAX x VIT_ITENS CatItems (alocada so quando a Vitrine abre pela
// primeira vez), de modo que mudar de fileira nao refaz pedido nenhum.
//
// CUSTO NA TV: o mesmo da Home com o foco nas fileiras. UMA arte de tela cheia
// (a do destaque, na camada GFX_HERO_CHEIO) e os cartazes; nenhuma passada de
// blur. Durante os 250 ms da troca de destaque a arte anterior fica por baixo
// (duas camadas, so ali, como a Home faz).
#define VIT_LIN_MAX COL_SOURCE_MAX
#define VIT_LIN_SINTETICA_MAX 12
#define VIT_ITENS   42   // teto por fileira: Ajustes > Coleções > Quantidade de itens (12..42)
// JANELA DE MEMORIA (dono, 15/11: ate 42 itens por fileira). Um CatItem tem
// ~17,8 KB; 42 itens x 64 fileiras passariam de 48 MB. Cada fileira ganha seu
// buffer SO quando os dados chegam, so as fileiras perto do foco sao pedidas, e
// as distantes devolvem a memoria (voltam a pendente e recarregam ao chegar
// perto). Teto pratico: ~(ATRAS+FRENTE+1) fileiras residentes.
#define VIT_CARGA_FRENTE 5    // fileiras com conteudo pedidas abaixo do foco
#define VIT_CARGA_ATRAS  2    // e acima
#define VIT_SOLTA_FRENTE 9    // alem disto (abaixo) a fileira devolve a memoria
#define VIT_SOLTA_ATRAS  5    // (acima)
#define VIT_SOLTA_FRENTE_FRACA 6
#define VIT_SOLTA_ATRAS_FRACA  3
// Itens por fileira EM VIGOR, fixado quando o buffer nasce (vitIniciar).
static int vitCap = VIT_ITENS;
enum { VIT_PEND = 0, VIT_CARREGANDO, VIT_PRONTA, VIT_VAZIA };
typedef struct { char nome[96]; int n, brutoN, extraN, estado, semFonte, mais, pagina, grupo, porTipo; } VitLin;
static VitLin vitLin[VIT_LIN_MAX];
static CatItem *vitLinBuf[VIT_LIN_MAX];   // por fileira, alocado quando os dados chegam
static int vitPronto, vitFraca;
// Preserva a primeira fileira original. Se ela tem filmes vistos, guarda ate
// outra fileira de candidatos ja recebidos para preencher as vagas visiveis.
// A reserva extra so e alocada onde ha vagas; jamais dispara um pedido novo.
static CatItem *vitExtra[VIT_LIN_MAX];
static CatItem *vitLinAloc(int r) {
  if (r < 0 || r >= VIT_LIN_MAX) return NULL;
  if (!vitLinBuf[r]) vitLinBuf[r] = calloc((size_t)vitCap, sizeof(CatItem));
  return vitLinBuf[r];
}
static void vitLinSoltar(int r) {
  free(vitLinBuf[r]); vitLinBuf[r] = NULL;
  free(vitExtra[r]); vitExtra[r] = NULL;
}
static short vitVis[VIT_LIN_MAX][VIT_ITENS];
static unsigned vitRevisao;
static int vitOcultar = -1, vitProjetoSujo;
static VtChave vitFocoChave;
static int vitN, vitMulti, vitCarga, vitFR, vitFC, vitMexeu, vitUnicoN = -1;
static int vitColLemb[VIT_LIN_MAX];
// 2.0.39: A POSICAO HORIZONTAL DE CADA FILEIRA SOBREVIVE A REPROJECAO. Na TV toda
// reprojecao (fonte/pagina recebida, fileira descarregada ou recarregada pela
// janela de carga, revalidacao do cache) zerava vitSx de TODAS as fileiras e a
// mola as trazia de volta: ao subir/descer, as fileiras ja avancadas voltavam ao
// 1o cartaz e deslizavam ate o lugar (2.0.31 trouxe o gatilho, o foco fixo da
// 2.0.33 o deixou visivel). Agora cada fileira guarda a CHAVE ESTAVEL (IMDb+tipo
// ou TMDB, nao o titulo) do item lembrado; a reprojecao reencontra a coluna por
// ela e so corrige o deslocamento pela diferenca de alvo (sem salto), e a
// fileira descarregada volta direto para a coluna onde estava.
static VtChave vitLembChave[VIT_LIN_MAX];
static unsigned char vitLembTem[VIT_LIN_MAX];   // 0 nada, 1 item (chave), 2 "Ver tudo"
static unsigned char vitSxAjuste[VIT_LIN_MAX];  // 0 nada, 1 soma a diferenca de alvo, 2 assenta no alvo
static int vitSxColAnt[VIT_LIN_MAX], vitSxTotAnt[VIT_LIN_MAX];
static Uint32 vitNomeEm, vitReleEm;
static float vitScroll, vitScrollV, vitSx[VIT_LIN_MAX], vitSv[VIT_LIN_MAX];
// O destaque: o que esta na tela, o que saiu, e o que o foco pede.
// 2.0.38: PILHA DE ATE 3 CAMADAS, cada uma com o proprio progresso. Uma troca
// interrompida por outra (o passo seguinte chega antes de os ~350 ms acabarem)
// nao reaproveita mais o destaque "meio entrado" como se estivesse inteiro: ele
// continua com a opacidade que tinha e a nova camada entra por cima. Sem salto.
#define VIT_HERO_DUR (NV_MOTION_MS / 1000.0f)   // 2.0.40: o tempo-base (350 ms)
static int vitHeroAtual = -1, vitHeroAnt = -1, vitHeroAnt2 = -1, vitHeroDes = -1;
static Uint32 vitHeroDesde;
static float vitHeroEntra = 1.0f, vitHeroEntraAnt = 1.0f, vitHeroEntraAnt2 = 1.0f;
static void vitHeroZerar(void) {
  vitHeroAtual = vitHeroAnt = vitHeroAnt2 = -1;
  vitHeroEntra = vitHeroEntraAnt = vitHeroEntraAnt2 = 1.0f;
}
// EXPANSAO DO CARTAZ VERTICAL EM FOCO (Ajustes > Coleções, dono 15/11): o mesmo
// gesto da Home. Relogio zera a cada movimento; abre depois do atraso parado.
// 2.0.37: o cartaz que ESTAVA aberto fecha animado. World (20/11): o relogio
// e o da Home (expansao.h) — mola de partida macia nas duas direcoes, ate tres
// cartazes fechando ao mesmo tempo (seta segurada) e a volta a um que fecha
// continua de onde ele esta.
static ExpEstado vitExp;
static int vitExpIniciado;
#define VIT_EXP_ASPECTO (16.0f / 9.0f)

// Largura extra de um cartaz aberto (ou fechando) para o cartaz (r, c): *propria
// e o que ele mesmo ganha; o retorno e o quanto os da direita deslocam.
static float vitExpExtra(int r, int c, float w, float h, float *propria) {
  float un = h * VIT_EXP_ASPECTO - w;
  *propria = un * exp_propria(&vitExp, r, c);
  return exp_empurra(&vitExp, r, c, NULL, NULL, un);
}

// Cada fileira tem seu pedido; o catalogo unico de Ver tudo continua separado.
// A tela e o fio possuem uma referencia cada. Fechar/trocar nao espera rede,
// e os pedidos abandonados continuam contando no teto global de tres pedidos.
#define VIT_PEDIDOS_MAX 3
typedef struct {
  int refs, pronto, atual, limite;
  unsigned geracao;
  uint64_t chave;   // colcache: o fio grava o resultado bom para a proxima vez
  NvPerfilSessao sessao;
  ColSource fonte;
  char base[NV_ADDON_URL_MAX];
  DescColecaoResultado resultado;
  CatItem itens[];
} VitPedido;
static VitPedido *vitPedidos[VIT_LIN_MAX];
static ColSource vitFontes[VIT_LIN_MAX];
static char vitBases[VIT_LIN_MAX][NV_ADDON_URL_MAX], vitColecaoChave[160];
static unsigned vitGeracao, vitColecoesRev;
static Uint32 vitRetentarEm[VIT_LIN_MAX];
static unsigned char vitDesligada[VIT_LIN_MAX];
// A fileira veio do colcache (memoria ou disco): continua na tela enquanto a
// revalidacao em segundo plano nao traz algo DIFERENTE, e uma falha de rede
// nao a apaga.
static unsigned char vitDoCache[VIT_LIN_MAX], vitRevalidar[VIT_LIN_MAX];
static NvPerfilSessao vitSessao;
static pthread_mutex_t vitPedidosTrava = PTHREAD_MUTEX_INITIALIZER;
static int vitEmVoo;
// A continuacao explicita conserva o leitor de paginas existente. Sua vaga
// so volta quando aquele fio termina, mesmo se a tela ja foi fechada.
static int vitManualFonte = -1, vitManualVivo;
static unsigned char vitManualPendente[VIT_LIN_MAX];
static unsigned vitManualGeracao;
static NvPerfilSessao vitManualSessao;
static ColSource vitManualCopia;
static char vitManualBase[NV_ADDON_URL_MAX];
static Uint32 vitManualDesde;

static void vitManualLiberarSlot(void) {
  if (vitManualVivo && !desc_vertudo_carregando()) {
    pthread_mutex_lock(&vitPedidosTrava); vitEmVoo--; pthread_mutex_unlock(&vitPedidosTrava);
    vitManualVivo = 0;
  }
}

static void vitPedidoSoltar(VitPedido *p) {
  if (p && __atomic_sub_fetch(&p->refs, 1, __ATOMIC_ACQ_REL) == 0) free(p);
}
static void vitCancelar(void) {
  vitGeracao++;
  vitManualFonte = -1;
  memset(vitManualPendente, 0, sizeof vitManualPendente);
  vitManualLiberarSlot();
  for (int r = 0; r < VIT_LIN_MAX; r++) {
    VitPedido *p = vitPedidos[r]; vitPedidos[r] = NULL;
    vitPedidoSoltar(p);
  }
}
static void *vitPedidoLer(void *u) {
  VitPedido *p = u;
  p->atual = desc_colecao_inicial(&p->fonte, p->base, p->sessao,
                                 p->itens, p->limite, &p->resultado);
  // Resultado bom vai para o colcache AQUI, no fio do pedido: a compressao e o
  // arquivo nunca custam um quadro. Os itens extras moram depois de `limite`.
  if (p->atual && !p->resultado.erro && p->chave) {
    int n = p->resultado.n, ex = p->resultado.extraN;
    if (n < 0) n = 0;
    if (n > p->limite) n = p->limite;
    if (ex < 0) ex = 0;
    if (ex > p->limite) ex = p->limite;
    ColCacheMeta m = { n, ex, p->resultado.pagina, p->resultado.mais };
    if (!ex || n == p->limite) colcache_gravar(p->chave, p->itens, &m);
    else {
      CatItem *junto = malloc(sizeof(CatItem) * (size_t)(n + ex));
      if (junto) {
        memcpy(junto, p->itens, sizeof(CatItem) * (size_t)n);
        memcpy(junto + n, p->itens + p->limite, sizeof(CatItem) * (size_t)ex);
        colcache_gravar(p->chave, junto, &m);
        free(junto);
      }
    }
  }
  __atomic_store_n(&p->pronto, 1, __ATOMIC_RELEASE);
  pthread_mutex_lock(&vitPedidosTrava);
  vitEmVoo--;
  pthread_mutex_unlock(&vitPedidosTrava);
  vitPedidoSoltar(p);
  return NULL;
}
static int vitPedidoLargar(int r) {
  pthread_t fio;
  VitPedido *p;
  pthread_mutex_lock(&vitPedidosTrava);
  if (vitEmVoo >= VIT_PEDIDOS_MAX) { pthread_mutex_unlock(&vitPedidosTrava); return 0; }
  vitEmVoo++;
  pthread_mutex_unlock(&vitPedidosTrava);
  p = calloc(1, sizeof *p + sizeof(CatItem) * (size_t)(2 * vitCap));
  if (p) {
    p->refs = 2; p->limite = vitCap; p->geracao = vitGeracao;
    p->sessao = vitSessao; p->fonte = vitFontes[r];
    snprintf(p->base, sizeof p->base, "%s", vitBases[r]);
    p->chave = colcache_chave(&vitFontes[r], vitBases[r], vitCap);
    if (pthread_create(&fio, NULL, vitPedidoLer, p) == 0) {
      vitPedidos[r] = p;
      if (!(vitDoCache[r] && vitLin[r].estado == VIT_PRONTA)) vitLin[r].estado = VIT_CARREGANDO;
      pthread_detach(fio);
      printf("[vertudo] vitrine: fonte %d iniciada\n", r + 1);
      return 1;
    }
    free(p);
  }
  pthread_mutex_lock(&vitPedidosTrava); vitEmVoo--; pthread_mutex_unlock(&vitPedidosTrava);
  return -1;
}
static int vitManualLargar(int r, Uint32 agora) {
  vitManualLiberarSlot();
  if (vitManualFonte >= 0 || vitManualVivo || desc_vertudo_carregando()) return 0;
  pthread_mutex_lock(&vitPedidosTrava);
  if (vitEmVoo >= VIT_PEDIDOS_MAX) { pthread_mutex_unlock(&vitPedidosTrava); return 0; }
  vitEmVoo++;
  pthread_mutex_unlock(&vitPedidosTrava);
  vitManualVivo = 1; vitManualFonte = r; vitManualPendente[r] = 0;
  vitManualGeracao = vitGeracao; vitManualSessao = vitSessao;
  vitManualCopia = vitFontes[r];
  snprintf(vitManualBase, sizeof vitManualBase, "%s", vitBases[r]);
  vitManualDesde = agora ? agora : 1u;
  source = r; carregarFonte(); vitLin[r].estado = VIT_CARREGANDO;
  return 1;
}
#ifdef NV_TOUCH_UI
// A target belongs to the rendered title and projection, never just a row index.
static VtChave vitPonteiroChave[VIT_LIN_MAX][VIT_ITENS];
static unsigned char vitPonteiroVisivel[VIT_LIN_MAX][VIT_ITENS];
static unsigned char vitPonteiroTudoVisivel[VIT_LIN_MAX];
static int vitPonteiroGeracao, vitPonteiroPronto;
static uint64_t vitPonteiroSessao;
static ToqueRolagem vitToqueY, vitToqueX[VIT_LIN_MAX];
static int vitToqueLinha = -1, vitToqueGeracao;
static void vitPonteiroInvalidar(int reiniciarRolagem) {
  vitPonteiroGeracao = vitPonteiroGeracao == INT_MAX ? 1 : vitPonteiroGeracao + 1;
  vitPonteiroPronto = 0;
  if (reiniciarRolagem) {
    vitToqueLinha = -1; vitToqueGeracao = 0;
    memset(&vitToqueY, 0, sizeof vitToqueY);
    memset(vitToqueX, 0, sizeof vitToqueX);
  } else if (vitToqueGeracao) vitToqueGeracao = vitPonteiroGeracao;
  memset(vitPonteiroVisivel, 0, sizeof vitPonteiroVisivel);
  memset(vitPonteiroTudoVisivel, 0, sizeof vitPonteiroTudoVisivel);
}
#endif


static CatItem *vitItem(int r, int c) {
  if (!vitPronto || r < 0 || r >= vitN || c < 0 || c >= vitLin[r].n) return NULL;
  int i = vitVis[r][c];
  if (i < vitCap) return vitLinBuf[r] ? &vitLinBuf[r][i] : NULL;
  return vitExtra[r] ? &vitExtra[r][i - vitCap] : NULL;
}
static CatItem *vitChave(int k) { return k < 0 ? NULL : vitItem(k / vitCap, k % vitCap); }
static int vitTemVerTudo(int r) { return r >= 0 && r < vitN && vitLin[r].n > 0 && !vitLin[r].semFonte; }
static int vitColunas(int r) { return vitLin[r].n + vitTemVerTudo(r); }
static int chaveValida(const VtChave *k) { return k->id[0] || k->tmdb > 0; }
// Guarda a chave do item na coluna `col` da fileira `r` (antes de ela mudar).
static void vitLembrarLinha(int r, int col) {
  if (r < 0 || r >= vitN || !vitLin[r].n) return;   // vazia: fica a chave que ja havia
  if (vitTemVerTudo(r) && col == vitLin[r].n) { memset(&vitLembChave[r], 0, sizeof vitLembChave[r]); vitLembTem[r] = 2; return; }
  guardarChave(&vitLembChave[r], vitItem(r, col));
  vitLembTem[r] = chaveValida(&vitLembChave[r]) ? 1 : 0;
}
static int vitPaginaFonte(void) {
  return lay == VT_LAY_VITRINE && vitMulti ? vitLin[source].pagina :
         (collection->sources[source].prov[0] ? 1 : 0);
}
static int vitMaisLinha(void) {
  for (int r = 0; r < vitN; r++)
    if (vitLin[r].mais && !vitLin[r].n) return r;
  return vitN;
}
static void vitFocarLinha(int linha) { vitFR = linha; vitFC = 0; }
static void vitFocarMais(void) { vitFocarLinha(vitMaisLinha()); }
static int vitTemMaisVistos(void) {
  return vitMaisLinha() < vitN;
}
static void vitPedirMais(void) {
  if (vitFR >= vitN || !vitLin[vitFR].mais) return;
  vitLin[vitFR].mais = 0;
  if (vitMulti) {
    vitManualPendente[vitFR] = 1;
    vitLin[vitFR].estado = VIT_PEND;
    return;
  }
  if ((!vitMulti || source == vitFR) && !desc_vertudo_carregando()) {
    vitCarga = vitFR; vitLin[vitFR].estado = VIT_CARREGANDO;
    desc_vertudo_mais();
  } else vitLin[vitFR].estado = VIT_PEND;
}
static int vitTemItens(void) {
  for (int r = 0; r < vitN; r++) if (vitLin[r].n > 0) return 1;
  return 0;
}
// A fileira com itens mais proxima de `r` na direcao `d` (+1 / -1); -1 se nao ha.
static int vitLinhaVizinha(int r, int d) {
  for (r += d; r >= 0 && r < vitN; r += d) if (vitLin[r].n > 0 || vitLin[r].mais) return r;
  return -1;
}
static void vitFocoInicial(void) {
  if (vitFR < vitN && vitLin[vitFR].mais && !vitLin[vitFR].n) return;
  if (vitLin[vitFR < vitN ? vitFR : 0].n > 0 && vitFR < vitN) return;
  for (int r = 0; r < vitN; r++) if (vitLin[r].n > 0) { vitFR = r; vitFC = 0; return; }
}
static void vitLembrarFoco(void) { guardarChave(&vitFocoChave, vitItem(vitFR, vitFC)); }
static void vitProjetar(void) {
  int ocultar = ajustes_ocultar_filmes_vistos();
  unsigned revisao = cat_revisao_itens();
  if (!vitProjetoSujo && ocultar == vitOcultar && (!ocultar || revisao == vitRevisao)) return;
  int linha = vitFR, coluna = vitFC;
  int noTudo = vitTemVerTudo(linha) && coluna == vitLin[linha].n;
  // 2.0.36: o destaque sobrevive a reprojecao pela CHAVE do titulo. Toda fonte
  // recebida (e toda pagina) reprojeta a Vitrine, e zerar o destaque aqui o
  // fazia sumir e reentrar do zero (entra=0, sem destaque anterior) — o log da
  // TV mostrou o bloco vermelho logo apos "fonte N recebida". Os indices sao da
  // projecao antiga; a chave acha o MESMO titulo na nova (se sumiu, zera como antes).
  VtChave heroChave, antChave;
  CatItem *heroAntes = vitChave(vitHeroAtual), *antAntes = vitChave(vitHeroAnt);
  VtChave ant2Chave; CatItem *ant2Antes = vitChave(vitHeroAnt2);
  guardarChave(&heroChave, heroAntes); guardarChave(&antChave, antAntes); guardarChave(&ant2Chave, ant2Antes);
  // A coluna da fileira em foco vive em vitFC (vitColLemb dela so e gravado ao sair).
  if (vitFR < vitN && vitLin[vitFR].n) vitColLemb[vitFR] = vitFC;
  for (int r = 0; r < vitN; r++) {
    VitLin *l = &vitLin[r];
    int antesN = l->n, colAnt = vitColLemb[r], totAnt = vitColunas(r);
    short antesVis[VIT_ITENS];
    memcpy(antesVis, vitVis[r], sizeof antesVis);
    // A chave e a do ULTIMO estado consistente (gravada no fim da reprojecao
    // anterior e a cada passo do foco): aqui o buffer da fileira ja pode ter
    // sido reescrito pela fonte/pagina nova.
    int lembradoTudo = vitLembTem[r] == 2, porChave = vitLembTem[r] == 1, achou = 0;
    int lembrado = !porChave && colAnt < antesN ? vitVis[r][colAnt] : -1;   // sem ID: indice bruto, como antes
    l->n = 0;
    for (int i = 0; i < l->brutoN + (ocultar ? l->extraN : 0) && l->n < vitCap; i++) {
      int bruto = i < l->brutoN ? i : vitCap + i - l->brutoN;
      CatItem *it = bruto < vitCap ? (vitLinBuf[r] ? &vitLinBuf[r][bruto] : NULL)
                                   : (vitExtra[r] ? &vitExtra[r][bruto - vitCap] : NULL);
      if (!it) continue;
      if (ocultar && desc_filme_visto(it)) continue;
      vitVis[r][l->n] = (short)bruto;
      if (!achou && (porChave ? mesmaChave(&vitLembChave[r], it) : bruto == lembrado)) { vitColLemb[r] = l->n; achou = 1; }
      l->n++;
    }
    if (lembradoTudo && vitTemVerTudo(r)) vitColLemb[r] = l->n;
    else if (!achou && !antesN) vitColLemb[r] = 0;   // fileira nova (ou o item lembrado saiu): do inicio
    else if (vitColLemb[r] >= vitColunas(r)) vitColLemb[r] = l->n ? l->n - 1 : 0;
    if (vtTelefone()) {
      // Receiving another row/page must not cancel an unchanged mobile swipe.
      int manter = l->n >= antesN;
      for (int i = 0; manter && i < antesN; i++)
        if (antesVis[i] != vitVis[r][i]) manter = 0;
      if (!manter) vitSx[r] = vitSv[r] = 0.0f;
      vitSxAjuste[r] = 0;
    } else if (!l->n) {
      vitSxAjuste[r] = 0;   // vazia: nada na tela; a chave fica para quando voltar
    } else if (!antesN) {
      // (Re)carregada: ja na coluna lembrada, sem deslizar. A fileira refeita
      // pelo reagrupamento (mesmo grupo) ja veio com o ajuste por diferenca.
      if (vitSxAjuste[r] != 1) { vitSxAjuste[r] = 2; vitSv[r] = 0.0f; }
    } else if (!vitSxAjuste[r]) {
      // Mesma fileira: o deslocamento atual fica; so soma a diferenca entre o
      // alvo novo e o antigo (item lembrado mudou de coluna, fileira cresceu).
      vitSxAjuste[r] = 1; vitSxColAnt[r] = colAnt; vitSxTotAnt[r] = totAnt;
    }
    vitLembrarLinha(r, vitColLemb[r]);   // vazia: mantem a chave anterior
  }
  vitFR = linha < vitN ? linha : (vitN ? vitN - 1 : 0);
  vitFC = coluna;
  // Procura primeiro na mesma fileira: catalogos distintos podem repetir a obra.
  int achou = 0;
  for (int passo = 0; !noTudo && passo < vitN && !achou; passo++) {
    int r = (vitFR + passo) % vitN;
    for (int c = 0; c < vitLin[r].n; c++)
      if (mesmaChave(&vitFocoChave, vitItem(r, c))) { vitFR = r; vitFC = c; achou = 1; break; }
  }
  if (vitN && !vitLin[vitFR].n && !vitLin[vitFR].mais) {
    int r = vitLinhaVizinha(vitFR, 1);
    if (r < 0) r = vitLinhaVizinha(vitFR, -1);
    if (r >= 0) { vitFR = r; vitFC = vitColLemb[r]; }
  }
  if (!vitN) vitFR = vitFC = 0;
  else if (noTudo && vitTemVerTudo(vitFR)) vitFC = vitLin[vitFR].n;
  else if (vitFC >= vitColunas(vitFR)) vitFC = vitLin[vitFR].n ? vitLin[vitFR].n - 1 : 0;
  // Os indices dos destaques eram da projecao anterior; nao deixam um filme
  // ocultado aparecer no hero nem apontam para o vizinho que tomou seu lugar.
  // Religa pelo titulo (a copia mais proxima do foco); sem achar, zera como antes.
  { int novoHero = -1, novoAnt = -1;
    int novoAnt2 = -1;
    for (int passo = 0; vitN && passo < vitN && ((heroAntes && novoHero < 0) || (antAntes && novoAnt < 0) || (ant2Antes && novoAnt2 < 0)); passo++) {
      int r = (vitFR + passo) % vitN;
      for (int c = 0; c < vitLin[r].n; c++) {
        CatItem *cand = vitItem(r, c);
        if (!cand) continue;
        if (heroAntes && novoHero < 0 && mesmaChave(&heroChave, cand)) novoHero = r * vitCap + c;
        if (antAntes && novoAnt < 0 && mesmaChave(&antChave, cand)) novoAnt = r * vitCap + c;
        if (ant2Antes && novoAnt2 < 0 && mesmaChave(&ant2Chave, cand)) novoAnt2 = r * vitCap + c;
      }
    }
    vitHeroDes = -1;
    if (novoHero < 0) vitHeroZerar();
    else {
      vitHeroAtual = novoHero; vitHeroAnt = novoAnt; vitHeroAnt2 = novoAnt < 0 ? -1 : novoAnt2;
      if (vitHeroAnt < 0) vitHeroEntraAnt = 1.0f;
      if (vitHeroAnt2 < 0) vitHeroEntraAnt2 = 1.0f;
    }
  }
  vitOcultar = ocultar; vitRevisao = revisao; vitProjetoSujo = 0;
  vitLembrarFoco();
  vitLembrarLinha(vitFR, vitFC);
#ifdef NV_TOUCH_UI
  vitPonteiroInvalidar(0);
#endif
}

// O texto de cada aba: o mesmo da barra de abas (ver a nota em themeHeader).
// Nome de cada fonte: titulo da conta/manisfesto e, para addon catalogos,
// o genero que o Original acrescenta ao nome da subcategoria. TMDB/Trakt ja
// trazem titulo proprio e nao recebem sufixo de genero.
static void nomesFontesColecao(char nomes[][180], int n) {
  for (int i = 0; i < n; i++) {
    const ColSource *s = &collection->sources[i];
    char base[128] = "";
    const char *tipo = i18n((s->prov[0] ? !strcmp(s->midia, "TV") : !strcmp(s->type, "series")) ? "Séries" : "Filmes");
    col_nome_fonte(s, desc_nome_catalogo(baseDaFonte(s), s->type, s->catId), base, sizeof base);
    if (!base[0]) snprintf(base, sizeof base, "%s", tipo);
    if (!s->prov[0] && s->genre[0] && strcasecmp(s->genre, "None"))
      snprintf(nomes[i], 180, "%.127s · %.47s", base, s->genre);
    else
      snprintf(nomes[i], 180, "%s", base);
  }
}
static void rotulosAbas(char (*rot)[180]) {
  static char nomeAba[COL_SOURCE_MAX][180];
  int n = collection->nSources;
  if (n > COL_SOURCE_MAX) n = COL_SOURCE_MAX;
  nomesFontesColecao(nomeAba, n);
  for (int i = 0; i < n; i++) {
    const ColSource *s = &collection->sources[i];
    const char *tipo = i18n((s->prov[0] ? !strcmp(s->midia, "TV") : !strcmp(s->type, "series")) ? "Séries" : "Filmes");
    int repete = 0;
    for (int j = 0; j < n; j++)
      if (j != i && !strcmp(nomeAba[i], nomeAba[j])) repete = 1;
    if (repete) snprintf(rot[i], 180, "%.168s · %.7s", nomeAba[i], tipo);
    else snprintf(rot[i], 180, "%s", nomeAba[i]);
  }
}
static void vitNomes(void) {
  static char rot[COL_SOURCE_MAX][180];
  if (!vitMulti || !collection) return;
  rotulosAbas(rot);
  for (int r = 0; r < vitN; r++) snprintf(vitLin[r].nome, sizeof vitLin[r].nome, "%s", rot[r]);
}

static void vitIniciar(void) {
  vitCancelar();
  vitSessao = nv_perfil_sessao_ler();
  vitColecoesRev = col_revisao();
  vitColecaoChave[0] = 0;
  if (collection) col_chave_pasta(collection, vitColecaoChave, sizeof vitColecaoChave);
#ifdef NV_TOUCH_UI
  vitPonteiroInvalidar(1);
  vitPonteiroSessao = perfis_sessao_geracao();
#endif
  // FOLLOW_LAYOUT com uma fonte continua sendo uma fileira, nao um catalogo
  // agrupado por decada. Uma pasta com varias fontes sempre e multi-fileira.
  vitMulti = collection && (collection->nSources > 1 ||
      collection->viewMode == COL_VIEW_FOLLOW_LAYOUT ||
      collection->viewMode == COL_VIEW_ROWS);
  vitN = vitMulti && collection->nSources > 0
      ? (collection->nSources < VIT_LIN_MAX ? collection->nSources : VIT_LIN_MAX) : 0;

  // ITENS POR FILEIRA: o ajuste do perfil (12..42). A memoria nao e reservada
  // aqui: cada fileira aloca o proprio buffer quando os dados chegam e as
  // distantes do foco o devolvem (ver VIT_CARGA_*/VIT_SOLTA_* e vitCarregar).
  long memMb = 0; int mb = 0, fixo = 0, slots = 0;
  tex_orcamento_info(&mb, &memMb, &fixo, &slots);
  vitFraca = (memMb > 0 && memMb <= 1536) || ptv_gpu_fraca_atual();
  for (int r = 0; r < VIT_LIN_MAX; r++) { free(vitLinBuf[r]); vitLinBuf[r] = NULL; }
  vitCap = ajustes_col_itens_fileira();
  if (vitCap > VIT_ITENS) vitCap = VIT_ITENS;
  if (vitCap < 2) vitCap = 2;
  vitPronto = 1;
  if (!vitMulti)   // fileiras sinteticas (uma fonte agrupada): poucas, todas de uma vez
    for (int r = 0; r < VIT_LIN_SINTETICA_MAX; r++)
      if (!vitLinAloc(r)) { vitPronto = 0; break; }

  memset(vitLin, 0, sizeof vitLin);
  memset(vitRetentarEm, 0, sizeof vitRetentarEm);
  memset(vitDesligada, 0, sizeof vitDesligada);
  memset(vitDoCache, 0, sizeof vitDoCache); memset(vitRevalidar, 0, sizeof vitRevalidar);
  for (int r = 0; r < VIT_LIN_MAX; r++) { free(vitExtra[r]); vitExtra[r] = NULL; }
  memset(vitVis, 0, sizeof vitVis);
  memset(&vitFocoChave, 0, sizeof vitFocoChave);
  vitOcultar = -1; vitProjetoSujo = 1;
  memset(vitColLemb, 0, sizeof vitColLemb);
  memset(vitLembChave, 0, sizeof vitLembChave); memset(vitLembTem, 0, sizeof vitLembTem);
  memset(vitSxAjuste, 0, sizeof vitSxAjuste);
  memset(vitSx, 0, sizeof vitSx); memset(vitSv, 0, sizeof vitSv);
  vitCarga = 0; vitFR = vitFC = 0; vitMexeu = 0; vitUnicoN = -1;
  vitScroll = vitScrollV = 0.0f;
  vitHeroZerar(); vitHeroDes = -1;
  memset(vitLinAlfa, 0, sizeof vitLinAlfa); memset(vitLinEntra, 0, sizeof vitLinEntra);
  vitFocoR = vitFocoC = vitFocoAntR = vitFocoAntC = -1; vitFocoF = vitFocoAntF = 0.0f;
  exp_zerar(&vitExp); vitExpIniciado = 1;   // nada aberto de uma visita anterior
  vitNomeEm = vitReleEm = 0;
  if (!vitPronto || !collection) { lay = VT_LAY_GRADE; celLay = -1; carregarFonte(); return; }
  if (vitMulti) {
    for (int r = 0; r < vitN; r++) {
      vitFontes[r] = collection->sources[r];
      snprintf(vitBases[r], sizeof vitBases[r], "%s", baseDaFonte(&vitFontes[r]));
    }
    vitNomes();
    for (int r = 0; r < vitN; r++) vitLin[r].pagina = 0;
  } else {
    vitFontes[0] = collection->sources[source];
    snprintf(vitBases[0], sizeof vitBases[0], "%s", baseDaFonte(&vitFontes[0]));
    carregarFonte();
  }
}

// Guarda a primeira pagina do catalogo aberto na fileira `r`.
static void vitGuardarPagina(int r) {
  int n = semFonte ? 0 : desc_vertudo_n_bruto(), k = 0, vistos = 0, skip = 0;
  CatItem *buf = vitLinAloc(r);
  vitLin[r].extraN = 0;
  if (!buf) { vitLin[r].estado = VIT_VAZIA; vitProjetoSujo = 1; return; }
  k = vitLin[r].brutoN;
  if (!k) {
    for (; skip < n && k < vitCap; skip++)
      if (desc_vertudo_item_bruto(skip, &buf[k])) k++;
    vitLin[r].brutoN = k;
  }
  for (int i = 0; i < k; i++) vistos += desc_filme_visto(&buf[i]);
  if (vistos && n > skip && !vitExtra[r]) vitExtra[r] = calloc((size_t)vitCap, sizeof(CatItem));
  if (vistos && vitExtra[r]) {
    CatItem it;
    for (int i = skip; i < n && vitLin[r].extraN < vitCap; i++)
      if (desc_vertudo_item_bruto(i, &it) && !desc_filme_visto(&it)) {
        int j;
        for (j = 0; j < k; j++)
          if (it.imdb[0] && !strcmp(it.imdb, buf[j].imdb) &&
              !strcmp(it.tipo, buf[j].tipo)) break;
        if (j == k) vitExtra[r][vitLin[r].extraN++] = it;
      }
  }
  vitLin[r].estado = k ? VIT_PRONTA : VIT_VAZIA;
  vitLin[r].semFonte = semFonte;
  vitLin[r].mais = !semFonte && !desc_vertudo_fim() && !desc_vertudo_erro();
  vitLin[r].pagina = desc_vertudo_pagina();
  vitProjetoSujo = 1;
#ifdef NV_TOUCH_UI
  vitPonteiroInvalidar(0);
#endif
}

// Somente o principal publica: os baldes dos fios nunca sao a memoria desenhada.
static void vitGuardarInicial(int r, const VitPedido *p) {
  int n = p->resultado.n, extra = p->resultado.extraN;
  if (n < 0) n = 0;
  if (n > vitCap) n = vitCap;
  if (extra < 0) extra = 0;
  if (extra > vitCap) extra = vitCap;
  CatItem *buf = n ? vitLinAloc(r) : NULL;
  if (n && !buf) n = 0;   // sem memoria: a fileira fica vazia, sem derrubar o app
  if (n) memcpy(buf, p->itens, sizeof(CatItem) * (size_t)n);
  vitLin[r].brutoN = n; vitLin[r].extraN = 0;
  if (extra && !vitExtra[r]) vitExtra[r] = calloc((size_t)vitCap, sizeof(CatItem));
  if (extra && vitExtra[r]) {
    memcpy(vitExtra[r], p->itens + vitCap, sizeof(CatItem) * (size_t)extra);
    vitLin[r].extraN = extra;
  }
  vitLin[r].estado = n || vitLin[r].extraN ? VIT_PRONTA : VIT_VAZIA;
  vitLin[r].semFonte = 0;
  vitLin[r].pagina = p->resultado.pagina;
  vitLin[r].mais = !p->resultado.erro && p->resultado.mais;
  vitProjetoSujo = 1;
  printf("[vertudo] vitrine: fonte %d recebida: %d titulos, %d reservas%s\n",
         r + 1, n, vitLin[r].extraN, p->resultado.erro ? " (falha)" : "");
}

// A FILEIRA SAI DO COLCACHE, sem rede: mesma publicacao de vitGuardarInicial.
// Devolve a idade (s) ou -1 sem cache. Os extras ficam depois dos `n`.
static long vitDoColcache(int r) {
  ColCacheMeta m;
  long idade = -1;
  int max = vitCap * 2;
  CatItem *tmp;
  uint64_t chave = colcache_chave(&vitFontes[r], vitBases[r], vitCap);
  if (!(tmp = malloc(sizeof(CatItem) * (size_t)max))) return -1;
  if (colcache_ler(chave, tmp, max, &m, &idade)) {
    int n = m.n > vitCap ? vitCap : m.n, ex = m.extraN > vitCap ? vitCap : m.extraN;
    CatItem *buf = n ? vitLinAloc(r) : NULL;
    if (n && !buf) { free(tmp); return -1; }
    if (n) memcpy(buf, tmp, sizeof(CatItem) * (size_t)n);
    vitLin[r].brutoN = n; vitLin[r].extraN = 0;
    if (ex && !vitExtra[r]) vitExtra[r] = calloc((size_t)vitCap, sizeof(CatItem));
    if (ex && vitExtra[r]) { memcpy(vitExtra[r], tmp + m.n, sizeof(CatItem) * (size_t)ex); vitLin[r].extraN = ex; }
    vitLin[r].estado = n || vitLin[r].extraN ? VIT_PRONTA : VIT_VAZIA;
    vitLin[r].semFonte = 0; vitLin[r].pagina = m.pagina; vitLin[r].mais = m.mais;
    vitDoCache[r] = 1; vitProjetoSujo = 1;
  } else idade = -1;
  free(tmp);
  return idade;
}
// A revalidacao trouxe o MESMO conteudo (mesmos ids, na mesma ordem)?
static int vitMesmoConteudo(int r, const VitPedido *p) {
  int n = p->resultado.n > vitCap ? vitCap : p->resultado.n;
  if (n != vitLin[r].brutoN || !vitLinBuf[r]) return 0;
  for (int i = 0; i < n; i++)
    if (strcmp(vitLinBuf[r][i].imdb, p->itens[i].imdb) || strcmp(vitLinBuf[r][i].tipo, p->itens[i].tipo)) return 0;
  return p->resultado.mais == vitLin[r].mais;
}

// SEM ABAS: as fileiras saem dos titulos lidos. Tipos misturados = uma fileira
// por tipo; senao uma por decada (a mais nova primeiro, "sem data" por ultimo).
static void vitAgrupar(int n) {
  static short chave[VT_MAX];
  static CatItem tmp;
  int chaves[VIT_LIN_SINTETICA_MAX + 1], nc = 0, filmes = 0, series = 0;
  int focoTudo = vitTemVerTudo(vitFR) && vitFC == vitLin[vitFR].n;
  char focoNome[96] = "";
  if (focoTudo) snprintf(focoNome, sizeof focoNome, "%s", vitLin[vitFR].nome);
  // 2.0.39 (TV): os grupos sao refeitos do zero e um grupo novo muda os indices.
  // A memoria de cada fileira (chave do item lembrado e deslocamento) segue o
  // NOME do grupo, nao a posicao no vetor.
  static struct { char nome[96]; VtChave chave; unsigned char tem; float x, v; int col, tot; } memo[VIT_LIN_MAX];
  int memoN = vtTelefone() ? 0 : vitN;
  for (int r = 0; r < memoN; r++) {
    snprintf(memo[r].nome, sizeof memo[r].nome, "%s", vitLin[r].nome);
    memo[r].chave = vitLembChave[r]; memo[r].tem = vitLembTem[r];
    memo[r].x = vitSx[r]; memo[r].v = vitSv[r];
    memo[r].col = r == vitFR ? vitFC : vitColLemb[r]; memo[r].tot = vitColunas(r);
  }
#ifdef NV_TOUCH_UI
  // Later pages can insert a new decade before existing rows. Touch positions
  // belong to those groups, not to the array slots they happened to occupy.
  static struct { char nome[96]; float x, v; int coluna, livre, n; VtChave itens[VIT_ITENS]; } antes[VIT_LIN_MAX];
  int antesN = vtTelefone() ? vitN : 0, ancora = -1, gesto = vitToqueLinha;
  VtChave focoAntes; guardarChave(&focoAntes, vitItem(vitFR, vitFC));
  for (int r = 0; r < antesN; r++) {
    snprintf(antes[r].nome, sizeof antes[r].nome, "%s", vitLin[r].nome);
    antes[r].x = vitSx[r]; antes[r].v = vitSv[r];
    antes[r].coluna = vitColLemb[r]; antes[r].livre = vitToqueX[r].livre;
    antes[r].n = vitLin[r].n;
    for (int c = 0; c < antes[r].n; c++) guardarChave(&antes[r].itens[c], vitItem(r, c));
  }
  if (antesN && vitToqueY.livre) {
    ancora = (int)(vitScroll / vitPasso());
    if (ancora < 0) ancora = 0;
    if (ancora >= antesN) ancora = antesN - 1;
  }
#endif
  if (n > VT_MAX) n = VT_MAX;
  for (int i = 0; i < n; i++)
    if (desc_vertudo_item_bruto(i, &tmp)) { if (!strcmp(tmp.tipo, "series")) series = 1; else filmes = 1; }
  for (int i = 0; i < n; i++) {
    int k;
    chave[i] = -1;
    if (!desc_vertudo_item_bruto(i, &tmp)) continue;
    if (filmes && series) k = !strcmp(tmp.tipo, "series") ? 1 : 0;
    else { int a = nv_ano_meta(tmp.meta); k = a ? a / 10 * 10 : 0; }
    chave[i] = (short)k;
    { int j; for (j = 0; j < nc && chaves[j] != k; j++) {}
      if (j == nc && nc < VIT_LIN_SINTETICA_MAX) chaves[nc++] = k; }
  }
  // Ordem das fileiras: tipos filmes, series; decadas da mais nova a mais velha.
  for (int i = 1; i < nc; i++) {
    int v = chaves[i], j = i;
    int mistos = filmes && series;
    for (; j > 0; j--) {
      int a = chaves[j - 1], antes = mistos ? a > v : (v == 0 ? 0 : (a == 0 ? 1 : a < v));
      if (!antes) break;
      chaves[j] = chaves[j - 1];
    }
    chaves[j] = v;
  }
  memset(vitLin, 0, sizeof vitLin);
  vitN = nc;
  for (int r = 0; r < nc; r++) {
    VitLin *l = &vitLin[r];
    l->grupo = chaves[r]; l->porTipo = filmes && series;
    if (filmes && series) snprintf(l->nome, sizeof l->nome, "%s", i18n(chaves[r] ? "Séries" : "Filmes"));
    else if (chaves[r]) snprintf(l->nome, sizeof l->nome, i18n("Anos %d"), chaves[r]);
    else snprintf(l->nome, sizeof l->nome, "%s", i18n("Sem data"));
    int vistos = 0;
    for (int i = 0; i < n; i++) {
      if (chave[i] != chaves[r] || !desc_vertudo_item_bruto(i, &tmp)) continue;
      if (l->brutoN < vitCap) {
        vistos += desc_filme_visto(&tmp);
        vitLinBuf[r][l->brutoN++] = tmp;
      } else if (vistos && l->extraN < vitCap && !desc_filme_visto(&tmp)) {
        if (!vitExtra[r]) vitExtra[r] = calloc((size_t)vitCap, sizeof(CatItem));
        if (vitExtra[r]) vitExtra[r][l->extraN++] = tmp;
      }
    }
    l->estado = VIT_PRONTA;
  }
  if (memoN) {
    for (int r = 0; r < vitN; r++) {
      memset(&vitLembChave[r], 0, sizeof vitLembChave[r]); vitLembTem[r] = 0;
      vitSx[r] = vitSv[r] = 0.0f; vitColLemb[r] = 0; vitSxAjuste[r] = 0;
      for (int i = 0; i < memoN; i++) if (!strcmp(vitLin[r].nome, memo[i].nome)) {
        vitLembChave[r] = memo[i].chave; vitLembTem[r] = memo[i].tem;
        vitSx[r] = memo[i].x; vitSv[r] = memo[i].v;
        if (memo[i].tot > 0) { vitSxAjuste[r] = 1; vitSxColAnt[r] = memo[i].col; vitSxTotAnt[r] = memo[i].tot; }
        break;
      }
    }
  }
  vitProjetoSujo = 1;
  vitProjetar();
  if (vitFR >= vitN) vitFR = vitN ? vitN - 1 : 0;
  if (vitN && vitFC >= vitColunas(vitFR)) vitFC = vitLin[vitFR].n ? vitLin[vitFR].n - 1 : 0;
#ifdef NV_TOUCH_UI
  if (antesN) {
    vitToqueLinha = -1;
    memset(vitToqueX, 0, sizeof vitToqueX);
    for (int r = 0; r < vitN; r++) {
      vitSx[r] = vitSv[r] = 0.0f; vitColLemb[r] = 0;
      for (int i = 0; i < antesN; i++) if (!strcmp(vitLin[r].nome, antes[i].nome)) {
        int manter = antes[i].n <= vitLin[r].n;
        for (int c = 0; manter && c < antes[i].n; c++)
          if (!mesmaChave(&antes[i].itens[c], vitItem(r, c))) manter = 0;
        vitColLemb[r] = antes[i].n > 0 && antes[i].coluna == antes[i].n ? vitLin[r].n :
                        antes[i].coluna < vitLin[r].n ? antes[i].coluna :
                        (vitLin[r].n ? vitLin[r].n - 1 : 0);
        if (manter) {
          vitSx[r] = antes[i].x; vitSv[r] = antes[i].v;
          vitToqueX[r].livre = antes[i].livre;
          if (i == gesto) vitToqueLinha = r;
        }
        if (i == ancora) { vitScroll += (r - i) * vitPasso(); vitScrollV = 0.0f; }
        break;
      }
    }
    if (gesto >= 0 && vitToqueLinha < 0) vitToqueGeracao = 0;
    for (int r = 0; r < vitN; r++)
      for (int c = 0; c < vitLin[r].n; c++)
        if (mesmaChave(&focoAntes, vitItem(r, c))) { vitFR = r; vitFC = c; r = vitN; break; }
    // The highlight's encoded row/column belongs to the previous grouping.
    vitHeroZerar(); vitHeroDes = -1;
  }
  vitPonteiroInvalidar(0);
#endif
  if (focoTudo) for (int r = 0; r < vitN; r++)
    if (!strcmp(vitLin[r].nome, focoNome)) {
      vitFR = r; vitFC = vitColLemb[r] = vitLin[r].n; break;
    }
}

// Um passo da carga, por quadro: as fontes paralelas, as paginas do catalogo
// unico e a releitura das fontes cujo addon ainda nao tinha endereco.
static void vitCarregar(Uint32 agora) {
  if (!vitPronto) return;
  if (!vitMulti) {
    int n = desc_vertudo_n_bruto();
    if (n != vitUnicoN) { vitUnicoN = n; if (n > 0) vitAgrupar(n); }
    for (int r = 0; r < vitN; r++)
      vitLin[r].mais = !semFonte && !desc_vertudo_carregando() &&
                       !desc_vertudo_fim() && !desc_vertudo_erro();
    // O catalogo traz 100 por pagina: pede mais ate haver o que agrupar.
    if (n > 0 && n < 300 && !desc_vertudo_carregando() && !desc_vertudo_fim() &&
        !desc_vertudo_erro()) desc_vertudo_mais();
    if (semFonte && baseDaFonte(&collection->sources[source])[0]) vitIniciar();
    return;
  }
  if (!nv_perfil_sessao_atual(vitSessao)) {
    vitCancelar(); aberta = 0; return;
  }
  // A conta pode substituir o bloco de fontes enquanto a pasta esta aberta.
  // Reencontrar pela chave antes de ler fontes; os pedidos guardam copias.
  unsigned revisao = col_revisao();
  int nomesNovos = revisao != vitColecoesRev;
  if (revisao != vitColecoesRev) {
    const ColFolder *atual = NULL;
    for (int i = 0; i < col_n(); i++) {
      const ColFolder *f = col_folder(i);
      char chave[160];
      if (!f) continue;
      col_chave_pasta(f, chave, sizeof chave);
      if (!strcmp(chave, vitColecaoChave)) { atual = f; break; }
    }
    if (!atual) { vitCancelar(); aberta = 0; return; }
    collection = atual;
    // A quantidade de fontes pode mudar durante o sync. Recalcula o buffer e
    // invalida os pedidos antigos antes de voltar a carregar as fileiras.
    vitIniciar();
    return;
  }
  int novas = collection->nSources < VIT_LIN_MAX ? collection->nSources : VIT_LIN_MAX;
  for (int r = novas; r < vitN; r++) {
    if (vitManualFonte == r) vitManualFonte = -1;
    vitManualPendente[r] = 0;
    vitPedidoSoltar(vitPedidos[r]); vitPedidos[r] = NULL;
    vitLinSoltar(r);
    memset(&vitLin[r], 0, sizeof vitLin[r]);
  }
  int antigas = vitN;
  vitN = novas;
  if (antigas != vitN) vitProjetoSujo = 1;
  int rever = (Sint32)(agora - vitReleEm) > 1500;
  if (rever) vitReleEm = agora;
  if (rever || nomesNovos || antigas != vitN) vitNomes();
  // JANELA DE CARGA: so as fileiras perto do foco sao pedidas. Conta fileiras
  // que podem ter conteudo (nao as vazias/desligadas), para uma sequencia de
  // fontes vazias nao deixar o resto da pasta sem carregar.
  int cargaIni = vitFR, cargaFim = vitFR;
  for (int k = 0, r = vitFR; r >= 0 && k <= VIT_CARGA_ATRAS; r--) { cargaIni = r; if (vitLin[r].estado != VIT_VAZIA) k++; }
  for (int k = 0, r = vitFR; r < vitN && k <= VIT_CARGA_FRENTE; r++) { cargaFim = r; if (vitLin[r].estado != VIT_VAZIA) k++; }
  int cacheLidas = 0;
  for (int r = 0; r < vitN; r++) {
    const ColSource *s = &collection->sources[r];
    char base[NV_ADDON_URL_MAX];
    snprintf(base, sizeof base, "%s", baseDaFonte(s));
    if (r >= antigas || memcmp(&vitFontes[r], s, sizeof *s) || strcmp(vitBases[r], base)) {
      if (vitManualFonte == r) vitManualFonte = -1;
      vitManualPendente[r] = 0;
      vitPedidoSoltar(vitPedidos[r]); vitPedidos[r] = NULL;
      vitLinSoltar(r);
      vitLin[r].n = vitLin[r].brutoN = vitLin[r].extraN = 0;
      vitLin[r].mais = 0; vitLin[r].pagina = 0;
      vitLin[r].estado = VIT_PEND; vitLin[r].semFonte = 0;
      vitRetentarEm[r] = 0;
      vitDesligada[r] = 2;   // a nova fonte ainda nao foi conferida
      vitFontes[r] = *s;
      snprintf(vitBases[r], sizeof vitBases[r], "%s", base);
      vitProjetoSujo = 1;
    }
    // TMDB e Trakt sao fontes reais da Collection, nao addons ausentes.
    // A disponibilidade do servico e verificada abaixo; se estiver configurado,
    // a mesma fila limitada carrega a fonte sem transforma-la em uma linha vazia.
    int desligada = !fonteDisponivel(s, base);
    if (vitDesligada[r] != desligada) {
      if (vitManualFonte == r) vitManualFonte = -1;
      vitManualPendente[r] = 0;
      vitDesligada[r] = (unsigned char)desligada;
      vitPedidoSoltar(vitPedidos[r]); vitPedidos[r] = NULL;
      vitLinSoltar(r);
      vitLin[r].n = vitLin[r].brutoN = vitLin[r].extraN = 0;
      vitLin[r].mais = 0; vitLin[r].pagina = 0;
      vitLin[r].estado = desligada ? VIT_VAZIA : VIT_PEND;
      vitLin[r].semFonte = desligada; vitRetentarEm[r] = 0;
      vitProjetoSujo = 1;
    }
    // Desligada na conta: vazia, sem rede nem tentativa repetida. Religar
    // muda apenas esta fileira para pendente, inclusive com a pasta aberta.
    if (desligada) continue;
    VitPedido *p = vitPedidos[r];
    if (p && __atomic_load_n(&p->pronto, __ATOMIC_ACQUIRE)) {
      vitPedidos[r] = NULL;
      int valido = p->geracao == vitGeracao && p->sessao == vitSessao && p->atual &&
                   fonteDisponivel(&p->fonte, p->base);
      if (valido && vitDoCache[r] && vitLin[r].estado == VIT_PRONTA && vitMesmoConteudo(r, p)) {
        // Revalidacao sem novidade: nada muda na tela (sem piscar).
        printf("[vertudo] vitrine: fonte %d revalidada sem mudancas\n", r + 1);
      } else if (valido && !(vitDoCache[r] && p->resultado.erro && vitLin[r].estado == VIT_PRONTA)) {
        vitGuardarInicial(r, p);
        vitDoCache[r] = 0;
      } else if (vitDoCache[r] && vitLin[r].estado == VIT_PRONTA) {
        // A rede falhou: o cache continua valendo, sem nova tentativa agora.
      } else {
        vitLin[r].estado = VIT_PEND;
        vitRetentarEm[r] = agora + 1500u;
      }
      vitPedidoSoltar(p);
    }
    VitLin *l = &vitLin[r];
    // Revalidacao que nao coube na fila de pedidos: sai quando houver vaga.
    if (vitRevalidar[r] && l->estado == VIT_PRONTA && !vitPedidos[r] && r >= cargaIni && r <= cargaFim) {
      if (vitPedidoLargar(r) > 0) vitRevalidar[r] = 0;
      continue;
    }
    if (l->semFonte && !rever) continue;
    if (l->semFonte) l->estado = VIT_PEND;
    if (l->estado != VIT_PEND) continue;
    if (vitRetentarEm[r] && (Sint32)(agora - vitRetentarEm[r]) < 0) continue;
    vitRetentarEm[r] = 0;
    int disponivel = fonteDisponivel(s, base);
    if (!disponivel) {
      l->estado = VIT_VAZIA; l->semFonte = 1; continue;
    }
    l->semFonte = 0;
    if (vitManualPendente[r]) { vitManualLargar(r, agora); continue; }
    if (r < cargaIni || r > cargaFim) continue;   // longe do foco: espera a vez
    // COLCACHE PRIMEIRO (2.0.34): memoria ou disco publicam a fileira AGORA, sem
    // esqueleto. Fresco: nenhum pedido. Velho: pede em segundo plano e so troca
    // se vier diferente. Fileira que devolveu a memoria volta daqui.
    if (cacheLidas >= 3) continue;   // no maximo 3 leituras de cache por quadro
    cacheLidas++;
    { long idade = vitDoColcache(r);
      if (idade >= 0 && idade < COLCACHE_FRESCO_S) continue;
      if (idade >= 0) printf("[vertudo] vitrine: fonte %d do cache (%lds), revalidando\n", r + 1, idade); }
    { int rc = vitPedidoLargar(r);
      if (rc <= 0 && vitLin[r].estado == VIT_PRONTA) vitRevalidar[r] = 1;   // fila cheia: tenta depois
      else if (rc < 0) vitRetentarEm[r] = agora + 1500u; }
  }
  // As paginas pedidas pelo botao preservam o prefixo canonico da fileira;
  // os outros pedidos iniciais continuam publicando independentemente.
  if (vitManualFonte >= 0 && !desc_vertudo_carregando() &&
      (desc_vertudo_n_bruto() > 0 || desc_vertudo_fim() || desc_vertudo_erro() || semFonte ||
       (Sint32)(agora - vitManualDesde) > 2500)) {
    int r = vitManualFonte;
    if (r < vitN && vitManualGeracao == vitGeracao &&
        nv_perfil_sessao_atual(vitManualSessao) &&
        !memcmp(&vitManualCopia, &vitFontes[r], sizeof vitManualCopia) &&
        !strcmp(vitManualBase, vitBases[r]) && !vitDesligada[r])
      vitGuardarPagina(r);
    vitManualFonte = -1;
  }
  vitManualLiberarSlot();
  // DEVOLVE A MEMORIA das fileiras longe do foco: voltam a pendente e
  // recarregam quando a janela de carga chegar nelas.
  { int atras = vitFraca ? VIT_SOLTA_ATRAS_FRACA : VIT_SOLTA_ATRAS;
    int frente = vitFraca ? VIT_SOLTA_FRENTE_FRACA : VIT_SOLTA_FRENTE;
    for (int r = 0; r < vitN; r++) {
      if (r >= vitFR - atras && r <= vitFR + frente) continue;
      if (vitLin[r].estado != VIT_PRONTA || vitPedidos[r] || vitManualFonte == r ||
          vitManualPendente[r] || !vitLinBuf[r]) continue;
      // 2.0.39: a chave (gravada no ultimo estado consistente) leva a fileira
      // de volta para esta coluna quando ela recarregar.
      vitSxAjuste[r] = 0;
      vitLinSoltar(r);
      vitLin[r].n = vitLin[r].brutoN = vitLin[r].extraN = 0;
      vitLin[r].mais = 0; vitLin[r].pagina = 0;
      vitLin[r].estado = VIT_PEND;
      vitSx[r] = vitSv[r] = 0.0f;
      vitProjetoSujo = 1;
    } }
  // Mantem o indicador da primeira fileira pendente, sem serializar a fila.
  for (vitCarga = 0; vitCarga < vitN; vitCarga++)
    if (vitLin[vitCarga].estado == VIT_PEND || vitLin[vitCarga].estado == VIT_CARREGANDO) break;

}

// ABRIR UM TITULO (a grade e a Vitrine). O item NAO esta no catalogo global —
// veio de uma pagina que so esta tela leu. Entra por cat_acrescentar para que a
// tela de titulo possa abri-lo por indice, que e como todo o app trabalha.
static void abrirItem(const CatItem *src) {
  CatItem it = *src;
  // Item de fonte TMDB (issue #44): o id e "tmdb:<n>", nao imdb. Quem resolve e
  // o caminho sob-demanda ja usado pela filmografia de elenco: external_ids ->
  // imdb -> meta do Cinemeta -> cat_acrescentar, e o desc_titulo_pronto do loop
  // principal abre o detalhe.
  if (!strncmp(it.imdb, "tmdb:", 5)) {
    desc_pedir_titulo_tmdb(atol(it.imdb + 5), !strcmp(it.tipo, "series") ? "tv" : "movie");
  } else {
    int idx = it.imdb[0] ? cat_indice_por_imdb(it.imdb) : -1;
    if (idx < 0) idx = cat_acrescentar(&it);
    if (idx >= 0) pedAbrir = idx;
  }
}

// A fonte inteira conserva genero/provedor/filtros. Nas fileiras sinteticas,
// o mesmo grupo da Vitrine e projetado localmente sobre as paginas recebidas.
static int vitAbrirTudo(int r) {
  if (!aberta || lay != VT_LAY_VITRINE || !vitTemVerTudo(r) || !collection ||
      !nv_perfil_sessao_atual(vitSessao) || col_revisao() != vitColecoesRev) return 0;
  int f = vitMulti ? r : source;
  if (f < 0 || f >= collection->nSources) return 0;
  int snapshot = vitMulti ? r : 0;
  if (memcmp(&vitFontes[snapshot], &collection->sources[f], sizeof(ColSource)) ||
      strcmp(vitBases[snapshot], baseDaFonte(&collection->sources[f]))) return 0;
  vitTudoOrigem = collection; vitTudoPasta = *collection;
  vitTudoFonte = collection->sources[f];
  snprintf(vitTudoBase, sizeof vitTudoBase, "%s", baseDaFonte(&vitTudoFonte));
  vitTudoPasta.sources = &vitTudoFonte; vitTudoPasta.nSources = 1;
  vitTudoFonteAntes = source; vitTudoLinha = r;
  snprintf(vitTudoNome, sizeof vitTudoNome, "%s", vitLin[r].nome);
  vitTudoFiltro = vitMulti ? 0 : vitLin[r].porTipo ? 1 : 2;
  vitTudoGrupo = vitLin[r].grupo; vitTudoEscolha = layEscolha; vitTudoSemAuto = semVitrineAuto;
  vitTudoN = 0; vitTudoBruto = vitTudoMaisBruto = -1;
  for (int i = 0; i < vitN; i++)
    if (vitLin[i].estado == VIT_CARREGANDO) vitLin[i].estado = VIT_PEND;
  ctxhold_cancelar(&hold);
  snprintf(titulo, sizeof titulo, "%s", vitTudoNome);
  vitTudo = 1; collection = &vitTudoPasta; source = tabCursor = tabFocus = 0;
  layEscolha = VT_LAY_GRADE; semVitrineAuto = 1;
  openSource();
  return 1;
}
static int vitVoltarTudo(void) {
  if (!vitTudo) return 0;
  const ColFolder *f = vitTudoOrigem;
  if (!nv_perfil_sessao_atual(vitSessao)) { vitTudo = 0; vitCancelar(); aberta = 0; return 1; }
  int mudou = col_revisao() != vitColecoesRev;
  if (mudou) {
    f = NULL;
    for (int i = 0; i < col_n(); i++) {
      const ColFolder *atual = col_folder(i); char chave[160];
      if (!atual) continue;
      col_chave_pasta(atual, chave, sizeof chave);
      if (!strcmp(chave, vitColecaoChave) && !strcmp(atual->id, vitTudoPasta.id)) { f = atual; break; }
    }
  }
  vitTudo = 0; vitTudoTecla = 0;
  if (!f || !f->nSources) { vitCancelar(); aberta = 0; return 1; }
  collection = f; source = vitTudoFonteAntes < f->nSources ? vitTudoFonteAntes : 0;
  layEscolha = vitTudoEscolha; semVitrineAuto = vitTudoSemAuto;
  resolverLayout(); celLay = orderN = -1; tabFocus = exibAberta = ddAberto = 0;
  snprintf(titulo, sizeof titulo, "%s", collection->title);
  // Rebuild after replacement: preview rows still belong to the old sources.
  if (mudou) vitIniciar();
  vitFR = vitTudoLinha < vitN ? vitTudoLinha : 0;
  if (vitMulti) {
    // Nomes de abas podem repetir; a volta pertence a fonte, nao ao rotulo.
    for (int r = 0; r < f->nSources && r < vitN; r++)
      if (!memcmp(&f->sources[r], &vitTudoFonte, sizeof vitTudoFonte)) { vitFR = r; break; }
  } else {
    for (int r = 0; r < vitN; r++)
      if (vitLin[r].grupo == vitTudoGrupo && vitLin[r].porTipo == (vitTudoFiltro == 1)) { vitFR = r; break; }
  }
  vitFC = vitLin[vitFR].n; vitColLemb[vitFR] = vitFC; vitMexeu = 1;
  ctxhold_cancelar(&hold);
#ifdef NV_TOUCH_UI
  vitPonteiroInvalidar(0);
#endif
  return 1;
}

static int primeiraFonte(void) {
  for (int i = 0; i < collection->nSources; i++) {
    const ColSource *s = &collection->sources[i];
    if (s->prov[0] || baseDaFonte(s)[0]) return i;
  }
  return 0;
}
// A EXIBICAO ESCOLHIDA no trilho aberto, e ela fica gravada para esta pasta.
static void escolherExibicao(int prox) {
  if (vitTudo && prox == VT_LAY_VITRINE) { vitVoltarTudo(); return; }
  int antes = lay;
  exibAberta = 0;
  layEscolha = prox; if (!vitTudo && !perftv_enabled) gravarEscolha(collection, prox);
  resolverLayout(); foco = 0; scrollY = velY = 0; armarOnda();
  if (lay != antes && (lay == VT_LAY_VITRINE || antes == VT_LAY_VITRINE)) {
    // A Vitrine nao tem aba aberta e as outras exibicoes precisam de uma: a
    // fonte volta para a primeira que tem endereco, e o cursor para o botao.
    source = primeiraFonte(); openSource();
  }
  tabCursor = cabN();   // o botao "Exibição", em qualquer estilo de abas
}

// OK no botao recolhido: abre o trilho com o cursor na exibicao aberta.
static void abrirSeletor(void) {
  for (int k = 0; k < (int)sizeof LAY_CICLO; k++) if (LAY_CICLO[k] == lay) exibCursor = k;
  // Telefone: o trilho aberto nao e desenhado, entao OK troca direto para a proxima.
  if (vtTelefone()) { escolherExibicao(LAY_CICLO[(exibCursor + 1) % (int)sizeof LAY_CICLO]); return; }
  exibAberta = 1;
}
// A PROXIMA EXIBICAO, e ela fica gravada para esta pasta (atalho do perftv).
// Parte da escolha gravada: depois de "Padrão" vem a primeira.
static void alternarExibicao(void) {
  int de = layEscolha >= VT_LAY_AUTO && layEscolha < VT_LAY_N ? layEscolha : lay;
  for (int k = 0; k < (int)sizeof LAY_CICLO; k++)
    if (LAY_CICLO[k] == de) { escolherExibicao(LAY_CICLO[(k + 1) % (int)sizeof LAY_CICLO]); return; }
}

const char *vertudo_perf_tag(void) {
  static const char *const tags[] = {
    "collection-auto", "collection-grid", "collection-timeline", "collection-ranking",
    "collection-saga", "collection-decades", "collection-wall", "collection-showcase",
    "collection-cover-flow"
  };
  return lay >= 0 && lay < VT_LAY_N ? tags[lay] : "collection-unknown";
}
void vertudo_perf_cycle(void) {
  if (!perftv_enabled) return;
  if (!aberta || !collection) {
    const ColFolder *f = col_folder(0);
    if (!f || !f->nSources) return;
    vertudo_colecao(f);
    layEscolha = VT_LAY_GRADE; resolverLayout();
    source = primeiraFonte(); openSource();
  } else alternarExibicao();
  tabFocus = 0;
}

static void vitEvento(int k) {
  int enter = k == SDLK_RETURN || k == SDLK_KP_ENTER;
  VitLin *l;
  vitProjetar();
  vitMexeu = 1;
  if (tabFocus && chipOculto()) tabFocus = 0;
  if (tabFocus) {   // o botao "Exibição", acima do destaque
    if (k == SDLK_LEFT) { if (tabCursor > cabN()) tabCursor--; else pedMenu = 1; }
    else if (k == SDLK_UP) { if (ajustes_menu_topo()) pedMenu = 1; else pedRelogio = 1; }
    else if (k == SDLK_DOWN && (vitTemItens() || temMaisVistos())) {
      tabFocus = 0;
      if (!vitTemItens()) vitFocarMais();
    }
    else if (enter) abrirSeletor();   // sem abas, sem botao de estilo
    return;
  }
  if (vitFR >= vitN) { if (k == SDLK_UP && !chipOculto()) tabFocus = 1, tabCursor = nAbas(); return; }
  l = &vitLin[vitFR];
  if (k == SDLK_LEFT) { if (vitFC > 0) vitFC--; else pedMenu = 1; }
  else if (k == SDLK_RIGHT) { if (vitFC + 1 < vitColunas(vitFR)) vitFC++; }
  else if (k == SDLK_DOWN) {
    int r = vitLinhaVizinha(vitFR, 1);
    if (r >= 0) { vitColLemb[vitFR] = vitFC; vitFR = r; vitFC = vitColLemb[r]; }
  } else if (k == SDLK_UP) {
    int r = vitLinhaVizinha(vitFR, -1);
    if (r >= 0) { vitColLemb[vitFR] = vitFC; vitFR = r; vitFC = vitColLemb[r]; }
    else if (chipOculto()) { if (ajustes_menu_topo()) pedMenu = 1; }
    else tabFocus = 1, tabCursor = cabN();
  } else if (enter) {
    const CatItem *it = vitItem(vitFR, vitFC);
    if (it) abrirItem(it);
    else if (vitFC == l->n && vitTemVerTudo(vitFR)) {
      if (vitAbrirTudo(vitFR)) vitTudoTecla = k;
      return;
    }
    else if (l->mais) carregarMaisVistos(vitFR, 0);
  }
  if (vitFR < vitN && vitFC >= vitColunas(vitFR)) vitFC = vitLin[vitFR].n ? vitLin[vitFR].n - 1 : 0;
  vitLembrarFoco();
  vitLembrarLinha(vitFR, vitFC);   // 2.0.39: reprojecao no mesmo quadro nao desfaz o passo
}

// Medidas das fileiras: as da Home (home_vitrine_medida). O topo da primeira e
// o da Home Moderna; a fileira em foco sobe para ele e a seguinte espia embaixo.
#define VIT_TITULO_H 46.0f
// O TITULO DA FILEIRA SOBE (dono, 21/11: "Top 10 Series fica muito perto do poster focado"):
// a Home deixa NV_LEGACY_ROW_HEAD_H (64) do titulo aos cartazes; aqui eram 46, e com o anel de
// 4 px por fora o texto quase encostava. Os cartazes e o passo ficam onde estavam (a espiada da
// fileira de baixo, tests/vitgeom.c); so o titulo, e o bloco do destaque acima dele, sobem.
#define VIT_TITULO_SOBE (NV_LEGACY_ROW_HEAD_H - VIT_TITULO_H)
#define VIT_CAB_FOLGA 24.0f
#define VIT_VAO_FIL 80.0f    // World 22/11: vao entre blocos (sinopse/cartaz -> titulo da fileira), ~2 dedos na TV   // entre o pe do cabecalho e o bloco do destaque
static float vitPasso(void) {
  float w, h, p, rot;
  home_vitrine_medida(&w, &h, &p, &rot);
  // World 22/11 (dono, com a mao na TV: "uns 2 dedos, tanto em cima e em baixo"): do pe do
  // cartaz ao titulo da fileira seguinte VIT_VAO_FIL, o mesmo vao da sinopse ao titulo da
  // primeira (home_vitrine_texto_el). Eram ~22 px: o titulo de baixo encostava no cartaz.
  return VIT_TITULO_H + h + rot + VIT_VAO_FIL + VIT_TITULO_SOBE;
}
// Onde o esvanecer esquerdo das fileiras ZERA: na borda da rail (+12) quando
// ela esta na tela (o cartaz anterior ao foco passava por baixo dos icones,
// TCL 20/11); sem rail, a borda da tela, como sempre.
static float vitRailZero(void) {
  float rail = menu_rail_borda();
  return rail > 0.0f ? rail / gfx_escala() + 12.0f : 0.0f;
}
// Topo da primeira fileira: o do layout da Home (home_vitrine_topo).
// O teto do bloco do destaque (o pe do cabecalho da colecao): o mesmo de vitDesenhar.
static float vitTetoBloco(void) { return vtIlhaY() + NV_ILHA_H + 14.0f + NV_CTRL_H * 0.5f + 34.0f + VIT_CAB_FOLGA; }
static float vitTopo(void) {
  float t = home_vitrine_topo(vitPasso());
  // World 22/11: o bloco do destaque (logo, meta, 3 linhas de sinopse) cabe inteiro entre o
  // cabecalho e a 1a fileira; senao a fileira desce (no telefone, como sempre).
  if (!vtTelefone() && home_vitrine_layout() != HOME_LAYOUT_DINAMICA) {
    float min = vitTetoBloco() + home_vitrine_bloco_min() + VIT_VAO_FIL;
    if (t < min) t = min;
  }
  return t;
}
// Janela vertical da Vitrine (TV), como na referencia da Home: a fileira em foco
// fica SEMPRE no mesmo ponto (topo), logo abaixo da sinopse, inclusive a ultima;
// a janela vai do topo ate o pe da tela, entao a fileira seguinte espia embaixo
// e nenhum cartaz e cortado por uma janela menor que a tela. A anterior some
// acima de topo - 8 (o vao entre fileiras e maior). No telefone a rolagem e livre.
static float vitAlturaViewport(void) {
  return fmaxf(0.0f, NV_TELA_H - (vitTopo() - 8.0f));
}

// COLUNAS DA FILEIRA DA VITRINE (adaptativas): a conta esta em vitgeom.h (testada
// em tests/vitgeom.c). Mesma regra da grade (vtColunas): so cartoes inteiros, o
// cartaz nao encolhe, o vizinho nunca fica cortado na lateral, retrato ou
// paisagem. No telefone a rolagem continua livre ao toque.
// FOCO FIXO NA TV (vitgeom_fixo_*): o cartaz em foco fica na ancora e a fileira
// desliza por baixo dele; as pontas mostram meio vizinho que esvanece (por pixel,
// gfx_esvanecer_x) ate a borda da tela, com a mesma largura dos dois lados.
// World 22/11: as fileiras comecam no INICIO DO CONTEUDO (vtInicio: depois da rail fixa,
// alinhadas ao titulo e a sinopse do destaque) e o foco, da coluna 1 em diante, fica a
// espiada do anterior a direita (vitgeom_espia_*, a mesma regra da Home). Antes a fileira
// era centrada na tela INTEIRA: com a rail fixa e a escala de texto o titulo e os cartazes
// passavam por baixo dos icones da barra (TCL, Prime Video).
#define VIT_MARGEM_DIR 48.0f
static float vitJanela(void) { return NV_TELA_W - VIT_MARGEM_DIR - vtInicio(); }
static int vitFixoN(float w, float passo) { return vitgeom_cabem(vtInicio(), NV_TELA_W - VIT_MARGEM_DIR, w, passo); }
static float vitAncora(float w, float passo) { (void)w; (void)passo; return vtInicio(); }
static float vitAlvoX(int col, int total, float w, float passo) {
  return vitgeom_espia_alvo(col, total, w, passo, vitJanela(), vitgeom_espia_ant(w));
}
#ifdef NV_TOUCH_UI
static float vitToqueMaxY(void) {
  return fmaxf(0.0f, (vitN - 1) * vitPasso());
}
static float vitToqueMaxX(int r) {
  float w, h, passo, rot;
  home_vitrine_medida(&w, &h, &passo, &rot);
  return fmaxf(0.0f, (vitColunas(r) - 1) * passo + w - (NV_TELA_W - vtInicio() - 48.0f));
}
#endif
static void vitAtualizarCorpo(float dt, Uint32 agora);
// 2.0.44: PRE-AQUECIMENTO DURANTE O VOO. O carregamento ja comecou no clique (openSource); enquanto o cartao
// cresce, o que so seria pedido NO DESENHO (cartazes das primeiras fileiras e a arte do destaque) tambem e
// pedido, para o conteudo ja estar pronto quando aparecer. Nada e desenhado aqui; so pedidos ao cache (os
// mesmos arquivos e larguras que o desenho pede, sem especular alem das 3 primeiras fileiras visiveis).
static void vooAquecer(void) {
  if (collection) {   // 2.0.45: o banner (fundo de toda a transformacao) e a logo do cabecalho ficam quentes
    const char *ban = col_banner(collection), *wm = wordmarkServico();
    if (ban && ban[0]) (void)tex_obter_hero(ban);
    if (wm && wm[0]) (void)tex_obter_logo_larg(wm, 420.0f);
  }
  if (lay != VT_LAY_VITRINE || vtTelefone() || vitN <= 0) return;
  float esc = gfx_escala();
  gfx_escala_sair(vtEscala());
  float w, h, passo, rot;
  home_vitrine_medida(&w, &h, &passo, &rot);
  // D1 (2.0.45): a arte do TITULO focado nao entra na transformacao (o banner da pasta e o destaque); ela e
  // pedida so na revelacao (vitAtualizarCorpo). Os cartazes, em ordem de revelacao: 1a fileira, 2a, 3a.
  for (int r = 0; r < vitN && r < 3; r++) {
    int cols = (int)ceilf((NV_TELA_W - vitAncora(w, passo)) / passo) + 1;
    for (int c = 0; c < vitLin[r].n && c < cols; c++) (void)home_vitrine_cartao_aquecer(vitItem(r, c), w);
  }
  gfx_escala_sair(esc);
}
// 2.0.45: O PORTAO DA REVELACAO. Barato (so consultas ao cache, que tambem pedem): as listas das 3 primeiras
// fileiras resolvidas; na 1a todos os cartazes visiveis chegaram (ou falharam); na 2a e 3a, metade; a logo
// do cabecalho chegou (ou falhou). Fora da Vitrine de TV nao ha o que esperar.
static int vitTrPronto(void) {
  if (lay != VT_LAY_VITRINE || vtTelefone()) return 1;
  const char *wm = wordmarkServico();
  if (wm && wm[0] && !tex_obter_logo_larg(wm, 420.0f) && !tex_falhou(wm)) return 0;
  if (vitN <= 0) return 0;
  float esc = gfx_escala();
  gfx_escala_sair(vtEscala());
  float w, h, passo, rot;
  int ok = 1;
  home_vitrine_medida(&w, &h, &passo, &rot);
  for (int r = 0; ok && r < vitN && r < 3; r++) {
    const VitLin *l = &vitLin[r];
    if (!l->n) { if (l->estado == VIT_PEND || l->estado == VIT_CARREGANDO) ok = 0; continue; }
    int cols = (int)ceilf((NV_TELA_W - vitAncora(w, passo)) / passo) + 1, vis = 0, prontos = 0;
    for (int c = 0; c < l->n && c < cols; c++) { vis++; prontos += home_vitrine_cartao_pronto(vitItem(r, c), w); }
    if (r == 0 ? prontos < vis : prontos * 2 < vis) ok = 0;
  }
  gfx_escala_sair(esc);
  return ok;
}
// A fileira r tem o que mostrar: lista resolvida e os 3 primeiros cartazes
// visiveis (ou todos, se forem menos) chegaram ou falharam.
static int vitLinhaPronta(int r, float w, float passo) {
  const VitLin *l = &vitLin[r];
  int cols, vis = 0, prontos = 0;
  if (!l->n) return !(l->estado == VIT_PEND || l->estado == VIT_CARREGANDO);
  cols = (int)ceilf((NV_TELA_W - vitAncora(w, passo)) / passo) + 1;
  for (int c = 0; c < l->n && c < cols && vis < 3; c++) { vis++; prontos += home_vitrine_cartao_pronto(vitItem(r, c), w); }
  return prontos >= vis;
}
// 0..1 da entrada da fileira r na transformacao (substitui tr_fileira).
static float vitLinEntradaTr(int r, Uint32 agora, float w, float passo) {
  Uint32 piso;
  if (r >= VIT_LIN_ESPERA_MAX) return tr_fileira(r, vitTr.r);
  if (!vitRevelaEm) return 0.0f;
  agora = SDL_GetTicks();   // o relogio de agora (vitAgora e o do quadro anterior aqui)
  if (agora < vitRevelaEm) return 0.0f;
  if (!vitLinProntaEm[r]) {
    // Passado o teto do cartaz (VIT_CART_SEGURA_MS), entra de qualquer jeito: o
    // cartaz que faltar volta ao esqueleto de sempre, nunca a fileira presa.
    if (vitLinhaPronta(r, w, passo) || agora - vitRevelaEm >= VIT_CART_SEGURA_MS) vitLinProntaEm[r] = agora ? agora : 1u;
    else return 0.0f;
  }
  piso = vitRevelaEm + (Uint32)(TR_LIN_PASSO_MS * (float)(r < TR_LIN_MAX ? r : TR_LIN_MAX));
  if (r > 0 && r - 1 < VIT_LIN_ESPERA_MAX && vitLinProntaEm[r - 1]) {
    Uint32 ant = vitLinProntaEm[r - 1] + (Uint32)TR_LIN_PASSO_MS;
    if (ant > piso) piso = ant;
  } else if (r > 0) return 0.0f;   // a de cima ainda nao entrou: a ordem fica
  { Uint32 ini = vitLinProntaEm[r] > piso ? vitLinProntaEm[r] : piso;
    if (agora <= ini) return 0.0f;
    // Fileira que chega depois da cena assentada entra mais devagar (320 ms):
    // ela aparece sozinha sobre a Vitrine parada, sem o movimento em volta.
    return tr_suave((float)(agora - ini) / (ini - vitRevelaEm > (Uint32)TR_REVELA_MS ? 320.0f : TR_LIN_MS)); }
}
// D2 (2.0.45): PRE-CARGA POR PERMANENCIA. So o que o colcache ja tem (memoria ou disco, sem rede) das 3
// primeiras fontes; os cartazes visiveis vao para uma lista que vitPreverPasso pede um por quadro, so com o
// cache ocioso (tex_lookahead_pedir: nunca atrasa o que esta na tela). Nada fica alocado alem da lista.
// Carregamento progressivo (18/11): a 1a fileira vem no tamanho do cartao (e a
// que aparece primeiro na transformacao); a 2a e a 3a como PREVIA (decode
// reduzido do proprio arquivo), promovidas quando entram na tela.
static int vitPreverCheios;
void vertudo_prever(const ColFolder *f) {
  vitPreverN = vitPreverI = 0;
  vitPreverCheios = VIT_PREVER_MAX;
  if (!f || f->nSources <= 1 || aberta || vtTelefone()) return;
  int cap = ajustes_col_itens_fileira();
  if (cap > VIT_ITENS) cap = VIT_ITENS;
  CatItem *tmp = malloc(sizeof(CatItem) * (size_t)cap * 2);
  if (!tmp) return;
  float esc = gfx_escala();
  gfx_escala_sair(vtEscala());
  float w, h, passo, rot;
  home_vitrine_medida(&w, &h, &passo, &rot);
  int cols = (int)(NV_TELA_W / passo) + 2;
  vitPreverW = w;
  for (int r = 0; r < f->nSources && r < 3; r++) {
    const ColSource *src = &f->sources[r];
    char base[NV_ADDON_URL_MAX];
    ColCacheMeta m; long idade;
    snprintf(base, sizeof base, "%s", baseDaFonte(src));
    if (!fonteDisponivel(src, base)) continue;
    if (!colcache_ler(colcache_chave(src, base, cap), tmp, cap * 2, &m, &idade)) continue;
    if (r == 1) vitPreverCheios = vitPreverN;   // fim da 1a fileira: o resto vai como previa
    for (int c = 0; c < m.n && c < cap && c < cols && vitPreverN < VIT_PREVER_MAX; c++) {
      const char *cam = home_vitrine_cartao_caminho(&tmp[c]);
      if (!cam || !cam[0]) continue;
      snprintf(vitPrever[vitPreverN], sizeof vitPrever[0], "%s", cam);
      vitPreverTent[vitPreverN++] = 0;
    }
  }
  gfx_escala_sair(esc);
  free(tmp);
}
int vertudo_prever_pendentes(void) { return vitPreverN - vitPreverI; }
static void vitPreverPasso(void) {
  if (vitPreverI >= vitPreverN || aberta) return;
  int pediu = vitPreverI < vitPreverCheios ? tex_lookahead_pedir(vitPrever[vitPreverI], vitPreverW)
                                           : tex_previa_pedir(vitPrever[vitPreverI], vitPreverW);
  if (pediu || ++vitPreverTent[vitPreverI] > 30) vitPreverI++;
}
// AS MEDIDAS DA ATUALIZACAO NA MESMA ESCALA DO DESENHO. vertudo_desenhar desenha a
// Vitrine com gfx_escala_sair(vtEscala()), e o tamanho do cartaz (gfx_poster_k) e o
// topo (ANC) dependem da escala ativa. A atualizacao rodava na escala do app: o
// passo da fileira usado no alvo da rolagem era MAIOR que o do desenho, e a fileira
// em foco descia um pouco mais a cada tecla (vitFR * diferenca). Aqui as duas
// contas usam a mesma escala.
static void vitAtualizar(float dt, Uint32 agora) {
  float esc = gfx_escala();
  gfx_escala_sair(vtEscala());
  vitAtualizarCorpo(dt, agora);
  gfx_escala_sair(esc);
}
// PRE-CARGA DIRECIONAL (2.0.36). Pede os cartazes que o foco vai alcancar, pela
// direcao em que ele anda (ordem em vitgeom_prebusca). So especula com o cache
// ocioso e sem pressao (tex_lookahead_pedir), entao nunca atrasa o que esta
// visivel; um pedido novo por vez, no maximo 20 varreduras por segundo.
static void vitPrebuscar(float w, float passo, Uint32 agora) {
  static int antFR = -1, antFC = -1, eixo = 1, dh = 1, dv = 1;
  static Uint32 ultima;
  if (vitFR != antFR || vitFC != antFC) {
    if (antFR >= 0) {
      if (vitFR != antFR) { eixo = 1; dv = vitFR > antFR ? 1 : -1; }
      else { eixo = 0; dh = vitFC > antFC ? 1 : -1; }
    }
    antFR = vitFR; antFC = vitFC;
  }
  if (vtTelefone() || tabFocus || vitN <= 0 || vitFR < 0 || vitFR >= vitN || !vitLin[vitFR].n ||
      w <= 0.0f || passo <= 0.0f || agora - ultima < 50) return;
  ultima = agora;
  int nCols[VIT_LIN_MAX], lemb[VIT_LIN_MAX], nLin = vitN < VIT_LIN_MAX ? vitN : VIT_LIN_MAX;
  for (int r = 0; r < nLin; r++) { nCols[r] = vitLin[r].n; lemb[r] = vitColLemb[r]; }
  int fc = vitFC < vitLin[vitFR].n ? vitFC : vitLin[vitFR].n - 1;
  int nAdiante = (int)ceilf((NV_TELA_W - vitAncora(w, passo)) / passo);
  VgPre cand[32];
  int n = vitgeom_prebusca(vitFR, fc, eixo, dh, dv, nLin, nCols, lemb, nAdiante, cand, 32);
  for (int i = 0; i < n; i++) {
    CatItem *it = vitItem(cand[i].r, cand[i].c);
    if (it && home_vitrine_cartao_prebuscar(it)) break;
  }
}
static void vitAtualizarCorpo(float dt, Uint32 agora) {
  float w, h, passo, rot, alvo;
  vitCarregar(agora);
  vitProjetar();
  if (!vitMexeu) vitFocoInicial();
  vitLembrarFoco();
  vitLembrarLinha(vitFR, vitFC);   // 2.0.39: a coluna da fileira em foco, pela chave
  // Destaque: o item sob o foco; so troca depois de 140 ms parado e com a arte
  // nova decodificada (ou 700 ms de espera), e a arte nova entra em 250 ms.
  { int des = !vitItem(vitFR, vitFC) ? -1 : vitFR * vitCap + vitFC;
    CatItem *it = vitChave(des);
    if (des != vitHeroDes) { vitHeroDes = des; vitHeroDesde = agora; }
    if (it && des != vitHeroAtual) {
      Uint32 parado = agora - vitHeroDesde;
      // World (20/11): a cena so troca com a ARTE e a LOGO resolvidas (pronta ou
      // inexistente), para o fundo novo nunca chegar com a area da logo vazia.
      // Tetos: arte 700 ms (como antes), logo 1200 ms (rede travada); passou o
      // teto da logo, ela entra suave quando chegar (heroLogoEntra), sem nome no lugar.
      int arteOk = home_vitrine_arte_pronta(it) || parado >= 700;
      int logoOk = home_vitrine_logo_pronta(it) || parado >= 1200;
      if (vitHeroAtual < 0 || (parado >= 140 && arteOk && logoOk)) {
        // Empilha: o que estava na tela continua com o progresso que tinha.
        // Se ja eram 3 camadas, a de baixo sai e a seguinte termina na hora
        // (so no toque rapidissimo; no ritmo normal a de baixo ja acabou).
        if (vitHeroAnt2 >= 0 && vitChave(vitHeroAnt2)) vitHeroEntraAnt = 1.0f;
        vitHeroAnt2 = vitHeroAnt; vitHeroEntraAnt2 = vitHeroEntraAnt;
        vitHeroAnt = vitHeroAtual; vitHeroEntraAnt = vitHeroEntra;
        vitHeroAtual = des;
        vitHeroEntra = 0.0f;   // o primeiro destaque tambem entra (abrir ou trocar de Collection)
      }
    } else if (!it && !vitChave(vitHeroAtual)) vitHeroZerar();
    // Foco no "Ver tudo" ou numa fileira ainda sem titulo: o destaque que esta
    // na tela FICA (2.0.34). Zerar escurecia a tela inteira e o destaque
    // seguinte entrava animado de novo — a piscada ao subir/descer.
    // D1 (2.0.45): enquanto o BANNER DA PASTA e o destaque, o titulo focado nao entra: so depois da
    // transformacao, quando a arte dele chegou (pedida a partir da revelacao), pelo crossfade de sempre.
    int seguraTitulo = 0;
    if (vitBannerAtivo) {
      CatItem *h0 = vitChave(vitHeroAtual);
      if (vooTem) { seguraTitulo = 1; if (h0 && vitTr.fase >= TR_REVELA) (void)home_vitrine_arte_pronta(h0); }
      // Sem teto aqui de proposito: com o banner por baixo nada fica vazio, e soltar o titulo antes da arte
      // faria a arte dele aparecer de uma vez quando chegasse (tela cheia). Falha conta como resolvida.
      else if (h0 && (!home_vitrine_arte_pronta(h0) || !home_vitrine_logo_pronta(h0))) seguraTitulo = 1;
    }
    // LOGOS ADIANTADAS: os vizinhos do foco na fileira (+-1, +-2), sem fura-fila.
    // Quando o foco chega neles, a logo ja esta pronta e a cena troca inteira.
    if (vitFR >= 0 && vitFR < vitN) {
      static const int VIZ[4] = { 1, -1, 2, -2 };
      for (int k = 0; k < 4; k++) {
        CatItem *vz = vitItem(vitFR, vitFC + VIZ[k]);
        if (vz && home_vitrine_logo_prebuscar(vz)) break;
      }
    }
    if (seguraTitulo) vitHeroEntra = 0.0f;
    else if (vitHeroEntra < 1.0f) {
      vitHeroEntra += dt / VIT_HERO_DUR;   // troca do destaque: arte e texto juntos, sem salto
      if (vitHeroEntra >= 1.0f) { vitHeroEntra = 1.0f; vitHeroAnt = vitHeroAnt2 = -1; vitHeroEntraAnt = vitHeroEntraAnt2 = 1.0f; }
    }
    if (vitHeroAnt >= 0 && vitHeroEntraAnt < 1.0f) {   // a camada anterior termina de entrar, no mesmo ritmo
      vitHeroEntraAnt += dt / VIT_HERO_DUR;
      if (vitHeroEntraAnt >= 1.0f) { vitHeroEntraAnt = 1.0f; vitHeroAnt2 = -1; vitHeroEntraAnt2 = 1.0f; }
    }
    if (vitHeroAnt2 >= 0 && vitHeroEntraAnt2 < 1.0f)
      vitHeroEntraAnt2 = fminf(1.0f, vitHeroEntraAnt2 + dt / VIT_HERO_DUR);
    // O banner sai de cena quando o titulo ja entrou inteiro por cima dele.
    // Arte do titulo que FALHOU: o banner fica (melhor que a tela escura de uma vez); sai quando uma arte cobrir.
    if (vitBannerAtivo && !vooTem && vitHeroEntra >= 1.0f) { CatItem *h0 = vitChave(vitHeroAtual); if (h0 && home_vitrine_arte_ok(h0)) vitBannerAtivo = 0; }
  }
  home_vitrine_medida(&w, &h, &passo, &rot);
  vitPrebuscar(w, passo, agora);
  alvo = (float)vitFR * vitPasso();
  { // So cartaz VERTICAL (o deitado ja mostra a arte larga) de um titulo de
    // verdade (nao "Ver tudo"), fora do telefone e sem menu do cartaz aberto.
    int pode = ajustes_col_expandir() && !vtTelefone() && !tabFocus && w < h &&
               vitFR < vitN && vitFC < vitLin[vitFR].n && vitItem(vitFR, vitFC);
    if (!vitExpIniciado) { exp_zerar(&vitExp); vitExpIniciado = 1; }
    exp_passo(&vitExp, vitFR, vitFC, pode, ctx_aberto(), agora,
              (unsigned)(ajustes_col_expandir_atraso() * 1000.0f), dt, anim_politica_reduzida); }
  { // O FOCO ENTRA E SAI ANIMADO: anel e zoom crescem no cartaz novo enquanto o
    // anterior volta ao tamanho de repouso (nada de liga/desliga num quadro).
    int r = tabFocus ? -1 : vitFR, c = tabFocus ? -1 : vitFC;
    if (r != vitFocoR || c != vitFocoC) {
      if (vitFocoR >= 0 && vitFocoF > 0.05f) { vitFocoAntR = vitFocoR; vitFocoAntC = vitFocoC; vitFocoAntF = vitFocoF; }
      vitFocoR = r; vitFocoC = c; vitFocoF = 0.0f;
    }
    vitFocoF = anim_mola(vitFocoF, r >= 0 ? 1.0f : 0.0f, dt, vitW1());
    vitFocoAntF = anim_mola(vitFocoAntF, 0.0f, dt, vitW1());
    if (vitFocoAntF < 0.01f) vitFocoAntR = vitFocoAntC = -1; }
  float dtV = dt;
  { static Uint32 ultScroll;   // relogio proprio para as molas horizontais (TV lenta)
    float dtR = ultScroll && agora > ultScroll ? (float)(agora - ultScroll) / 1000.0f : dt;
    ultScroll = agora;
    if (dtR < dt) dtR = dt;
    dtV = dtR > 0.3f ? 0.3f : dtR; }
#ifdef NV_TOUCH_UI
  if (!vtTelefone()) {
    toquerol_limpar(&vitToqueY);
    for (int r = 0; r < VIT_LIN_MAX; r++) toquerol_limpar(&vitToqueX[r]);
    vitToqueLinha = -1; vitToqueGeracao = 0;
  }
  if (vitToqueY.livre) vitScroll = anim_clamp(vitScroll, 0.0f, vitToqueMaxY());
  else
#endif
  vitScroll = anim_mola2_tempo(&vitScrollV, vitScroll, alvo, dtV, (NV_MOLA2_SCROLL * ajustes_navegacao_velocidade_cartazes()));
  for (int r = 0; r < vitN; r++) {
    vitLinAlfa[r] = anim_mola(vitLinAlfa[r], r == vitFR && !tabFocus ? 1.0f : 0.78f, dt, NV_MOLA_TELA);
    if (vitLin[r].n > 0) vitLinEntra[r] = anim_mola(vitLinEntra[r], 1.0f, dt, NV_MOLA_TELA * 0.8f);
    // A fileira em foco anda ate o cartaz caber com um vizinho a vista.
#ifdef NV_TOUCH_UI
    if (vitToqueX[r].livre) vitSx[r] = anim_clamp(vitSx[r], 0.0f, vitToqueMaxX(r));
    else
#endif
    if (!vtTelefone()) {
      // TV: janela de cartoes inteiros; toda fileira assenta no alvo (nao so a
      // focada), entao nenhuma fica com um cartao cortado depois de repouso.
      int cab = vitFixoN(w, passo), col = r == vitFR ? vitFC : vitColLemb[r];
      float alvoX = vitAlvoX(col, vitColunas(r), w, passo);
      (void)cab;
      if (vitSxAjuste[r] == 2) { vitSx[r] = alvoX; vitSv[r] = 0.0f; }
      else if (vitSxAjuste[r] == 1)
        vitSx[r] += alvoX - vitAlvoX(vitSxColAnt[r], vitSxTotAnt[r], w, passo);
      vitSxAjuste[r] = 0;
      // Cartaz aberto perto do fim da fileira: ela anda o que falta para ele
      // caber antes da espiada da direita (e volta quando fecha).
      for (int k = 0; k <= EXP_SAI_N; k++) {
        const ExpCartao *ek = k ? &vitExp.sai[k - 1] : &vitExp.abre;
        int er = ek->r, ec = ek->c;
        float ef = ek->p;
        if (r == er && ef > 0.0f) {
          float A = vitAncora(w, passo), extra = (h * VIT_EXP_ASPECTO - w) * ef;
          float dir = A + ec * passo - alvoX + w + extra, lim = NV_TELA_W - VIT_MARGEM_DIR;
          if (dir > lim) alvoX += dir - lim;
        }
      }
      vitSx[r] = anim_mola2_tempo(&vitSv[r], vitSx[r], alvoX, dtV, vitW2());
    } else if (r == vitFR)
      vitSx[r] = anim_mola2(&vitSv[r], vitSx[r], vitFC > 1 ? (vitFC - 1) * passo : 0.0f,
                            dt, (NV_MOLA2_SCROLL * ajustes_navegacao_velocidade_cartazes()));
  }
}

#ifdef NV_TOUCH_UI
static int vitToqueDono(void) {
  return vtTelefone() && aberta && lay == VT_LAY_VITRINE &&
         perfis_sessao_atual(vitPonteiroSessao);
}
static int vitPonteiroFocar(int i, int geracao) {
  if (!vitToqueDono()) return 0;
  vitProjetar();
  int r = i / VIT_ITENS, c = i % VIT_ITENS;
  if (!vitPonteiroPronto || geracao != vitPonteiroGeracao ||
      r < 0 || r >= vitN || c < 0 || c >= vitLin[r].n ||
      !vitPonteiroVisivel[r][c] || !mesmaChave(&vitPonteiroChave[r][c], vitItem(r, c))) return 0;
  tabFocus = 0; vitFR = r; vitFC = vitColLemb[r] = c; vitMexeu = 1;

  return 1;
}
static void ponteiroVitrine(int i, int geracao) { (void)vitPonteiroFocar(i, geracao); }
static void abrirVitrine(int i, int geracao) {
  // An explicit activation consumes a stale tap; rejecting focus alone would
  // still let the pointer synthesize Return and open the previously focused title.
  if (vitPonteiroFocar(i, geracao)) abrirItem(vitItem(vitFR, vitFC));
}
static int vitPonteiroTudoFocar(int r, int geracao) {
  if (!vitToqueDono()) return 0;
  vitProjetar();
  if (!vitPonteiroPronto || geracao != vitPonteiroGeracao ||
      !vitTemVerTudo(r) || !vitPonteiroTudoVisivel[r] || col_revisao() != vitColecoesRev) return 0;
  tabFocus = 0; vitFR = r; vitFC = vitColLemb[r] = vitLin[r].n; vitMexeu = 1;
  return 1;
}
static void ponteiroVitrineTudo(int r, int geracao) { (void)vitPonteiroTudoFocar(r, geracao); }
static void abrirVitrineTudo(int r, int geracao) {
  if (vitPonteiroTudoFocar(r, geracao)) vitAbrirTudo(r);
}
static int toqueVitrineRolar(const PonteiroRolagem *e) {
  if (!vitToqueDono() || !vitPonteiroPronto) return 0;
  if (e->fase == PONT_ROL_INICIO) {
    vitToqueLinha = -1;
    if (e->eixoY) {
      if (!toquerol_evento(&vitToqueY, e)) return 0;
      vitScrollV = 0.0f;
    } else {
      for (int r = 0; r < vitN; r++)
        if (toquerol_evento(&vitToqueX[r], e)) {
          vitToqueLinha = vitFR = r; vitFC = vitColLemb[r]; vitSv[r] = 0.0f; break;
        }
      if (vitToqueLinha < 0) return 0;
    }
    tabFocus = 0; vitMexeu = 1;
    vitToqueGeracao = vitPonteiroGeracao; ctxhold_cancelar(&hold);
    return 1;
  }
  if (vitToqueGeracao != vitPonteiroGeracao) return 0;
  ToqueRolagem *rol = e->eixoY ? &vitToqueY :
    vitToqueLinha >= 0 && vitToqueLinha < vitN ? &vitToqueX[vitToqueLinha] : NULL;
  if (!rol) return 0;
  int ret = toquerol_evento(rol, e);
  if (e->fase == PONT_ROL_FIM || e->fase == PONT_ROL_CANCELAR) vitToqueLinha = -1;
  return ret;
}
static void toqueVitrineRetomarFoco(void) {
  if (!vitToqueDono()) return;
  float w, h, passo, rot;
  home_vitrine_medida(&w, &h, &passo, &rot);
  if (vitToqueY.livre) {
    float topo = vitTopo();
    float melhor = 1e9f, centro = topo - 8.0f + vitAlturaViewport() * .5f;
    for (int r = 0; r < vitN; r++) if (vitLin[r].n > 0) {
      float d = fabsf(topo + r * vitPasso() - vitScroll + VIT_TITULO_H + h * .5f - centro);
      if (d < melhor) { melhor = d; vitFR = r; vitFC = vitColLemb[r]; tabFocus = 0; }
    }
  }
  for (int r = 0; r < vitN; r++) {
    if (vitToqueX[r].livre && vitLin[r].n > 0) {
      int c = (int)((vitSx[r] + (NV_TELA_W - vtInicio()) * .35f) / passo);
      vitColLemb[r] = c < 0 ? 0 : c < vitColunas(r) ? c : vitColunas(r) - 1;
      if (vitFR == r) vitFC = vitColLemb[r];
    }
    toquerol_limpar(&vitToqueX[r]);
  }
  toquerol_limpar(&vitToqueY); vitToqueLinha = -1;

}
static float toqueVertudoMax(void) {
  return fmaxf(0.0f, VT_TOPO + celFim + VT_GAP_Y - NV_TELA_H + 120.0f);
}
static int toqueVertudoRolar(const PonteiroRolagem *e) {
  if (!layout_modo_mobile()) { toqueLivre = toqueNasAbas = 0; return 0; }
  float escala = vtEscala();
  if (e->fase == PONT_ROL_INICIO) {
    toqueNasAbas = 0;
    if (toquerol_evento(&toqueAbas, e)) { toqueNasAbas = 1; ctxhold_cancelar(&hold); return 1; }
    float x = e->x / escala, y = e->y / escala;
    if (!e->eixoY || x < vtInicio() || x >= (vtTelefone() ? vtFim() : VT_PAN_X) || y < VT_TOPO - 12.0f || y >= NV_TELA_H || nItens() < 1) return 0;
    toqueLivre = 1; velY = 0.0f; ctxhold_cancelar(&hold);
    return 1;
  }
  if (toqueNasAbas) return toquerol_evento(&toqueAbas, e);
  if (e->fase == PONT_ROL_MOVER || e->fase == PONT_ROL_INERCIA) {
    float antes = scrollY;
    scrollY = anim_clamp(scrollY - e->delta / escala, 0.0f, toqueVertudoMax());
    if (scrollY + NV_TELA_H >= VT_TOPO + celFim - 2.0f * (VT_CARD_H + VT_GAP_Y)) desc_vertudo_mais();
    return fabsf(scrollY - antes) > 0.001f;
  }
  return 1;
}
static void toqueVertudoRetomarFoco(void) {
  if (layout_modo_mobile() && toqueLivre && nItens() > 0) {
    float alvo = scrollY + NV_TELA_H * 0.30f - VT_TOPO, distancia = 1e9f;
    int coluna = celCol[foco];
    for (int i = 0; i < celN; i++) {
      float d = fabsf(celY[i] - alvo);
      if (d < distancia || (d == distancia && celCol[i] == coluna)) {
        distancia = d; foco = i;
      }
    }
    tabFocus = 0;
  }
  toqueLivre = 0;
}
#endif

void vertudo_evento(const SDL_Event *e) {
  int n, k;
  pedRelogio = 0;
  if (!aberta) return;
  if (vitTrocaPend && e->type == SDL_KEYDOWN && e->key.keysym.sym != SDLK_AC_BACK) return;   // trocando de Collection
  if (vitTudo && !nv_perfil_sessao_atual(vitSessao)) { vitTudo = 0; vitCancelar(); aberta = 0; return; }
  n = nItens();
  // O OK que abriu a grade termina aqui, sem ativar seu primeiro cartaz.
  if (vitTudoTecla && (e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) &&
      e->key.keysym.sym == vitTudoTecla) {
    if (e->type == SDL_KEYUP) vitTudoTecla = 0;
    return;
  }
  if (lay != VT_LAY_VITRINE) n = atualizarGrade();
#ifdef NV_TOUCH_UI
  if (toquerol_navegacao(e)) toquerol_limpar(&toqueAbas);
  if (e->type == SDL_KEYDOWN) {
    if (lay == VT_LAY_VITRINE) toqueVitrineRetomarFoco();
    else toqueVertudoRetomarFoco();
  }
#endif
  if (lay != VT_LAY_VITRINE) lembrarFoco();
  switch (ctxhold_evento(&hold, e, celulaAceitaMenu())) {
    case CTXH_LONGO: if (!menuNoFocado()) abrirFocado(); return;
    case CTXH_CONSUMIDO: return;
    case CTXH_TOQUE: abrirFocado(); return;
    default: break;
  }
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) {
    if (ddAberto) { ddAberto = 0; return; }   // Voltar fecha a lista, nao a pagina
    if (exibAberta) { exibAberta = 0; return; }   // e recolhe o seletor
    if (vitVoltarTudo()) return;
    vitCancelar(); aberta = 0; return;
  }
  if (n > VT_MAX) n = VT_MAX;
  exibAberta = exibAberta && tabFocus && tabCursor == cabN();   // o ponteiro tirou o foco
  if (exibAberta) {   // o trilho aberto fica com as setas ate escolher ou sair
    if (k == SDLK_RIGHT) { if (exibCursor + 1 < (int)sizeof LAY_CICLO) exibCursor++; return; }
    if (k == SDLK_LEFT && exibCursor > 0) { exibCursor--; return; }
    exibAberta = 0;
    if (k == SDLK_RETURN || k == SDLK_KP_ENTER) { escolherExibicao(LAY_CICLO[exibCursor]); return; }
  }
  if (lay == VT_LAY_VITRINE) { vitEvento(k); return; }
  // #414: OK on the current catalog retries even while the header has focus.
  // Other tabs, the layout button and the Menu selector (menuOk) keep theirs.
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && desc_vertudo_erro() &&
      (!collection || !tabFocus || (estiloAbas() != VT_AB_MENU && tabCursor < cabN() && tabCursor == cursorInicial()))) {
    desc_vertudo_mais(); return;
  }
  if(collection&&tabFocus) {
    int na = cabN(), est = estiloAbas();
    if(ddAberto) { ddEvento(k); return; }
    if(k==SDLK_LEFT||(k==SDLK_UP&&ajustes_menu_topo())){ if(k==SDLK_LEFT&&tabCursor>0)tabCursor--; else pedMenu=1; }
    if(k==SDLK_UP&&!ajustes_menu_topo()&&!chipOculto())pedRelogio=1;
    if(k==SDLK_RIGHT&&tabCursor<(chipOculto()?na-1:na+(nAbas()>0)))tabCursor++;   // o estilo so com abas
    if(chipOculto()&&tabCursor>=na)tabCursor=na>0?na-1:0;
    // Paginas: a seta ja troca de pagina (o deslize e o feedback).
    if((k==SDLK_LEFT||k==SDLK_RIGHT)&&est==VT_AB_PAGINAS&&tabCursor<na&&tabCursor!=source)irPagina(tabCursor);
    if(k==SDLK_RETURN||k==SDLK_KP_ENTER) {
      if(tabCursor>na) alternarEstiloAbas();
      else if(tabCursor==na) {
        abrirSeletor();
      } else if(est==VT_AB_MENU) menuOk();
      else if(est==VT_AB_PAGINAS) {
        // Um clique numa seta/ponto so move o cursor; o OK que o segue troca.
        if(tabCursor!=source)irPagina(tabCursor); else tabFocus=0;
      } else {source=tabCursor;openSource();tabFocus=0;}
    }
    if(k==SDLK_DOWN&&(n>0||temMaisVistos()))tabFocus=0;
    return;
  }
  if (precisaMontar(n)) montar(n);
  if(k==SDLK_UP&&collection&&(n<1||celLin[foco]==0)){
    if(chipOculto()&&cabN()==0){if(ajustes_menu_topo())pedMenu=1;return;}   // sem abas e sem botao: nada acima
    tabFocus=1;tabCursor=cursorInicial();return;}
  if((k==SDLK_RETURN||k==SDLK_KP_ENTER)&&desc_vertudo_erro()){desc_vertudo_mais();return;}
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && !e->key.repeat && temMaisVistos()) {
    carregarMaisVistos(0, 0); return;
  }
  // ESQUERDA no primeiro item de uma linha: a barra lateral (ver pedMenu).
  if (k == SDLK_LEFT && (n < 1 || foco == 0 || celLin[foco - 1] != celLin[foco])) { pedMenu = 1; return; }
  if (n < 1) { if (k == SDLK_UP) pedRelogio = 1; return; }
  if (k == SDLK_RIGHT && foco + 1 < n) foco++;
  else if (k == SDLK_LEFT) foco--;
  else if (k == SDLK_DOWN && lay == VT_LAY_FLUXO) return;
  else if (k == SDLK_DOWN) { int v = vizinhoNaLinha(n, celLin[foco] + 1);
                             foco = v >= 0 ? v : n - 1; }
  else if (k == SDLK_UP) { int v = vizinhoNaLinha(n, celLin[foco] - 1);
                           if (v >= 0) foco = v; else if (ajustes_menu_topo()) pedMenu = 1; else pedRelogio = 1; }
  else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) abrirFocado();
  lembrarFoco();
  // Chegando perto do fim, pede a proxima pagina. Antes de o dono ver o vazio,
  // e nao quando ele ja esta olhando para ele.
  if (vitTudo && vitTudoFiltro) vitTudoAvancar(n);
  else if (vertudo_quer_mais(ordenaPorAno(), n, foco, 600, desc_vertudo_carregando(),
                        desc_vertudo_fim(), desc_vertudo_erro())) desc_vertudo_mais();
}

// TETO DO SCROLL PARA A LINHA EM FOCO (vertudo_atualizar): o maior scroll
// que mantem o TOPO do cartaz focado inteiro abaixo do recorte do cabecalho
// (VT_TOPO - 12), com folga para tudo o que o foco puxa para fora da caixa
// (focoprof.h). Linha 0 devolve negativo — scroll zero, cartaz colado no VT_TOPO.
static float tetoScrollFoco(float celY, float celH) {
  return celY + 12.0f - (celH * FOCO_ZOOM * 0.5f + NV_ANEL_FOCO);
}

void vertudo_atualizar(float dt, Uint32 agora) {
  vitManualLiberarSlot();
#ifdef NV_TOUCH_UI
  if (!layout_modo_mobile()) toqueLivre = toqueNasAbas = 0;
#endif
  float alvo, maxY;
  int n = nItens();
  anim = anim_mola(anim, aberta ? 1.0f : 0.0f, dt, NV_MOLA_TELA);
  if (vitEntradaT < HT_ENT_TOTAL_MS) vitEntradaT += dt * 1000.0f; else vitEntradaT = 1.0e6f;
  vitPreverPasso();
  if (vooTem && !aberta) vooSaindo = 1;   // Voltar no meio: o relogio para e tudo apaga com a Vitrine
  if (vooTem) {
    if (vooSaindo) { if (anim < 0.01f) { vooTem = 0; vooSaindo = 0; tr_parar(&vitTr); } }
    else {
      const char *ban = collection ? col_banner(collection) : NULL;
      GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
      // O destaque cobre a tela (Moderna/Dinamica) e o banner existe: nada da Vitrine por baixo do cartao.
      vooDestCobre = vooDestino.x <= 0.5f && vooDestino.y <= 0.5f && vooDestino.w >= tela.w - 0.5f &&
                     vooDestino.h >= tela.h - 0.5f && ban && ban[0] && !tex_falhou(ban);
      vooAquecer();
      tr_passo(&vitTr, dt * 1000.0f, vitTrPronto());
      if (vitTr.fase >= TR_REVELA && !vitRevelaEm) vitRevelaEm = agora ? agora : 1u;
      if (!tr_ativo(&vitTr)) { vooTem = 0; vitBannerSolta = agora ? agora : 1u; }
      // Vitrine: cada parte tem a propria curva (anim = 1). Grade/outras exibicoes da pasta: o fundo opaco delas
      // acende com o relogio por baixo do cartao, como na 2.0.44 (nunca uma tela escura de uma vez sobre a Home).
      anim = lay == VT_LAY_VITRINE ? 1.0f : tr_fundo(vitTr.p);
    }
  }
  if (!aberta) { vitTrocaPend = NULL; ctxhold_cancelar(&hold); return; }
  if (vitTrocaPend) {   // saida da Collection atual (~250 ms), depois a troca
    vitTrocaSai += dt / 0.25f;
    if (vitTrocaSai >= 1.0f) {
      const ColFolder *f = vitTrocaPend;
      vitTrocando = 1; vertudo_colecao(f); vitTrocando = 0;
      vitTrocaSai = 0.0f;
    }
    return;
  }
  if (vitTudo && !nv_perfil_sessao_atual(vitSessao)) { vitTudo = 0; vitCancelar(); aberta = 0; return; }
  if (lay != VT_LAY_VITRINE) n = atualizarGrade();
  // SEGUROU OK NUM CARTAO ATE O LIMIAR: o menu do cartaz, com o dedo ainda no
  // botao (ctxlista.h).
  if (ctxhold_passo(&hold, agora, 1) && menuNoFocado()) ctxhold_cancelar(&hold);
  if (n > VT_MAX) n = VT_MAX;
  vitTudoAvancar(n);
  // Tela cronologica pede todas as paginas sem esperar o foco descer.
  if (!(vitTudo && vitTudoFiltro) && ordenaPorAno() && vertudo_quer_mais(1, n, foco, VT_MAX, desc_vertudo_carregando(),
                                          desc_vertudo_fim(), desc_vertudo_erro()))
    desc_vertudo_mais();
  // A SONDA PODE CHEGAR DEPOIS DA TELA. Enquanto a fonte nao tem endereco,
  // reconferir por quadro custa uma varredura de poucas strings (addons_n e
  // dezenas, nao milhares) e SO acontece no estado de erro — assim a colecao se
  // preenche sozinha quando o manifesto responde, em vez de exigir sair e
  // entrar de novo.
  if (semFonte && collection && lay != VT_LAY_VITRINE) {
    const ColSource *s = &collection->sources[source];
    if (baseDaFonte(s)[0]) openSource();
  }
  for (int i = 0; i < COL_SOURCE_MAX; i++) {
    float alvoTab = collection && tabFocus && i == tabCursor && i < cabN() ? 1.0f : 0.0f;
    tabAnim[i] = anim_mola(tabAnim[i], alvoTab, dt,
                           alvoTab > tabAnim[i] ? NV_MOLA_FOCO : NV_MOLA_DESFOCO);
  }
  { float alvoP = collection && tabFocus && tabCursor == cabN() ? 1.0f : 0.0f;
    pillAnim = anim_mola(pillAnim, alvoP, dt, alvoP > pillAnim ? NV_MOLA_FOCO : NV_MOLA_DESFOCO); }
  abasCapsulaAtualizar(dt);
  if (!tabFocus) ddAberto = 0;
  ddAnim = anim_mola(ddAnim, ddAberto ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  pgSlide = anim_mola(pgSlide, 0.0f, dt, NV_MOLA_TELA);
  if (barraW < 0.0f) { barraX = barraAlvoX; barraW = barraAlvoW; }
  else { barraX = anim_mola(barraX, barraAlvoX, dt, NV_MOLA_FOCO * 0.6f);
         barraW = anim_mola(barraW, barraAlvoW, dt, NV_MOLA_FOCO * 0.6f); }
  if (lay == VT_LAY_VITRINE) { vitAtualizar(dt, agora); return; }
  n = atualizarGrade();
  if (lay == VT_LAY_LINHA && celN > 0) {
    float alvoP = celY[foco < celN ? foco : celN - 1] + VT_TL_PONTO;
    for (int i = 0; i < celN; i++) {
      float alvoF = i == foco && !tabFocus ? 1.0f : 0.0f;
      tlFoco[i] = anim_mola(tlFoco[i], alvoF, dt, alvoF > tlFoco[i] ? NV_MOLA_FOCO : NV_MOLA_DESFOCO);
    }
    tlProg = tlProg < 0.0f ? alvoP : anim_mola(tlProg, alvoP, dt, NV_MOLA_FOCO);
  }
  if (lay == VT_LAY_FLUXO && celN > 0)
    fluxoPos = fluxoPos < 0.0f ? (float)foco : anim_mola(fluxoPos, (float)foco, dt, NV_MOLA_FOCO);
  if ((lay == VT_LAY_SAGA || listaDePessoa()) && n <= 200 && (Sint32)(agora - vistosEm) > 1500)
    { contarVistos(n); vistosEm = agora; }
  // Mira a linha focada a 30% da altura util, como o resto do app faz.
  alvo = n ? VT_TOPO + celY[foco] - NV_TELA_H * 0.30f : 0.0f;
  // Decadas: o primeiro cartaz de uma secao leva o cabecalho junto.
  // Saga: a trilha de capitulos fica acima da linha e entra junto.
  if (lay == VT_LAY_SAGA) alvo -= VT_SAGA_TRILHA;
  if (lay == VT_LAY_DECADAS && n)
    for (int k = 0; k < decN; k++) if (decY[k] + VT_DEC_H == celY[foco]) { alvo -= VT_DEC_H; break; }
  if(tabFocus)alvo=0;
  // O CARTAZ EM FOCO NAO ENTRA POR BAIXO DO RECORTE DO CABECALHO. A mira em
  // 30% deixa o topo da linha focada a 4 px do corte (VT_TOPO - 12), e o que o
  // foco tem por fora da caixa — o crescimento de FOCO_ZOOM ou o anel de
  // NV_ANEL_FOCO (focoprof.h) — era cortado no topo (dono, 14/10: "quando ta
  // focado, fica cortado no top"). O teto garante a folga; a Home resolve o
  // mesmo problema mantendo a fileira em foco sempre no mesmo Y, abaixo do
  // corte dela.
  if (n && celN > 0 && foco >= 0 && foco < celN) { float teto = tetoScrollFoco(celY[foco], celH[foco]);
    if (alvo > teto) alvo = teto; }
  maxY = VT_TOPO + celFim + VT_GAP_Y - NV_TELA_H + 120.0f;
  if (maxY < 0.0f) maxY = 0.0f;
  if (alvo < 0.0f) alvo = 0.0f;
  if (alvo > maxY) alvo = maxY;
#ifdef NV_TOUCH_UI
  if (toqueLivre) scrollY = anim_clamp(scrollY, 0.0f, maxY);
  else
#endif
    scrollY = anim_mola2(&velY, scrollY, alvo, dt, (NV_MOLA2_SCROLL * ajustes_navegacao_velocidade_cartazes()));
}

// PAINEL DA DIREITA: o que a grade sozinha nao diz — sinopse, generos, nota.
//
// Sem ele o dono ve 50 cartazes e nenhuma informacao; era o que faltava para a
// tela deixar de ser so uma parede de imagens.
//
// FICHA DIAGRAMADA (dono, 13/10: "na lateral nao colocar o poster de novo,
// vamos colocar mais informacoes sobre o titulo"). So a Grade (a exibicao de
// cartazes), o Cover Flow, o Ranking e as Decadas dispensam o cartaz lateral,
// porque la o cartaz ja esta na celula grande — Decadas vale para o streaming
// tambem, a mesma tela (dono, 14/10: "no modo ranking dos streaming ainda ta
// mostrando o poster, tem que tirar, decades tb"); nas outras o cartaz fica.
// Com o cartaz na ficha, a LOGO tambem nao entra: so o nome escrito (dono,
// 14/10: "quando tem poster nas infos nao colocar a logo"). Ordem:
// logo/titulo, ano e
// duracao com a classificacao, generos, notas, sinopse e, quando o catalogo ja
// os trouxe,
// direcao e elenco. Nada aqui pede rede: e so o que o CatItem tem.
static int painelComCartaz(void) {
  return lay != VT_LAY_GRADE && lay != VT_LAY_FLUXO && lay != VT_LAY_RANKING &&
         lay != VT_LAY_DECADAS;
}
static float painelCredito(const char *rot, const char *val, float y, float a) {
  TxtLinha r = txt_linha(TXT_MINI, rot, 160, 168, 182, 255);
  txt_desenhar_alpha(r, VT_PAN_X, y, a);
  y += r.h + 4.0f;
  return y + txt_bloco_corta(TXT_DET_META2, val, 226, 230, 238, VT_PAN_X, y, VT_PAN_W, 30.0f, a, 2) + 14.0f;
}
static void painel(float a) {
  CatItem it;
  // A base fica a mesma distancia da borda que a direita (a da ilha).
  float y = VT_PAN_Y, base = NV_TELA_H - NV_ILHA_MARGEM_D;
  int cartaz = painelComCartaz();
  char elenco[200] = "";
  if (!viewItem(foco, &it)) return;

  if (cartaz) {
    // Mantém a geometria 2:3 mesmo sem arte para não deslocar os metadados.
    GfxRect r = { VT_PAN_X, y, VT_PAN_ART_W, VT_PAN_ART_H };
    const char *arte = posterprov_card_addon(it.origem, it.imdb, it.tmdb, it.tipo,
                                              it.poster[0] ? it.poster : it.backdrop);
    float raio = ajustes_raio_poster_px() / VT_PAN_ART_H;   // o raio de cartaz do app
    GLuint t = arte[0] ? tex_obter_larg(arte, VT_PAN_ART_W) : 0;
    gfx_cor(r, raio, 1, 1, 1, 0.05f * a);
    if (t) {
      gfx_tex_aspect_atual = tex_aspecto(arte);
      gfx_rect(r, t, GFX_CARD, 0, 0, 0, raio, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    } else {
      TxtLinha falta = txt_linha_corta(TXT_HERO_META, "Sem pôster", 185, 191, 204, 255, r.w-24);
      txt_desenhar_alpha(falta, r.x+(r.w-falta.w)*.5f, r.y+(r.h-falta.h)*.5f, a);
    }
    y += VT_PAN_ART_H + 20.0f;
  } else y += 8.0f;

  // LOGO so quando a ficha NAO tem o cartaz: com o poster ao lado o nome
  // escrito com a fonte da interface (dono, 14/10: "quando tem poster nas
  // infos nao colocar a logo"). Sem o cartaz ele cresce e e a ancora da ficha.
  { float maxW = VT_PAN_W, maxH = cartaz ? 82.0f : 128.0f;
    GLuint tl = !cartaz && it.logo[0] ? tex_obter_logo_larg(it.logo, maxW) : 0;
    float ap = tl ? tex_aspecto(it.logo) : 0.0f;
    if (tl && ap > 0.0f) {
      float wL = maxW, hL = wL / ap;
      if (hL > maxH) { hL = maxH; wL = hL * ap; }
      { GfxRect rl = { VT_PAN_X, y, wL, hL };
        GfxModo m = tex_marca_escura(it.logo) ? GFX_MARCA : GFX_TEXTO;
        gfx_tex_aspect_atual = 0.0f;
        gfx_rect(rl, tl, m, 0, 0, 0, 0.0f, 1, 1, 1, a); }
      y += hL + 16.0f;
    } else {
      // Titulo longo nao corta na 2a linha ("The Beat of the..."): desce um
      // degrau da escala e ganha a 3a.
      TxtEstilo es = cartaz ? TXT_HEADLINE : TXT_TITULO2;
      float ld = cartaz ? 38.0f : 58.0f;
      int nl = 2;
      if (txt_bloco(es, it.titulo, 0, 0, 0, 0, 0, VT_PAN_W, ld, 0.0f, 0) > 2.0f * ld + 1.0f) {
        es = cartaz ? TXT_CALLOUT : TXT_HEADLINE; ld = cartaz ? 34.0f : 44.0f; nl = 3; }
      y += txt_bloco_corta(es, it.titulo, 245, 245, 245, VT_PAN_X, y, VT_PAN_W, ld, a, nl) + 18.0f;
    } }

  // Ano · duracao (ou temporadas) e o selo da classificacao na mesma linha.
  { float x = VT_PAN_X;
    if (it.meta[0]) {
      TxtLinha t = txt_linha_corta(TXT_DET_META2, it.meta, 240, 241, 244, 255, VT_PAN_W - 90.0f);
      txt_desenhar_alpha(t, x, y + (BADGE_H - t.h) * 0.5f, a);
      x += t.w + 14.0f;
    }
    if (it.classificacao[0]) badge_desenhar(x, y, it.classificacao, BADGE_NEUTRO, a);
    if (it.meta[0] || it.classificacao[0]) y += BADGE_H + 12.0f; }
  if (it.genero[0])
    y += txt_bloco_corta(TXT_CAPTION, it.genero, 196, 205, 219, VT_PAN_X, y, VT_PAN_W, 28.0f, a * 0.9f, 2) + 14.0f;

  // Notas: IMDb (amarelo da marca) e os servicos onde o titulo esta.
  { int nota = imdbnota_obter(it.imdb, it.nota, !strcmp(it.tipo, "series"));
    float x = VT_PAN_X, w = badge_imdb(x, y, nota, 0, a);
    if (w > 0) x += w + 18.0f;
    w = badges_desenhar(badges_provedor(it.provNome), x, y, VT_PAN_X + VT_PAN_W - x, BADGE_H, a);
    if (x > VT_PAN_X || w > 0) y += BADGE_H + 20.0f; }

  // Os creditos ficam na base; a sinopse ocupa o que sobra entre os dois.
  for (int k = 0, m = 0; k < it.nElenco && k < 3; k++)
    if (it.elenco[k].nome[0])
      m += snprintf(elenco + m, sizeof elenco - (size_t)m, "%s%s", m ? ", " : "", it.elenco[k].nome);
  { float creditos = (it.direcao[0] ? 92.0f : 0.0f) + (elenco[0] ? 92.0f : 0.0f);
    if (it.sinopse[0]) {
      // Sem espaco para tudo, os creditos saem antes das tres linhas da sinopse.
      float ld = sinopse_entrelinha(TXT_DET_META2, 32.0f);
      int linhas = (int)((base - creditos - y) / ld), cabe = (int)((base - y - 12.0f) / ld);
      if (linhas < 3) linhas = cabe < 3 ? cabe : 3;
      if (linhas > (cartaz ? 6 : 9)) linhas = cartaz ? 6 : 9;
      if (linhas > 0) {
        y += 6.0f;
        y += txt_bloco_corta(sinopse_estilo(TXT_DET_META2), it.sinopse, 226, 229, 236,
                             VT_PAN_X, y, VT_PAN_W, ld, a * 0.9f, linhas) + 18.0f;
      }
    }
    if (it.direcao[0] && y + 70.0f <= base) y = painelCredito(i18n("Direção"), it.direcao, y, a);
    if (elenco[0] && y + 70.0f <= base) painelCredito(i18n("Elenco"), elenco, y, a); }
}
// FUNDO DA COLECAO: sempre Frost ou a arte borrada da pasta, nunca a arte crua
// ("em colecoes deixa o fundo sempre frost ou blur", 04/10/2026). Ajustes >
// Fundo = Frost fica Frost; senao a arte borrada (fundo.c), e sem a arte
// ainda cai no Frost. A arte de cabecalho (editorial) vem POR CIMA.
static void fundoColecao(float a) {
  GfxRect tela={0,0,NV_TELA_W,NV_TELA_H};
  GLuint t=0;
  const char *art=collection?(collection->editorial&&collection->detailHero[0]
                              ?collection->detailHero:col_banner(collection)):"";
  int modo=fundo_modo()==FUNDO_FROST?FUNDO_FROST:FUNDO_BORRADA;
  if(art&&art[0])t=tex_obter_hero(art);   /* a Borrada desfoca esta textura */
  if(modo==FUNDO_BORRADA&&!t)modo=FUNDO_FROST;
  fundo_desenhar_modo(modo,tela,0.0f,art,a);
}
static void themeBackground(float a) {
  fundoColecao(a);
  degradeMarca(a, NV_SHELF_TOP);
  if(collection) {
    if(collection->editorial) {
      GLuint art=tex_obter_hero(collection->detailHero);
      /* The content starts at 332; the separate detail illustration ends at 320. */
      float telaW = vtTelefone() ? NV_TELA_W : 1920.0f;
      GfxRect header={0,0,telaW,320};
      if(collection->editorial==2&&tex_aspecto(collection->detailHero)>0) {
        float aspect=tex_aspecto(collection->detailHero);
        header.w=fminf(telaW,header.h*aspect);header.h=header.w/aspect;
        header.x=telaW-header.w;
      }
      if(art)gfx_rect(header,art,collection->editorial==2?GFX_EDITORIAL:GFX_TEXTO,0,0,0,0,1,1,1,a);
      return;
    }
  }
}

// PONTEIRO (#99). As mesmas variaveis das setas: a aba sob o cursor ganha o
// cursor das abas, o cartaz ganha o foco da grade; o OK do clique segue o
// caminho de sempre (aba abre a fonte, cartaz abre o titulo ou, segurado, o
// menu do cartaz).
static void ponteiroAba(int i, int b) {
  (void)b;
  if (!collection || i < 0 || i >= collection->nSources) return;
#ifdef NV_TOUCH_UI
  toqueLivre = 0;
  toquerol_limpar(&toqueAbas);
#endif
  tabFocus = 1; tabCursor = i;
}
// Menu suspenso: um item da lista. O OK do clique escolhe (ddEvento).
static void ponteiroDd(int i, int b) {
  (void)b;
  tabFocus = 1; tabCursor = cabN() - 1; ddAberto = 1; ddCursor = i;
}
static void ponteiroCartaz(int i, int b) {
  (void)b;
  atualizarGrade();
  // O alvo foi registrado no ultimo desenho. Se a projecao mudou desde la,
  // este indice pertence a outro cartaz: o proximo quadro registra alvos novos.
  if (!gradePonteiroValido || gradePonteiroRevisao != gradeRevisao) return;
  if (i < 0 || i >= nItens()) return;
#ifdef NV_TOUCH_UI
  toqueLivre = 0;
#endif
  tabFocus = 0; foco = i; ddAberto = 0;
  lembrarFoco();
  if (vitTudo && vitTudoFiltro) vitTudoAvancar(nItens());
  else if (foco >= nItens() - VT_COLS * 2) desc_vertudo_mais();
}

// Streaming com a lista pronta: a contagem e "organizado por servico" so
// poluiam o cabecalho sob o wordmark. Carregando/erro/sem fonte seguem falando.
static int legendaOculta(void) {
  return grupo("Streaming")&&nItens()>0&&!semFonte&&!desc_vertudo_erro();
}
#define VT_ABA_FADE 72.0f
// ROTULOS DAS ABAS, NA ORDEM DO CATALOGO (sources[0..n]). NOME DA ABA (01/10,
// pasta Netflix mostrava "streaming_netflix_movies · Movies"): o nome legivel
// da fonte (titulo da conta, senao o do manifesto — #76) e NUNCA o id cru.
// Sem nome, ou com o mesmo nome da pasta ("Netflix" na pasta Netflix), so o
// tipo: "Filmes" / "Séries". O tipo so acompanha quando duas abas teriam o
// mesmo texto.
static void abaRotulos(char rot[][180], int nAbas) {
  static char nomeAba[COL_SOURCE_MAX][180];
  if (nAbas > COL_SOURCE_MAX) nAbas = COL_SOURCE_MAX;
  nomesFontesColecao(nomeAba, nAbas);
  for (int i = 0; i < nAbas; i++) {
    const ColSource *s = &collection->sources[i];
    const char *tipo = i18n((s->prov[0] ? !strcmp(s->midia, "TV") : !strcmp(s->type, "series")) ? "Séries" : "Filmes");
    int repete = 0;
    for (int j = 0; j < nAbas; j++)
      if (j != i && !strcmp(nomeAba[i], nomeAba[j])) repete = 1;
    if (repete) snprintf(rot[i], 180, "%.168s · %.7s", nomeAba[i], tipo);
    else snprintf(rot[i], 180, "%s", nomeAba[i]);
  }
}
// A faixa rola o minimo para a aba alvo entrar inteira, fora da zona de
// esmaecimento. Voltando para as primeiras abas, se a alvo cabe inteira sem
// rolar, a fileira ENCONSTA NO LOGO (rol 0): o respiro fixo de antes escondia
// a primeira aba atras do esmaecimento e a fileira parecia comecar na segunda,
// desalinhada do cabecalho.
static float abaRolRecentrar(float rol,float ini,float fim,float fimMax,float W) {
  if(fim-rol>W-VT_ABA_FADE)rol=fim-(W-VT_ABA_FADE);
  if(ini-rol<VT_ABA_FADE)rol=fim<=W-VT_ABA_FADE?0.0f:fmaxf(0.0f,ini-VT_ABA_FADE);
  if(rol>fimMax)rol=fimMax;
  return rol<0?0:rol;
}
// Opacidade de uma aba perto das bordas da faixa: some antes de chegar no
// botao "Exibicao" (ou na borda esquerda rolada) em vez de ser cortada seco.
// O esmaecimento segue a borda que ENTRA/SAI: pela borda de dentro, uma aba
// pela metade na borda sumia INTEIRA — a primeira aba da Netflix rolava para
// tras do esmaecimento e a fileira parecia comecar na segunda.
static float abaFade(float x,float larg,float x0,float W,int maisEsq,int maisDir) {
  float f=1.0f;
  if(maisDir)f=fminf(f,(x0+W-x)/VT_ABA_FADE);
  if(maisEsq)f=fminf(f,(x+larg-x0)/VT_ABA_FADE);
  return f<0?0:f;
}

// --- ESTILO DAS ABAS: desenho (#205) -----------------------------------------
// Os tres estilos novos usam a mesma altura/linha das pilulas de sempre (Y=253,
// H=52) e as mesmas cores (corFocoFonte + tintas do tema), so mudam o arranjo.
#define VT_AB_Y 253.0f
#define VT_AB_H 52.0f
#define VT_AB_PAD 22.0f
// 0 = baixo, 1 = esquerda, 2 = direita; triangulo cheio feito de tiras.
static void abaSeta(float cx, float cy, int dir, float r, float g, float b, float a) {
  for (int k = 0; k < 5; k++) {
    float t = 10.0f - 2.0f * k;
    if (dir == 0) gfx_cor((GfxRect){cx - t * .5f, cy - 4.0f + 2.0f * k, t, 2}, 0, r, g, b, a);
    else gfx_cor((GfxRect){dir == 2 ? cx - 4.0f + 2.0f * k : cx + 2.0f - 2.0f * k, cy - t * .5f, 2, t}, 0, r, g, b, a);
  }
}
static float abaTexto(const char *s, float maxw) {
  return txt_linha_corta(TXT_HERO_META, s, 176, 176, 176, 255, maxw).w;
}
static void ponteiroCab(int i, int b) {
  (void)b;
#ifdef NV_TOUCH_UI
  toqueLivre = 0;
  toquerol_limpar(&toqueAbas);
#endif
  tabFocus = 1; tabCursor = i;
}
// Pilula de texto centrado: realce na cor do tema quando `sel`, vidro leve no
// resto; o foco (fa) acende por cima como nas abas de sempre.
static void abaPilula(GfxRect p, const char *s, int f, int sel, float fa, float a, float tw) {
  if (sel) {
    float sr, sg, sb; corFocoFonte(&sr, &sg, &sb);
    gfx_cor(p, NV_RAIO_PILL, sr, sg, sb, a * (0.62f + 0.38f * fa));
  } else if (ajustes_vidro()) gfx_vidro_folha(p, NV_RAIO_PILL, a * (1 - fa));
  else gfx_cor(p, NV_RAIO_PILL, 1, 1, 1, .04f * a * (1 - fa));
  if (fa > .001f) focoAbaFonte(p, fa, a);
  int cor = f ? ajustes_tinta_foco() : sel ? ajustes_tinta_foco2() : 176;
  TxtLinha t = txt_linha_corta(TXT_HERO_META, s, cor, cor, cor, 255, tw);
  txt_desenhar_alpha(t, p.x + (p.w - t.w) * .5f, p.y + (p.h - t.h) * .5f, a);
}

// MENU SUSPENSO: [Filmes] [Séries] [catalogo v] e a lista caindo do catalogo.
static void abasMenu(float x0, float W, char rot[][180], float a) {
  const float H = VT_AB_H, PAD = VT_AB_PAD, GAP = 12.0f, Y = VT_AB_Y, SETA = 30.0f;
  int cat = cabN() - 1, tipos = doisTipos();
  float x = x0, catX = x0, catW = 0;
  for (int i = 0; i <= cat; i++) {
    int isCat = i == cat;
    const char *s = isCat ? rot[source] : i18n(i ? "Séries" : "Filmes");
    float tw = fminf(420.0f, W - PAD * 2 - (isCat ? SETA : 0));
    float w = abaTexto(s, tw) + PAD * 2 + (isCat ? SETA : 0);
    GfxRect p = {x, Y, w, H};
    int f = tabFocus && tabCursor == i, sel = !isCat && tipos && abaSerie(source) == i;
    ponteiro_alvo(p.x, p.y, p.w, p.h, ponteiroCab, NULL, i, 0);
    abaPilula(p, s, f, sel, tabAnim[i], a, tw);
    if (isCat) {
      int cor = f ? ajustes_tinta_foco() : 176;
      abaSeta(p.x + p.w - PAD - 5.0f, p.y + H * .5f + ddAnim * -1.0f, 0, cor / 255.0f, cor / 255.0f, cor / 255.0f, a);
      catX = p.x; catW = p.w;
    }
    x += w + GAP;
  }
  if (ddAnim < .01f) return;
  int idx[COL_SOURCE_MAX], n = abasDoTipo(abaSerie(source), idx);
  if (!n) return;
  const float RH = VT_DD_RH, RPAD = 18.0f;
  float pw = catW;
  for (int k = 0; k < n; k++) pw = fmaxf(pw, abaTexto(rot[idx[k]], 420.0f) + RPAD * 2);
  float px = catX, py = Y + H + 8.0f - (1.0f - ddAnim) * 10.0f;
  int vis, base = ddJanela(n, ddCursor, py, &vis);   // limitada, segue o foco
  float ph = vis * RH + 12.0f, aa = a * ddAnim;
  if (px + pw > x0 + W) px = fmaxf(x0, x0 + W - pw);
  gfx_vidro_material((GfxRect){px, py, pw, ph}, 22.0f, GFX_VIDRO_DENS, aa);
  for (int k = base; k < base + vis; k++) {
    GfxRect row = {px + 6.0f, py + 6.0f + (k - base) * RH, pw - 12.0f, RH};
    int f = ddAberto && ddCursor == k, atual = idx[k] == source;
    if (ddAberto) ponteiro_alvo(row.x, row.y, row.w, row.h, ponteiroDd, NULL, k, 0);
    if (f) {
      float sr, sg, sb; corFocoFonte(&sr, &sg, &sb);
      gfx_cor(row, NV_RAIO_PILL, sr, sg, sb, aa * .85f);
    } else if (atual) gfx_cor(row, NV_RAIO_PILL, 1, 1, 1, .07f * aa);
    int cor = f ? ajustes_tinta_foco() : atual ? 245 : 190;
    TxtLinha t = txt_linha_corta(TXT_HERO_META, rot[idx[k]], cor, cor, cor, 255, pw - RPAD * 2);
    txt_desenhar_alpha(t, row.x + RPAD, row.y + (RH - t.h) * .5f, aa);
  }
}

// BARRA CENTRAL: todas as abas numa pilula de vidro no meio; o realce desliza.
// Devolve 0 quando nao cabe (o chamador cai nas pilulas rolaveis).
static int abasBarra(float x0, float W, char rot[][180], int n, float a) {
  const float H = VT_AB_H - 8.0f, PAD = 20.0f, GAP = 4.0f, Y = VT_AB_Y, IN = 4.0f;
  float larg[COL_SOURCE_MAX], total = 0;
  for (int i = 0; i < n; i++) {
    larg[i] = abaTexto(rot[i], 240.0f) + PAD * 2;
    total += larg[i] + (i ? GAP : 0);
  }
  float cw = total + IN * 2;
  if (cw > W) return 0;
  float cx = x0 + (W - cw) * .5f, x = cx + IN;
  int alvo = tabFocus ? tabCursor : source;
  if (alvo >= n) alvo = source;
  float ax = x, aw = larg[0], pos[COL_SOURCE_MAX];
  for (int i = 0; i < n; i++) {
    pos[i] = x;
    if (i == alvo) { ax = x; aw = larg[i]; }
    x += larg[i] + GAP;
  }
  barraAlvoX = ax; barraAlvoW = aw;
  if (barraW < 0.0f) { barraX = ax; barraW = aw; }
  gfx_vidro_painel((GfxRect){cx, Y, cw, VT_AB_H}, VT_AB_H * .5f, .55f, a);
  float fo = 0;
  for (int i = 0; i < n; i++) fo = fmaxf(fo, tabAnim[i]);
  float sr, sg, sb; corFocoFonte(&sr, &sg, &sb);
  GfxRect hl = {barraX, Y + IN, barraW, H};
  gfx_cor(hl, NV_RAIO_PILL, sr, sg, sb, a * (0.62f + 0.38f * fo));
  if (fo > .001f) focoAbaFonte(hl, fo, a);
  for (int i = 0; i < n; i++) {
    GfxRect p = {pos[i], Y + IN, larg[i], H};
    ponteiro_alvo(p.x, p.y, p.w, p.h, ponteiroAba, NULL, i, 0);
    // O realce so esta em cima da aba sob o cursor quando ele chega; ate la a
    // tinta acompanha o realce pela distancia ao centro dele.
    float cobre = 1.0f - fminf(1.0f, fabsf((p.x + p.w * .5f) - (barraX + barraW * .5f)) / (p.w * .5f + barraW * .5f));
    int sobre = cobre > .5f;
    int cor = sobre ? (tabFocus ? ajustes_tinta_foco() : ajustes_tinta_foco2()) : 176;
    TxtLinha t = txt_linha_corta(TXT_HERO_META, rot[i], cor, cor, cor, 255, larg[i] - PAD * 2);
    txt_desenhar_alpha(t, p.x + (p.w - t.w) * .5f, p.y + (H - t.h) * .5f, a);
    // A aberta, quando o cursor esta em outra: um ponto embaixo do nome.
    if (i == source && alvo != source)
      gfx_cor((GfxRect){p.x + p.w * .5f - 2.0f, Y + VT_AB_H - 7.0f, 4, 4}, .5f, sr, sg, sb, a);
  }
  return 1;
}

// PAGINAS: [<] [ nome ] [>] e pontos; o nome desliza como num carrossel.
static void abasPaginas(float x0, float W, char rot[][180], int n, float a) {
  const float H = VT_AB_H, PAD = VT_AB_PAD, GAP = 10.0f, Y = VT_AB_Y;
  float pw = 0;
  for (int i = 0; i < n; i++) pw = fmaxf(pw, abaTexto(rot[i], 420.0f));
  pw = fminf(pw + PAD * 2, fmaxf(120.0f, W - H * 2 - GAP * 2 - n * 20.0f));
  float tw = pw - PAD * 2, x = x0;
  int f = tabFocus && tabCursor < n;
  float sr, sg, sb; corFocoFonte(&sr, &sg, &sb);
  for (int lado = 0; lado < 2; lado++) {
    int alvo = source + (lado ? 1 : -1), ok = alvo >= 0 && alvo < n;
    GfxRect b = {lado ? x0 + H + GAP + pw + GAP : x0, Y, H, H};
    gfx_vidro_painel(b, H * .5f, .55f, a * (ok ? 1.0f : .45f));
    int cor = ok ? 226 : 110;
    abaSeta(b.x + H * .5f, Y + H * .5f, lado ? 2 : 1, cor / 255.0f, cor / 255.0f, cor / 255.0f, a);
    if (ok) ponteiro_alvo(b.x, b.y, b.w, b.h, ponteiroCab, NULL, alvo, 0);
  }
  x = x0 + H + GAP;
  GfxRect p = {x, Y, pw, H};
  float fa = tabAnim[source < n ? source : 0];
  if (f) gfx_cor(p, NV_RAIO_PILL, sr, sg, sb, a * (0.62f + 0.38f * fa));
  else gfx_vidro_painel(p, NV_RAIO_PILL, .55f, a);
  if (fa > .001f) focoAbaFonte(p, fa, a);
  int cor = f ? ajustes_tinta_foco() : 240;
  TxtLinha t = txt_linha_corta(TXT_HERO_META, rot[source], cor, cor, cor, 255, tw);
  gfx_recorte(p.x, p.y, p.w, p.h);
  txt_desenhar_alpha(t, p.x + (p.w - t.w) * .5f + pgSlide * 56.0f, p.y + (H - t.h) * .5f,
                     a * (1.0f - fminf(1.0f, fabsf(pgSlide))));
  gfx_sem_recorte();
  // Pontos: a pagina aberta e a mais larga, na cor do tema.
  float dx = x0 + H + GAP + pw + GAP + H + 18.0f;
  for (int i = 0; i < n; i++) {
    float dw = i == source ? 22.0f : 8.0f;
    if (dx + dw > x0 + W) break;
    GfxRect d = {dx, Y + H * .5f - 4.0f, dw, 8};
    if (i == source) gfx_cor(d, .5f, sr, sg, sb, a);
    else gfx_cor(d, .5f, 1, 1, 1, .28f * a);
    ponteiro_alvo(d.x - 6.0f, Y, dw + 12.0f, H, ponteiroCab, NULL, i, 0);
    dx += dw + 12.0f;
  }
}

// --- CABECALHO, DIRECAO A (dono, 14/10) ---------------------------------------
// Sobretitulo na linha da ilha do relogio, o nome grande embaixo e, na linha
// seguinte, o anel de vistos + contagem a esquerda e o seletor de exibicao a
// direita, colado na borda da ilha. Sem caixa: o vidro fica so no controle.
//
// ANEL: um ponto por titulo, ate VT_ANEL_MAX; acima disso o anel acende a
// fracao e o numero diz o resto. Nada visto = so a ficha com o total (um anel
// apagado lia como barra quebrada). Tudo visto = pontos no acento + check.
#define VT_ANEL_MAX 24
static void anelVistos(float cx, float cy, float a) {
  const float R = 26.0f;
  float ar, ag, ab;
  char num[12];
  int n = vcTotal < VT_ANEL_MAX ? vcTotal : VT_ANEL_MAX, todo = vcVistos >= vcTotal;
  int acesos = vcTotal <= VT_ANEL_MAX ? vcVistos : vcVistos * VT_ANEL_MAX / vcTotal;
  ajustes_acento(&ar, &ag, &ab);
  cabPontos = 0;
  if (!vcVistos) {
    GfxRect f = { cx - R - 4.0f, cy - R - 4.0f, (R + 4.0f) * 2.0f, (R + 4.0f) * 2.0f };
    gfx_cor(f, 0.5f, 1, 1, 1, 0.10f * a);
    gfx_anel(f, 0.5f, 1.5f, 1, 1, 1, 0.16f * a);
    snprintf(num, sizeof num, "%d", vcTotal);
  } else {
    float s = n > 16 ? 5.0f : 7.0f;
    if (!todo) acesos = acesos < 1 ? 1 : acesos >= n ? n - 1 : acesos;
    else gfx_rect((GfxRect){ cx - R * 2.2f, cy - R * 2.2f, R * 4.4f, R * 4.4f }, 0, GFX_SOMBRA, 1, 0, 0,
                  0.5f, ar, ag, ab, 0.22f * a);
    for (int i = 0; i < n; i++) {
      float t = -1.5707963f + 6.2831853f * i / n;
      GfxRect d = { cx + R * cosf(t) - s * 0.5f, cy + R * sinf(t) - s * 0.5f, s, s };
      if (todo) gfx_cor(d, 0.5f, ar, ag, ab, a);
      else gfx_cor(d, 0.5f, 1, 1, 1, (i < acesos ? 0.96f : 0.20f) * a);
    }
    cabPontos = n;
    if (todo) { gfx_icone((GfxRect){ cx - 14.0f, cy - 14.0f, 28, 28 }, "aj_check", ar, ag, ab, a); return; }
    snprintf(num, sizeof num, "%d", vcVistos);
  }
  TxtLinha t = txt_linha(TXT_ILHA_NOME, num, 242, 243, 247, 255);
  txt_desenhar_alpha(t, cx - t.w * 0.5f, cy - t.h * 0.5f, a);
}
// Anel + "N de M assistidos" e, embaixo, a legenda (no acento quando completo).
static void linhaVistos(float x0, float cy, float a) {
  char forte[64], meta[200];
  int todo = vcVistos >= vcTotal;
  anelVistos(x0 + 30.0f, cy, a);
  if (vcVistos) snprintf(forte, sizeof forte, i18n("%d de %d assistidos"), vcVistos, vcTotal);
  else snprintf(forte, sizeof forte, i18n("%d títulos"), vcTotal);
  snprintf(meta, sizeof meta, todo ? "%s  ·  %s" : "%.0s%s", i18n("Completo"), i18n(legendaGrupo()));
  TxtLinha f = txt_linha(TXT_ILHA_NOME, forte, 242, 243, 247, 255);
  TxtLinha m = txt_linha_corta(TXT_DET_META2, meta, 186, 192, 203, 255, 760.0f);
  txt_desenhar_alpha(f, x0 + 84.0f, cy - f.h - 1.0f, a);
  if (todo) { float ar, ag, ab; ajustes_acento(&ar, &ag, &ab);
              txt_desenhar_tom(txt_linha_corta(TXT_DET_META2, meta, 255, 255, 255, 255, 760.0f),
                               x0 + 84.0f, cy + 3.0f, ar, ag, ab, a); }
  else txt_desenhar_alpha(m, x0 + 84.0f, cy + 3.0f, a);
  cabFim = fmaxf(cabFim, fmaxf(cy + 30.0f, cy + 3.0f + m.h));   // anel (R + 4) e legenda
}

static void themeHeader(float a,float x0) {
  TxtLinha eyebrow=vtTelefone()
    ?txt_linha_corta(TXT_HERO_META,rotuloGrupo(),197,202,211,255,vtCabecalhoLargura(x0,960))
    :txt_linha(TXT_HERO_META,rotuloGrupo(),197,202,211,255);
  float ey=vtIlhaY()+(NV_ILHA_H-eyebrow.h)*.5f,fim;
  if(!grupo("Streaming"))txt_desenhar_alpha(eyebrow,x0,ey,a);
  cabEb=(GfxRect){x0,ey,eyebrow.w,eyebrow.h};
  int ehDiretor=collection&&!strcasecmp(collection->group,"Directors");
  float dTopo=vtIlhaY()-NV_ILHA_Y;   // World 22/11: com o menu no topo, o cabecalho desce para baixo dele
  // O wordmark de uma coleção de diretores pode conter cabeça ou lettering
  // composto. No cabeçalho da filmografia, o nome textual e o retrato limpo
  // deixam a identidade legível sem duplicar a mesma informação visual.
  const char *wm=grupo("Streaming")?wordmarkServico():collection?collection->logo:"";
  GLuint logo=!ehDiretor&&collection&&!collection->editorial&&wm[0]
             ?tex_obter_logo_larg(wm,560):0;
  float aspect=logo?tex_aspecto(wm):0;
  if(logo&&aspect>0) {
    // Wordmark oficial, grande o bastante para leitura a distancia. O PNG
    // transparente e importado em ate 800px, portanto 560px nao interpola para
    // cima nem perde a silhueta original da marca.
    float w=vtCabecalhoLargura(x0,560),h=w/aspect,y=(grupo("Streaming")?56:83)+dTopo;if(h>108){h=108;w=h*aspect;}
    // World 22/11: abaixo do menu no topo a marca encolhe para caber antes das abas/legenda.
    { float hm=(nAbas()?VT_AB_Y-NV_CTRL_H*.5f-24.0f:VT_TOPO-80.0f)-y; if(hm<40.0f)hm=40.0f; if(h>hm){h=hm;w=h*aspect;} }
    gfx_rect((GfxRect){x0,y,w,h},logo,tex_marca_escura(wm)?GFX_MARCA:GFX_TEXTO,0,0,0,0,.96f,.97f,.98f,a);
    fim=y+h;
  } else {
    // Nome em 76; o que nao cabe cai para 56 em ate duas linhas (uma so com
    // abas: a fileira delas mora logo abaixo).
    float lw=vtCabecalhoLargura(x0,940);
    if(vtTelefone()||txt_linha(TXT_TITULO1,titulo,242,243,247,255).w<=lw) {
      TxtLinha title=txt_linha_corta(TXT_TITULO1,titulo,242,243,247,255,lw);
      txt_desenhar_alpha(title,x0,80+dTopo,a);
      cabTt=(GfxRect){x0,80+dTopo,title.w,title.h};fim=80+dTopo+title.h;
    } else {
      float h=txt_bloco_corta(TXT_TITULO2,titulo,242,243,247,x0,86+dTopo,lw,62,a,nAbas()||dTopo>0.0f?1:2);
      cabTt=(GfxRect){x0,86+dTopo,lw,h};fim=86+dTopo+h;
    }
  }
  cabCy=fmaxf(218.0f,fim+40.0f);
  if(dTopo>0.0f&&!nAbas())cabCy=fminf(cabCy,VT_TOPO-36.0f);
  if(nAbas())cabCy=fminf(cabCy,VT_AB_Y-NV_CTRL_H*.5f-6.0f);   // acima da fileira das abas
  char caption[180];int n=nItens();
  if(semFonte) {
    snprintf(caption,sizeof caption,"%s",
      mensagemSemFonte(collection&&source<collection->nSources?&collection->sources[source]:NULL));
  }
  else if(desc_vertudo_erro())snprintf(caption,sizeof caption,"Não foi possível carregar. OK para tentar novamente.");
  else if(!n)snprintf(caption,sizeof caption,"%s",desc_vertudo_carregando()?"Carregando títulos…":"Nenhum título nesta lista.");
  else snprintf(caption,sizeof caption,i18n("%d títulos%s  ·  %s"),n,"",i18n(legendaGrupo()));
  // SAGA: quanto da franquia ja foi visto, na mesma linha.
  if(lay==VT_LAY_SAGA&&n>0&&!semFonte&&!desc_vertudo_erro()) {
    char vis[64]; size_t k=strlen(caption);
    snprintf(vis,sizeof vis,i18n("%d de %d assistidos"),sagaVistos,n>VT_MAX?VT_MAX:n);
    snprintf(caption+k,sizeof caption-k,"  ·  %s",vis);
  }
  if(listaDePessoa()&&vcTotal>0&&!semFonte&&!desc_vertudo_erro()&&!vtTelefone()) linhaVistos(x0,cabCy,a);
  else if(!legendaOculta()){TxtLinha sub=txt_linha_corta(TXT_DET_META2,caption,196,202,213,255,vtTelefone()?fminf(960.0f,NV_TELA_W-x0-48.0f):760.0f);txt_desenhar_alpha(sub,x0,cabCy-sub.h*.5f,a);}
  if(collection&&collection->nSources>1) {
    // ABAS NO PADRAO DO APP (19/09/2026, "deixar mais parecido com o restante").
    // Antes: pilula de 304x58 fixa com anel, sublinhado e escala no foco —
    // tres sinais que o resto do app ja abandonou (ver NV_COR_FOCO em
    // layout.h e as abas do painel de Salvos). Agora e a mesma gramatica das
    // abas de Salvos: largura pelo texto, 52 de altura, FOCO = preenchimento
    // na cor de realce graduada, com tinta contrastante e sem anel; ABERTA sem
    // foco conserva uma marca mais baixa da mesma familia; as outras ficam
    // quase transparentes. `tabAnim` cruza o repouso e o foco em vez de
    // escalar.
    // A faixa termina antes da coluna do painel: ali mora o botao "Exibição".
    const float H=52.0f,PAD=22.0f,GAP=12.0f,W=vtFim()-x0-64;
    float margem = vtTelefone() ? 0.0f : 6.0f;
    float textoW = vtTelefone() ? fminf(420.0f, W-PAD*2) : 420.0f;
    float larg[COL_SOURCE_MAX],pos[COL_SOURCE_MAX],px=0;
    static char rot[COL_SOURCE_MAX][180];
    int nAbas=collection->nSources;
    if(nAbas>COL_SOURCE_MAX)nAbas=COL_SOURCE_MAX;
    abaRotulos(rot,nAbas);
    for(int i=0;i<nAbas;i++) {
      // Medida com a cor de repouso; a cor certa e reaplicada no desenho (o
      // cache de linhas guarda as duas).
      larg[i]=txt_linha_corta(TXT_HERO_META,rot[i],176,176,176,255,textoW).w+PAD*2;pos[i]=px;px+=larg[i]+GAP;
    }
    // Estilos novos (#205): desenham o proprio arranjo; a barra que nao cabe na
    // largura cai nas pilulas rolaveis (as de sempre), sem mudar o D-pad.
    int est=estiloAbas();
    // Onde a capsula do foco vai: a aba com o cursor, nos estilos que a usam.
    if(est==VT_AB_CHIPS||est==VT_AB_TOPO) {
      int focoAba=tabFocus&&tabCursor<nAbas?tabCursor:-1;
      if(focoAba>=0){capAlvoX=pos[focoAba];capAlvoW=larg[focoAba];}
    }
    {
      if(est==VT_AB_MENU){abasMenu(x0,W,rot,a);return;}
      if(est==VT_AB_PAGINAS){abasPaginas(x0,W,rot,nAbas,a);return;}
      if(est==VT_AB_BARRA&&abasBarra(x0,W,rot,nAbas,a))return; }
    // ROLAGEM PELO CURSOR, nao por indice: as abas tem larguras diferentes,
    // entao "seis por vez" nao existe mais. A aba com o cursor (ou a aberta,
    // quando o cursor esta na grade) entra inteira; a rolagem so anda o que
    // precisa, e o resto da faixa fica onde estava.
    int alvo=tabFocus && tabCursor>=0 && tabCursor<nAbas ? tabCursor : source;
    if(alvo<0 || alvo>=nAbas)alvo=0;
    float ini=pos[alvo],fim=pos[alvo]+larg[alvo];
#ifdef NV_TOUCH_UI
    if (!toquerol_livre(&toqueAbas))
#endif
      tabRol=abaRolRecentrar(tabRol,ini,fim,fmaxf(0.0f,px-GAP-W),W);
#ifdef NV_TOUCH_UI
    tabRol = anim_clamp(tabRol, 0.0f, fmaxf(0.0f, px-GAP-W));
    toquerol_vincular(&toqueAbas, (GfxRect){x0-margem,244,W+2*margem,H+12}, gfx_escala(),
                     0.0f, fmaxf(0.0f, px-GAP-W), 0, &tabRol);
#endif
    gfx_recorte(x0-margem,244,W+2*margem,H+12);
    // A CAPSULA DO FOCO DESLIZA da aba antiga para a nova (mola, como a barra
    // central): no Topo e um sublinhado na cor do acento; nas pílulas e o
    // preenchimento com a luz de sempre. Sem foco nas abas ela desvanece.
    // VEM ANTES das abas: os rotulos ficam por cima dela — desenhada por
    // ultimo, a capsula tampava o texto da aba focada (Chips, Netflix na TCL).
    if(capA>.01f) {
      // capX e relativo a faixa (pos[]); as abas desenham em x0+pos-tabRol.
      // Sem o +x0 a capsula nasce x0 px a esquerda da aba (sob o logo).
      float cx=x0+capX-tabRol;
      float ca=a*abaFade(cx,capW,x0,W,tabRol>0.5f,px-GAP-tabRol>W+0.5f)*capA;
      if(ca>.005f) {
        if(est==VT_AB_TOPO) {
          float sr,sg,sb;corFocoFonte(&sr,&sg,&sb);
          gfx_cor((GfxRect){cx+PAD,253.0f+H-4.0f,fmaxf(4.0f,capW-PAD*2),3.0f},.5f,sr,sg,sb,ca);
        } else focoAbaFonte((GfxRect){cx,253.0f,capW,H},1.0f,ca);
      }
    }
    for(int i=0;i<nAbas;i++) {
      float x=x0+pos[i]-tabRol;
      if(x+larg[i]<=x0-margem||x>=x0+W+margem)continue;
      int f=tabFocus&&tabCursor==i,selecionada=source==i;
      float fa=tabAnim[i];
      float ah=a*abaFade(x,larg[i],x0,W,tabRol>0.5f,px-GAP-tabRol>W+0.5f);
      GfxRect pill={x,253,larg[i],H};
      if(ah<.005f)continue;
      ponteiro_alvo(fmaxf(pill.x,x0-margem),pill.y,fminf(pill.x+pill.w,x0+W+margem)-fmaxf(pill.x,x0-margem),pill.h,ponteiroAba,NULL,i,0);
      if(est==VT_AB_TOPO) {
        // Foco de botao = pilula cheia no acento, sem anel (layout.h).
        int cor=f?ajustes_tinta_foco():selecionada?255:160;
        if(f)focoAbaFonte(pill,1,ah);
        TxtLinha t=txt_linha_corta(selecionada?TXT_ILHA_SECAO:TXT_HERO_META,rot[i],cor,cor,cor,255,textoW);
        txt_desenhar_alpha(t,x+PAD,pill.y+(H-t.h)*.5f,ah);
        // Uma marca so para a aba aberta: a capsula deslizante ja e o foco,
        // entao ela nao acende quando o cursor esta em cima (igual ao Chips).
        if(selecionada&&!f)gfx_cor((GfxRect){x+PAD,pill.y+H-4,fminf(t.w,32.0f),3},.5f,1,1,1,ah);
        continue;
      }
      if(ajustes_vidro())gfx_vidro_folha(pill,NV_RAIO_PILL,ah*(1-fa));
      else gfx_cor(pill,NV_RAIO_PILL,1,1,1,.04f*ah*(1-fa));
      int cor=f?ajustes_tinta_foco():selecionada?255:176;
      TxtLinha t=txt_linha_corta(TXT_HERO_META,rot[i],cor,cor,cor,255,textoW);
      txt_desenhar_alpha(t,x+PAD,pill.y+(H-t.h)*.5f,ah);
      // A aba aberta, sem o foco em cima: a mesma marca curta do estilo Topo,
      // branca — sobre o degradê da marca a pílula da cor do tema sozinha nao
      // marcava nada (Netflix na TCL, "sem indicador de aba aberta").
      if(selecionada)gfx_cor((GfxRect){x+PAD,pill.y+H-4,fminf(t.w,32.0f),3},.5f,1,1,1,.85f*ah*(1.0f-fa));
    }
    gfx_sem_recorte();
  }
}

// ESTADO DO TITULO NO CARTAO (U4): o check unico do app (badges.c) quando visto,
// senao a barra fina de "parou em". Serve a todas as exibicoes.
static void marcaEstado(GfxRect r, const CatItem *it, float a) {
  if (vistoItem(it)) {
    if (ajustes_selo_visto()) badges_check_cartao(r, 0, a);
  } else if (it->progresso > 0) badges_progresso(r, it->progresso / 100.0f, a);
}

// LINHA DE ESTADO DA LINHA DO TEMPO (U5): "Visto em 12 mar 2024" (ultima vez que
// o app soube: lastWatchedMs do progresso, que Nuvio/Trakt/Simkl alimentam),
// "Visto" sem data, ou "Parou em 47 min". Vazio quando nao ha estado. A
// contagem de reproducoes ("2x") nao existe nos dados do app, entao nao entra.
static void estadoLinha(const CatItem *it, int visto, char *dst, size_t cap) {
  ProgRegistro rec;
  char key[64];
  int tem;
  dst[0] = 0;
  prog_chave(key, sizeof key, it->imdb, 0, 0);
  tem = it->imdb[0] && prog_por_chave(key, &rec);
  if (visto) {
    char dt[48];
    if (tem && rec.lastWatchedMs > 0 &&
        cwo_data_curta(rec.lastWatchedMs, (long long)time(NULL) * 1000, ajustes_idioma(), 0, dt, sizeof dt)) {
      snprintf(dst, cap, i18n("Visto em %s"), dt);
    } else snprintf(dst, cap, "%s", i18n("Visto"));
  } else if (it->progresso > 0 && tem && rec.posSeg >= 60.0) {
    snprintf(dst, cap, i18n("Parou em %d min"), (int)(rec.posSeg / 60.0));
  }
}

static void timelineCard(int i,float cy,float a,float x0) {
  CatItem it;if(!viewItem(i,&it))return;
  int sel=i==foco&&!tabFocus;float r,g,b;corColecao(&r,&g,&b);
  // A linha organiza a cronologia; não é uma borda decorativa de card. O
  // trecho ate o ano em foco fica preenchido (o tempo que ja passou), com
  // tiques entre um ano e o seguinte.
  float rail = vtTimelineRetrato() ? 64.0f : 109.0f, rx = x0 + rail;
  // O trecho de cada cartao vai de meio vao acima a meio vao abaixo: os
  // trechos emendam e o ponto cai no meio. Sem tiques soltos entre os anos.
  float t = i < VT_MAX ? tlFoco[i] : 0.0f, segY = cy - VT_TL_GAP * 0.5f, segH = VT_CARD_H + VT_TL_GAP;
  float fim = tlProg < 0.0f ? segY : tlProg + cy - celY[i];   // o mesmo deslocamento do cartao
  gfx_cor((GfxRect){rx,segY,2,segH},0,.48f,.47f,.46f,a*.45f);
  if(fim>segY)gfx_cor((GfxRect){rx-1,segY,4,fminf(segH,fim-segY)},0,r,g,b,a*.95f);
  int visto=vistoItem(&it);
  float ar,ag,ab;ajustes_acento(&ar,&ag,&ab);
  // Visto: disco no acento com o check; nao visto: ponto da cronologia.
  { float d=16.0f+10.0f*t,pc=cy+VT_TL_PONTO,passou=fim>=pc;
    if(visto)badges_check_disco(rx+1,pc,26.0f,a);
    else{
      if(t>.01f)gfx_cor((GfxRect){rx+1-(d+14)*.5f,pc-(d+14)*.5f,d+14,d+14},.5f,r,g,b,a*.28f*t);
      gfx_cor((GfxRect){rx+1-d*.5f,pc-d*.5f,d,d},.5f,sel?.95f:passou?r:.30f,sel?.95f:passou?g:.30f,
              sel?.97f:passou?b:.32f,a); } }
  // O ano em foco sobe UM degrau da escala (21 -> 28) e acende; os outros
  // ficam apagados. Centrado no ponto, ancorado a direita, rente a linha.
  char year[16];int y=yearOf(&it);if(y==9999)snprintf(year,sizeof year,"—");else snprintf(year,sizeof year,"%d",y);
  if(vtTimelineRetrato()){TxtLinha yr=txt_linha(TXT_DET_META2,year,219,210,195,255);txt_desenhar_alpha(yr,x0,cy+14,a);}
  else{
    TxtLinha yp=txt_linha(TXT_HERO_META,year,219,210,195,255);
    if(t<.99f)txt_desenhar_alpha(yp,rx-22-yp.w,cy+VT_TL_PONTO-yp.h*.5f,a*.55f*(1-t));
    if(t>.01f){TxtLinha yg=txt_linha(TXT_CALLOUT,year,245,240,230,255);
      txt_desenhar_alpha(yg,rx-22-yg.w,cy+VT_TL_PONTO-yg.h*.5f,a*t);}
  }
  // Os cartoes fora do foco recuam um pouco (sem foco na grade, todos iguais).
  if(!tabFocus)a*=.70f+.30f*t;
  GfxRect card=vtTimelineCard(cy), artRect=vtTimelineArte(card);
  float tx=artRect.x+artRect.w+24.0f;
  float tw=fmaxf(1.0f,card.x+card.w-tx-(vtTelefone()?24.0f:32.0f));
  // FOCO DE LINHA (layout.h, NV_COR_FOCO e material da ilha): o cartao e uma
  // linha que da para preencher, entao a superficie sobe um degrau e NENHUM
  // contorno (dono, 14/10: "o contorno no componente que ja nao usamos mais").
  // Raios concentricos: o do cartaz do app na arte, + o recuo no cartao.
  float rp=ajustes_raio_poster_px(),rc=(rp+12.0f)/card.h,ra=rp/artRect.h,k=tabFocus?0.0f:t;
  if(ajustes_vidro()){gfx_vidro_painel(card,rc,.55f,a);gfx_cor(card,rc,1,1,1,.13f*k*a);}
  else gfx_cor(card,rc,.09f+.08f*k,.095f+.081f*k,.105f+.099f*k,a);
  if(visto){gfx_cor(card,rc,ar,ag,ab,.07f*a);
    gfx_cor((GfxRect){card.x,card.y+12,3,card.h-24},.5f,ar,ag,ab,a);}
  // ARTE PREENCHE O QUADRO 16:9 (cover). Sem backdrop: o cartaz desfocado
  // preenche e o cartaz nitido fica no meio — nunca uma moldura cinza.
  const char *art=posterprov_card_formato(it.origem,it.imdb,it.tmdb,it.tipo,it.backdrop[0]?it.backdrop:it.poster,1);GLuint tex=art[0]?tex_obter_larg(art,390):0;
  GLuint fundo=tex&&!it.backdrop[0]?gfx_desfocado(tex,art):tex;
  if(fundo){gfx_card_forcar_cover_atual=1;gfx_tex_aspect_atual=tex_aspecto(art);
    gfx_rect(artRect,fundo,GFX_CARD,0,0,0,ra,0,0,0,a);gfx_card_forcar_cover_atual=0;gfx_tex_aspect_atual=0;}
  else gfx_cor(artRect,ra,NV_COR_ESQUELETO_R,NV_COR_ESQUELETO_G,NV_COR_ESQUELETO_B,a);
  if(tex&&!it.backdrop[0]){float pw=artRect.h*fminf(1.0f,tex_aspecto(art));
    gfx_cor(artRect,ra,0,0,0,.25f*a);
    gfx_tex_aspect_atual=tex_aspecto(art);
    gfx_rect((GfxRect){artRect.x+(artRect.w-pw)*.5f,artRect.y,pw,artRect.h},tex,GFX_CARD,0,0,0,ra,0,0,0,a);
    gfx_tex_aspect_atual=0;}
  else if(!tex){TxtLinha sa=txt_linha(TXT_MINI,"Sem arte",184,188,198,255);
    txt_desenhar_alpha(sa,artRect.x+(artRect.w-sa.w)*.5f,artRect.y+(artRect.h-sa.h)*.5f,a*.9f);}
  marcaEstado(artRect,&it,a);
  TxtLinha name=txt_linha_corta(TXT_CW_TITULO,it.titulo,242,243,247,255,tw);txt_desenhar_alpha(name,tx,cy+22,a);
  TxtLinha genre=txt_linha_corta(TXT_HERO_META,it.genero,187,194,207,255,tw);txt_desenhar_alpha(genre,tx,cy+64,a);
  char est[80];estadoLinha(&it,visto,est,sizeof est);
  if(est[0]){
    float ix=tx;
    if(visto){gfx_icone((GfxRect){tx,cy+102,22,22},"check",ar,ag,ab,a);ix=tx+28.0f;}
    TxtLinha el=txt_linha_corta(TXT_HERO_META,est,visto?ar*255:226,visto?ag*255:230,visto?ab*255:238,255,tw-(ix-tx));
    txt_desenhar_alpha(el,ix,cy+102+(22-el.h)*.5f,a);
  }
  sinopse_bloco_corta(TXT_DET_META2,it.sinopse,209,214,225,tx,cy+(est[0]?138:106),tw,30,a,est[0]?2:3);
}

// --- PECAS COMUNS DAS EXIBICOES ----------------------------------------------
// Raio do cartao em fracao da ALTURA, que e o que o shader mede (ver raioDe em
// home.c); o teto e metade da largura.
static float raioCard(float w, float h) {
  float r = ajustes_raio_poster_px() / h, teto = 0.5f * w / h;
  return r > teto ? teto : r;
}
// A ARTE DE UM CARTAO, com o foco do app (anel ou crescimento, focoprof.h), o
// esqueleto enquanto a arte nao chega e a chegada esvanecendo (revela.h).
// Devolve a caixa desenhada (maior que r0 quando o foco cresce).
static GfxRect cartaoArte(int i, GfxRect r0, const char *arte, float ac, Uint32 agora, int sel) {
  GfxRect r = r0;
  float raio = raioCard(r0.w, r0.h), aArte;
  GLuint t;
  if (sel) {
    if (ajustes_borda_foco()) foco_anel(r0, raio, 1.0f, ac);   // o raio da arte (pela altura)
    else r = foco_zoom(r0, 1.0f);
  }
  t = arte && arte[0] ? tex_obter_larg(arte, r0.w) : 0;
  aArte = i < VT_MAX ? revela_arte(&revArte[i], t != 0, agora) : 1.0f;
  if (!t || aArte < 0.999f)
    gfx_cor(r, raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, ac);
  if (t) {
    gfx_tex_aspect_atual = tex_aspecto(arte);
    gfx_rect(r, t, GFX_CARD, sel ? 1.0f : 0.0f, 0, 0, raio, 0, 0, 0, ac * aArte);
    gfx_tex_aspect_atual = 0.0f;
  }
  return r;
}
static const char *artePoster(const CatItem *it) {
  const char *pp = posterprov_card_addon(it->origem, it->imdb, it->tmdb, it->tipo, it->poster);
  return pp[0] ? pp : it->backdrop;
}

static void fluxoCartaz(int i, float x0, float a, Uint32 agora) {
  CatItem it;
  float sombra;
  GfxRect r = fluxoRect(i, x0, &sombra), c;
  // Mesmo recorte de fluxoDesenhar: descarte antes de pedir arte ou textura.
  if (r.x + r.w <= x0 - 24.0f || r.x >= fluxoFim() + 24.0f ||
      r.y + r.h <= VT_TOPO - 12.0f || r.y >= NV_TELA_H) return;
  if (!viewItem(i, &it)) return;
  ponteiro_alvo_faixa(r.x, r.y, r.w, r.h, VT_TOPO - 12.0f, NV_TELA_H, ponteiroCartaz, NULL, i, 0);
  c = cartaoArte(i, r, artePoster(&it), a, agora, i == foco && !tabFocus);   // o foco da Home
  if (sombra > 0.01f) gfx_cor(c, raioCard(c.w, c.h), 0.0f, 0.0f, 0.0f, a * sombra);
}
static void fluxoDesenhar(int n, float x0, float a, Uint32 agora) {
  float ax = x0, aw = fluxoFim() - x0, cx = fluxoCentro(x0), ly = fluxoLinhaY();
  float r, g, b;
  int c = (int)lroundf(fluxoPos), k;
  int vizinhos = ptv_gpu_fraca_atual() || gfx_efeitos_leves() ? 2 : 6;
  if (n <= 0 || fluxoPos < 0.0f) return;
  corColecao(&r, &g, &b);
  gfx_recorte(ax - 24.0f, VT_TOPO - 12.0f, aw + 48.0f, NV_TELA_H - VT_TOPO + 12.0f);
  // Do mais longe para o foco: o vizinho mais perto cobre o de tras.
  for (k = vizinhos; k >= 1; k--) {
    if (c - k >= 0) fluxoCartaz(c - k, x0, a, agora);
    if (c + k < n) fluxoCartaz(c + k, x0, a, agora);
  }
  if (c >= 0 && c < n) fluxoCartaz(c, x0, a, agora);
  // A linha do tempo: preenchida ate o centro (o tempo que ja passou), um
  // tique e o ano sob cada titulo; o do foco cresce como na vertical.
  gfx_cor((GfxRect){ ax, ly, aw, 2 }, 0, .48f, .47f, .46f, a * .45f);
  if (fluxoPos > 0.0f) gfx_cor((GfxRect){ ax, ly - 1, (cx - ax) * fminf(fluxoPos, 1.0f), 4 }, 0, r, g, b, a * .95f);
  for (k = c - vizinhos; k <= c + vizinhos; k++) {
    CatItem it;
    char ano[16];
    float sombra, t, tx;
    GfxRect q;
    int y;
    if (k < 0 || k >= n || !viewItem(k, &it)) continue;
    q = fluxoRect(k, x0, &sombra); tx = q.x + q.w * 0.5f;
    if (tx < ax || tx > ax + aw) continue;
    t = 1.0f - fminf(fabsf(k - fluxoPos), 1.0f);
    gfx_cor((GfxRect){ tx - 1.0f, ly - 6.0f, 2, 14 }, 0, .62f, .61f, .60f, a * (1.0f - sombra));
    y = yearOf(&it);
    if (y == 9999) snprintf(ano, sizeof ano, "—"); else snprintf(ano, sizeof ano, "%d", y);
    if (t < 0.99f) { TxtLinha yp = txt_linha(TXT_HERO_META, ano, 219, 210, 195, 255);
      txt_desenhar_alpha(yp, tx - yp.w * 0.5f, ly + 18.0f, a * 0.55f * (1.0f - t) * (1.0f - sombra * 0.5f)); }
    if (t > 0.01f) { TxtLinha yg = txt_linha(TXT_TITULO2, ano, 245, 240, 230, 255);
      txt_desenhar_alpha(yg, tx - yg.w * 0.5f, ly + 14.0f + (1.0f - t) * 10.0f, a * t); }
  }
  gfx_cor((GfxRect){ cx - 9.0f, ly - 8.0f, 18, 18 }, .5f, .95f, .95f, .97f, a);
  gfx_sem_recorte();
}

// A ficha do titulo do Cover Flow, ABAIXO da linha do tempo e rente a esquerda:
// logo (ou nome), ano · duracao, generos, IMDb e a sinopse, compacta.
static void fluxoInfo(float a, float x0) {
  CatItem it;
  float y = fluxoInfoY(), x = x0, w = fminf(1000.0f, fluxoFim() - x0);
  int nota;
  GLuint tl;
  if (!viewItem(foco, &it)) return;
  tl = it.logo[0] ? tex_obter_logo_larg(it.logo, 360.0f) : 0;
  if (tl && tex_aspecto(it.logo) > 0.0f) {
    float ap = tex_aspecto(it.logo), hL = 52.0f, wL = hL * ap;
    if (wL > 360.0f) { wL = 360.0f; hL = wL / ap; }
    gfx_rect((GfxRect){ x, y, wL, hL }, tl, tex_marca_escura(it.logo) ? GFX_MARCA : GFX_TEXTO,
             0, 0, 0, 0.0f, 1, 1, 1, a);
    y += hL + 10.0f;
  } else y += txt_bloco_corta(TXT_HEADLINE, it.titulo, 245, 245, 245, x, y, w, 38.0f, a, 1) + 10.0f;
  if (it.meta[0]) {
    TxtLinha t = txt_linha_corta(TXT_DET_META2, it.meta, 240, 241, 244, 255, 260.0f);
    txt_desenhar_alpha(t, x, y + (BADGE_H - t.h) * 0.5f, a);
    x += t.w + 14.0f;
  }
  if (it.classificacao[0]) { badge_desenhar(x, y, it.classificacao, BADGE_NEUTRO, a); x += 70.0f; }
  if (it.genero[0]) {
    TxtLinha g = txt_linha_corta(TXT_CAPTION, it.genero, 196, 205, 219, 255, 420.0f);
    txt_desenhar_alpha(g, x, y + (BADGE_H - g.h) * 0.5f, a * 0.9f);
    x += g.w + 18.0f;
  }
  nota = imdbnota_obter(it.imdb, it.nota, !strcmp(it.tipo, "series"));
  badge_imdb(x, y, nota, 0, a);
  y += BADGE_H + 8.0f;
  if (it.sinopse[0])
    txt_bloco_corta(TXT_DET_META2, it.sinopse, 226, 229, 236, x0, y, w, 30.0f, a * 0.9f, 2);
}
static const char *arteDeitada(const CatItem *it) {
  return posterprov_card_formato(it->origem, it->imdb, it->tmdb, it->tipo,
                                 it->backdrop[0] ? it->backdrop : it->poster, 1);
}

// O CARTAZ DA GRADE (Grade e Decadas): arte 2:3, titulo embaixo, e na Grade de
// um ranking o numero pequeno contornado de sempre.
static void cartazGrade(int i, const CatItem *it, float cx, float cy, float w, float h,
                        float ac, Uint32 agora, int sel) {
  int c = sel ? 255 : 214;
  if (lay == VT_LAY_GRADE) {
    // A GRADE DE SEMPRE, pixel por pixel: o mesmo raio e o mesmo foco de antes.
    GfxRect r0 = { cx, cy, w, h }, r = r0;
    // World 21/11: o raio dos Ajustes, pela altura (convencao do gfx_cor). Era px / VT_CARD_W:
    // o canto saia 1,5x o escolhido no cartaz em pe.
    float raio = raioCard(w, h), aArte;
    const char *arteCard = artePoster(it);
    GLuint t;
    if (sel && !ajustes_borda_foco()) r = foco_zoom(r0, 1.0f);
    if (sel && ajustes_borda_foco()) {
      GfxRect anel = { cx - NV_ANEL_FOCO, cy - NV_ANEL_FOCO,
                       w + 2 * NV_ANEL_FOCO, h + 2 * NV_ANEL_FOCO };
      float ar, ag, ab; ajustes_acento(&ar, &ag, &ab);
      if (ajustes_vidro()) gfx_vidro_cartao(r, raio, 1.0f, ac);
      else gfx_cor(anel, foco_anel_raio(r0, raio), ar, ag, ab, ac);
    }
    t = arteCard[0] ? tex_obter_larg(arteCard, w) : 0;
    aArte = i < VT_MAX ? revela_arte(&revArte[i], t != 0, agora) : 1.0f;
    if (!t || aArte < 0.999f)
      gfx_cor(r, raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, ac);
    if (t) {
      gfx_tex_aspect_atual = tex_aspecto(arteCard);
      gfx_rect(r, t, GFX_CARD, sel ? 1.0f : 0.0f, 0, 0, raio, 0, 0, 0, ac * aArte);
      gfx_tex_aspect_atual = 0.0f;
    }
  } else cartaoArte(i, (GfxRect){ cx, cy, w, h }, artePoster(it), ac, agora, sel);
  marcaEstado((GfxRect){ cx, cy, w, h }, it, ac);
  if (lay != VT_LAY_GRADE || vtTelefone()) { TxtLinha l = txt_linha_corta(TXT_DET_META2, it->titulo, c, c, c, 255, w);
    txt_desenhar_alpha(l, cx, cy + h + 10.0f, ac * (sel ? 1.0f : 0.86f)); }
  if (ranked && lay == VT_LAY_GRADE) {
    char rank[8];snprintf(rank,sizeof rank,"%d",it->ranking > 0 ? it->ranking : i+1);
    TxtLinha edge=txt_linha(TXT_RANK,rank,234,236,241,255),ink=txt_linha(TXT_RANK,rank,17,18,22,255);
    float x=cx-10,y=cy+h-edge.h;
    for(int dx=-2;dx<=2;dx+=2)for(int dy=-2;dy<=2;dy+=2)txt_desenhar_alpha(edge,x+dx,y+dy,ac);
    txt_desenhar_alpha(ink,x,y,ac);
  }
}

// RANKING: numeral grande no vao a esquerda, o cartaz por cima da ponta dele
// (o Top 10 da Dinamica, em pagina inteira). Os tres primeiros no acento.
static void rankingCard(int i, const CatItem *it, float cx, float cy, float w, float h,
                        float ac, Uint32 agora, int sel) {
  char num[8];
  TxtLinha nu;
  snprintf(num, sizeof num, "%d", it->ranking > 0 ? it->ranking : i + 1);
  nu = txt_linha(TXT_RANK_GRANDE, num, 255, 255, 255, 255);
  if (nu.tex && nu.w > 0) {
    // Cabe no vao (100 px) mais o pedaco que o cartaz cobre; "100" encolhe.
    float vao = 100.0f + 0.22f * w, e = vao / (float)nu.w, nw, nh;
    float ar, ag, ab;
    if (e > h * 0.86f / (float)nu.h) e = h * 0.86f / (float)nu.h;
    nw = nu.w * e; nh = nu.h * e;
    if (i < 3) { ajustes_acento(&ar, &ag, &ab); }
    else { ar = 0.91f; ag = 0.92f; ab = 0.94f; }
    gfx_rect((GfxRect){ cx + 0.22f * w - nw, cy + h - nh * NV_TOP10_NUM_BASE, nw, nh },
             nu.tex, GFX_TEXTO, 0, 0, 0, 0.0f, ar, ag, ab, ac * (i < 3 ? 0.95f : 0.62f));
  }
  cartaoArte(i, (GfxRect){ cx, cy, w, h }, artePoster(it), ac, agora, sel);
  marcaEstado((GfxRect){ cx, cy, w, h }, it, ac);
  { int c = sel ? 255 : 214;
    TxtLinha l = txt_linha_corta(TXT_DET_META2, it->titulo, c, c, c, 255, w);
    txt_desenhar_alpha(l, cx, cy + h + 10.0f, ac * (sel ? 1.0f : 0.86f)); }
}

// SAGA: o capitulo deitado com o numero dele e, acima da linha, a TRILHA: um
// no por capitulo, cheio no acento quando ja foi visto, e o trecho entre dois
// capitulos vistos aceso. E o "onde eu parei" da franquia sem abrir nada.
static void sagaCard(int i, const CatItem *it, float cx, float cy, float w, float h,
                     float ac, Uint32 agora, int sel, int n) {
  float ar, ag, ab, ty = cy - VT_SAGA_TRILHA * 0.5f - 2.0f;
  int visto = celVisto[i];
  char cap[48];
  ajustes_acento(&ar, &ag, &ab);
  // Trecho ate o proximo capitulo da MESMA linha.
  if (i + 1 < n && celLin[i + 1] == celLin[i]) {
    int aceso = visto && celVisto[i + 1];
    float x1 = cx + w * 0.5f, x2 = x0Atual + celX[i + 1] + celW[i + 1] * 0.5f;
    if (aceso) gfx_cor((GfxRect){ x1, ty - 2.0f, x2 - x1, 4.0f }, 0.5f, ar, ag, ab, ac * 0.9f);
    else gfx_cor((GfxRect){ x1, ty - 1.0f, x2 - x1, 2.0f }, 0.5f, 1, 1, 1, ac * 0.20f);
  }
  { float d = sel ? 22.0f : 16.0f;
    GfxRect no = { cx + w * 0.5f - d * 0.5f, ty - d * 0.5f, d, d };
    if (visto) gfx_cor(no, 0.5f, ar, ag, ab, ac);
    else { gfx_cor(no, 0.5f, 0.12f, 0.13f, 0.15f, ac);
           gfx_rect(no, 0, GFX_ANEL, 0, 2.0f / d, 0, 0.5f, 1, 1, 1, ac * (sel ? 0.95f : 0.45f)); } }
  { GfxRect r = cartaoArte(i, (GfxRect){ cx, cy, w, h }, arteDeitada(it), ac, agora, sel);
    char nn[8];
    TxtLinha l;
    float raio = raioCard(r.w, r.h);
    gfx_veu_base(r, raio, 0.5f, 0.75f * ac);
    snprintf(nn, sizeof nn, "%02d", i + 1);
    l = txt_linha(TXT_TITULO2, nn, 246, 247, 250, 255);
    txt_desenhar_alpha(l, r.x + 20.0f, r.y + r.h - l.h - 14.0f, ac * 0.96f);
    if (visto) badges_check_cartao(r, 0, ac);
    else if (it->progresso > 0) badges_progresso(r, it->progresso / 100.0f, ac); }
  snprintf(cap, sizeof cap, i18n("Capítulo %d"), i + 1);
  { TxtLinha e = txt_linha(TXT_MINI, cap, 170, 176, 188, 255);
    int c = sel ? 255 : 226;
    TxtLinha t = txt_linha_corta(TXT_CW_TITULO, it->titulo, c, c, c, 255, w);
    char ano[16]; int y = yearOf(it);
    txt_desenhar_alpha(e, cx, cy + h + 12.0f, ac * 0.9f);
    txt_desenhar_alpha(t, cx, cy + h + 12.0f + e.h + 4.0f, ac);
    if (y != 9999) {
      TxtLinha a; snprintf(ano, sizeof ano, "%d", y);
      a = txt_linha(TXT_MINI, ano, 170, 176, 188, 255);
      txt_desenhar_alpha(a, cx + w - a.w, cy + h + 12.0f, ac * 0.9f);
    } }
}

// MURAL: ladrilho deitado com o nome por dentro, sobre o veu da base. No
// grande o logo do titulo (quando ha) e a linha de meta.
static void muralTile(int i, const CatItem *it, float cx, float cy, float w, float h,
                      float ac, Uint32 agora, int sel) {
  GfxRect r = cartaoArte(i, (GfxRect){ cx, cy, w, h }, arteDeitada(it), ac, agora, sel);
  int grande = i % 5 == ((i / 5) & 1 ? 2 : 0);
  float pad = grande ? 28.0f : 18.0f, yb = r.y + r.h - pad;
  gfx_veu_base(r, raioCard(r.w, r.h), grande ? 0.55f : 0.62f, 0.85f * ac);
  int nome = ajustes_mural_titulos();   // #521
  if (grande && it->meta[0]) {
    TxtLinha m = txt_linha_corta(TXT_HERO_META, it->meta, 222, 226, 234, 255, r.w - 2 * pad);
    yb -= m.h; txt_desenhar_alpha(m, r.x + pad, yb, ac * 0.92f); yb -= 8.0f;
  }
  if (nome) { GLuint tl = grande && it->logo[0] ? tex_obter_logo_larg(it->logo, 320.0f) : 0;
    float ap = tl ? tex_aspecto(it->logo) : 0.0f;
    if (tl && ap > 0.0f) {
      float wl = vtTelefone() ? fminf(320.0f, r.w - 2 * pad) : 320.0f, hl = wl / ap;
      if (hl > 96.0f) { hl = 96.0f; wl = hl * ap; }
      gfx_rect((GfxRect){ r.x + pad, yb - hl, wl, hl }, tl,
               tex_marca_escura(it->logo) ? GFX_MARCA : GFX_TEXTO, 0, 0, 0, 0, 1, 1, 1, ac);
    } else {
      int c = sel ? 255 : 236;
      TxtLinha t = txt_linha_corta(grande ? TXT_TITULO2 : TXT_DET_META2, it->titulo, c, c, c, 255,
                                   r.w - 2 * pad);
      txt_desenhar_alpha(t, r.x + pad, yb - t.h, ac);
    } }
}

// DECADAS: o cabecalho da secao. Numeral grande, a contagem e um fio ate o fim
// da grade. `fixo` = a copia presa no topo enquanto a secao rola por baixo.
static void cabecalhoDecada(int k, float x0, float y, float a) {
  char ano[24], qtd[48];
  float ar, ag, ab;
  TxtLinha t, q;
  if (decAno[k]) snprintf(ano, sizeof ano, i18n("Anos %d"), decAno[k]);
  else snprintf(ano, sizeof ano, "%s", i18n("Sem data"));
  snprintf(qtd, sizeof qtd, i18n(decQtd[k] == 1 ? "%d título" : "%d títulos"), decQtd[k]);
  ajustes_acento(&ar, &ag, &ab);
  t = txt_linha(TXT_TITULO2, ano, 244, 245, 248, 255);
  q = txt_linha(TXT_HERO_META, qtd, 176, 182, 194, 255);
  gfx_cor((GfxRect){ x0, y + 16.0f, 6.0f, t.h - 14.0f }, 3.0f / (t.h - 14.0f), ar, ag, ab, a);
  txt_desenhar_alpha(t, x0 + 22.0f, y + 8.0f, a);
  txt_desenhar_alpha(q, x0 + 22.0f + t.w + 20.0f, y + 8.0f + (t.h - q.h) * 0.6f, a * 0.9f);
  gfx_cor((GfxRect){ x0 + 22.0f + t.w + q.w + 44.0f, y + 8.0f + t.h * 0.55f,
                     VT_CW - (22.0f + t.w + q.w + 44.0f), 1.0f }, 0, 1, 1, 1, a * 0.16f);
}

// O SELETOR DE EXIBICAO (direcao A, 14/10): segmentado de vidro com um icone
// por exibicao, na ordem do ciclo; a aberta mostra o nome. Fica na linha do
// anel (`cy`), terminando na borda direita da ilha do relogio. FOCO = a pilula
// cheia da barra de cima; a dica "OK troca" so aparece entao, a esquerda.
// UM CONTROLE SO (dono, 14/10: "esse botao de style tab ta solto"): o estilo
// das abas, quando ha abas, e o ultimo segmento do MESMO trilho.
#ifdef NV_TOUCH_UI
// Telefone: tocar no seletor troca para a proxima exibicao (o trilho aberto
// nao e desenhado la).
static void toqueExibicao(int a, int b) {
  (void)a; (void)b;
  if (!vtTelefone() || !collection || !aberta) return;
  toqueLivre = 0; toquerol_limpar(&toqueAbas);
  tabFocus = 1; tabCursor = cabN();
  alternarExibicao();
}
#endif
static void botaoExibicao(float a, float cy) {
  if (chipOculto()) return;
  const float H = NV_CTRL_H, S = 52.0f, P = 4.0f;
  float f = pillAnim, xd = NV_TELA_W - (vtTelefone() ? 48.0f : NV_ILHA_MARGEM_D);
  int est = nAbas() > 0, fe = tabFocus && tabCursor == cabN() + 1, nl = (int)sizeof LAY_CICLO;
  // RECOLHIDO (dono, 15/10): so a exibicao aberta, ate o OK abrir o trilho.
  int aberto = exibAberta && tabFocus && tabCursor == cabN() && !vtTelefone(), so = !aberto;
  int tinta = gfx_vidro_tinta(f), tn = gfx_vidro_tinta(aberto && LAY_CICLO[exibCursor] != lay ? 0.0f : f);
  TxtLinha nome = txt_linha_corta(TXT_AJ_SEG, LAY_NOME[lay], tn, tn, tn, 255, 260.0f);
  float selW = 50.0f + nome.w + 20.0f, w = P * 2.0f + S * ((so ? 0 : nl - 1) + est) + selW;
  float x = xd - w, sx = x + P;
  GfxRect trilho = { x, cy - H * 0.5f, w, H };
  cabSw = trilho; cabFim = fmaxf(cabFim, trilho.y + trilho.h);
#ifdef NV_TOUCH_UI
  if (vtTelefone()) {
    // The drawing stays unchanged; reserve a 48dp touch target in the header.
    float minimo = 144.0f / vtEscala();
    float hitW = fmaxf(trilho.w, minimo), hitH = fmaxf(trilho.h, minimo);
    float hitX = fminf(trilho.x, NV_TELA_W - 48.0f - hitW);
    float hitY = fmaxf(0.0f, trilho.y + (trilho.h - hitH) * .5f);
    if (lay != VT_LAY_VITRINE) hitY = fminf(hitY, VT_TOPO - 12.0f - hitH);
    ponteiro_alvo(hitX, hitY, hitW, hitH, NULL, toqueExibicao, 0, 0);
  }
#endif
  // O material da ilha do relogio (ilha.c): vidro, ou o plano com aro quando o
  // vidro esta desligado (efeitos minimos, TVs antigas).
  if (ajustes_vidro()) gfx_vidro_painel(trilho, NV_RAIO_PILL, 0.62f, a);
  else { gfx_cor(trilho, NV_RAIO_PILL, 0.12f, 0.124f, 0.143f, 0.92f * a);
         gfx_anel(trilho, NV_RAIO_PILL, 1.5f, 1, 1, 1, 0.085f * a); }
  for (int k = 0; k < nl; k++) {
    int sel = LAY_CICLO[k] == lay, cur = aberto ? k == exibCursor : sel;
    if (so && !sel) continue;
    GfxRect s = { sx, trilho.y + P, sel ? selW : S, H - P * 2.0f };
    float ic = cur && f > 0.5f ? tinta / 255.0f : 1.0f;
    if (sel) gfx_cor(s, NV_RAIO_PILL, 1, 1, 1, 0.14f * (1.0f - (cur ? f : 0.0f)) * a);
    if (cur) gfx_vidro_pilula_cheia(s, NV_RAIO_PILL, f, a);
    if (sel) txt_desenhar_alpha(nome, s.x + 50.0f, s.y + (s.h - nome.h) * 0.5f, a);
    gfx_icone((GfxRect){ s.x + 15.0f, s.y + (s.h - 22.0f) * 0.5f, 22, 22 }, LAY_ICONE[LAY_CICLO[k]],
              ic, ic, ic, (sel || cur ? 1.0f : 0.58f) * a);
    sx += s.w;
  }
  if (est) {   // o estilo das abas: o ultimo segmento do mesmo trilho
    GfxRect seg = { sx, trilho.y + P, S, H - P * 2.0f };
    float c = fe ? gfx_vidro_tinta(1.0f) / 255.0f : 1.0f;
    if (fe) gfx_vidro_pilula_cheia(seg, NV_RAIO_PILL, 1.0f, a);
    else gfx_cor((GfxRect){ seg.x, seg.y + 10.0f, 1.0f, seg.h - 20.0f }, 0, 1, 1, 1, 0.14f * a);
    gfx_icone((GfxRect){ seg.x + 15.0f, seg.y + (seg.h - 22.0f) * 0.5f, 22, 22 }, "aj_layout-list", c, c, c,
              (fe ? 1.0f : 0.58f) * a);
    ponteiro_alvo(seg.x, seg.y, seg.w, seg.h, ponteiroCab, NULL, cabN() + 1, 0);
  }
  if (f > 0.05f) {   // a dica so no foco, na mesma linha: a grade fica livre
    TxtLinha d = txt_linha(TXT_ILHA_SUB, "OK troca a exibição", 225, 228, 235, 255);
    txt_desenhar_alpha(d, x - 20.0f - d.w, cy - d.h * 0.5f, 0.72f * f * a);
  }
}

static void botaoMaisVistos(float a, float x0, float y, int linha) {
  if (!temMaisVistos()) return;
  float f = !tabFocus && (lay != VT_LAY_VITRINE || (vitFR < vitN && vitLin[vitFR].mais)) ? 1.0f : 0.0f, h = 60.0f;
  int cor = f > 0.5f ? ajustes_tinta_foco() : 214;
  float livre = fmaxf(1.0f, vtFim() - x0 - 44.0f);
  TxtLinha t = txt_linha_corta(TXT_HERO_META, i18n("Carregar mais"), cor, cor, cor, 255, livre);
  GfxRect botao = { x0, y, t.w + 44.0f, h };
  gfx_cor(botao, NV_RAIO_PILL, 1, 1, 1, 0.07f * a);
  focoAbaFonte(botao, f, a);
  txt_desenhar_alpha(t, botao.x + 22.0f, botao.y + (h - t.h) * 0.5f, a);
  ponteiro_alvo(botao.x, botao.y, botao.w, botao.h,
                ponteiroMaisVistos, carregarMaisVistos, linha, 0);
}

static void vooDesenhar(void);
static void vitDesenhar(float a, float x0) {
  vitProjetar();
  float w, h, passo, rot, pas = vitPasso(), topo = vitTopo();
  float viewportBottom = topo - 8.0f + vitAlturaViewport();
  CatItem *hero = vitChave(vitHeroAtual), *ant = vitChave(vitHeroAnt);
  // Camadas do destaque, de baixo para cima (2.0.38): cada uma com o progresso proprio.
  CatItem *camIt[3]; float camP[3]; int camN = 0;
  { CatItem *c2 = vitChave(vitHeroAnt2);
    if (c2 && vitHeroEntraAnt2 < 1.0f + 1e-6f && ant) { camIt[camN] = c2; camP[camN++] = vitHeroEntraAnt2; }
    if (ant) { camIt[camN] = ant; camP[camN++] = vitHeroEntraAnt; }
    if (hero) { camIt[camN] = hero; camP[camN++] = vitHeroEntra; } }
  int arte = 0;
  home_vitrine_medida(&w, &h, &passo, &rot);
#ifdef NV_TOUCH_UI
  if (vtTelefone()) {
    vitPonteiroPronto = 0;
    memset(vitPonteiroVisivel, 0, sizeof vitPonteiroVisivel);
    memset(vitPonteiroTudoVisivel, 0, sizeof vitPonteiroTudoVisivel);
    for (int r = 0; r < VIT_LIN_MAX; r++) vitToqueX[r].offset = NULL;
    toquerol_vincular(&vitToqueY, (GfxRect){x0, topo - 8.0f,
      NV_TELA_W - x0, fmaxf(0.0f, viewportBottom - (topo - 8.0f))}, vtEscala(), 0.0f, vitToqueMaxY(), 1, &vitScroll);
    ponteiro_rolagem(toqueVitrineRolar);
  }
#endif
  // A tela inteira ja e pintada pela arte do destaque (e o clear do quadro): so
  // a abertura/saida, com a Home por baixo, e as transicoes (destaque entrando,
  // troca de Collection) precisam do fundo escuro opaco por baixo.
  float sai = vitTrocaPend ? fminf(1.0f, vitTrocaSai) : 0.0f, base = a;
  sai = sai * sai * (3.0f - 2.0f * sai);
  // O fundo escuro so quando nada opaco cobre a tela: abrindo/saindo, trocando
  // de Collection, ou o PRIMEIRO destaque entrando. Na troca entre dois
  // destaques a arte que sai ja cobre tudo (2.0.34: era uma tela cheia a mais
  // em toda troca).
  int fundoPintado = a < 0.99f || sai > 0.0f || !hero || (camN > 0 && camP[0] < 1.0f);
  // 2.0.45: na transformacao o fundo escuro so existe onde o destaque nao cobre (Padrao) e acende com o
  // relogio; com o banner cobrindo a tela (Moderna/Dinamica) nada e pintado por baixo dele.
  const int trVivo = vooTem && !vooSaindo;
  if (trVivo) { fundoPintado = !vooDestCobre; base = vooDestCobre ? tr_fundo(vitTr.p) : tr_fundo_padrao(vitTr.r); }
  else if (vitBannerAtivo && vooDestCobre && a >= 0.99f && sai <= 0.0f) fundoPintado = 0;
  if (fundoPintado && base > 0.004f) {
    GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    gfx_cor(tela, 0.0f, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, base); }
  a *= 1.0f - sai;   // daqui para baixo tudo e conteudo: sai junto na troca
  // D1 (2.0.45): o BANNER DA PASTA e a primeira camada de destaque (o mesmo do hero da Home e do cartao):
  // aparece quando o cartao chega ao destino e assenta de 1,02 para 1,00; o titulo entra depois por cima.
  if (vitBannerAtivo && collection) {
    float ab = trVivo ? (vitTr.p >= TR_P_SAIDA && tr_cartao_alfa(vitTr.r) < 0.999f ? 1.0f : 0.0f) : 1.0f;
    if (home_vitrine_arte_banner(col_banner(collection), a * ab, vooTem ? tr_escala(vitTr.p) : 1.0f) && ab > 0.0f) arte = 1;
  }
  int dTroca = 0, dAnt = 0, dHero = 0;   // DIAGNOSTICO 2.0.35: codigos de home_vitrine_diag_arte
  if (hero) {
    // Composicao "por cima" camada a camada: mix(mix(c0, c1, e1), c2, e2). A
    // de baixo vale e(p) (1 quando ja entrou); duas primeiras numa passada so.
    float al[3];
    for (int i = 0; i < camN; i++) { float p = fminf(1.0f, camP[i]); al[i] = a * p * p * (3.0f - 2.0f * p); }
    int trocou = 0, i0 = 0;
    if (camN >= 2) {
      trocou = home_vitrine_arte_troca(camIt[0], al[0], camIt[1], al[1]);
      dTroca = home_vitrine_diag_arte();
      if (trocou) { arte = 1; i0 = 2; }
    }
    for (int i = i0; i < camN; i++) {
      arte |= home_vitrine_arte(camIt[i], al[i]);
      if (i == camN - 1) dHero = home_vitrine_diag_arte(); else dAnt = home_vitrine_diag_arte();
    }
  }
  // 2.0.36: com um titulo sob o foco e a arte do destaque ainda a caminho, o
  // fundo e o ESCURO do app, nunca o banner borrado da Collection (opaco, da cor
  // da marca: o "bloco vermelho" de ate 951 ms medido na TV). O banner fica so
  // para a Collection sem titulo nenhum.
  // 2.0.45: na transformacao quem cobre e o cartao/banner (e, no Padrao, o fundo que acende com o relogio):
  // nada de pintar a tela de escuro por cima da Home so porque o titulo ainda nao tem arte.
  int semArte = !arte && !trVivo;
  int fundoUsado = 0;
  if (semArte) {
    if (hero || vitItem(vitFR, vitFC)) {
      if (!fundoPintado) {
        GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
        gfx_cor(tela, 0.0f, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, a); }
    } else { fundoUsado = 1; fundoColecao(a); }
  }
  // 2.0.37: AO SAIR DA COLLECTION OS TEXTOS NAO SE SOBREPOEM. A saida apaga a
  // Vitrine inteira (anim 1 -> 0) por cima da Home, que ja mostra o proprio
  // cabecalho ("By Year"); o titulo da Collection, as fileiras e o destaque
  // apagavam no mesmo ritmo e os dois cabecalhos coexistiam (video 2.0.34). A
  // arte e o fundo seguem a saida normal; o CONTEUDO (textos, fileiras, cartazes)
  // some na primeira metade dela, antes de o cabecalho da Home aparecer.
  if (!aberta) {
    float f = fmaxf(0.0f, fminf(1.0f, (anim - 0.55f) / 0.45f));
    a *= f * f * (3.0f - 2.0f * f);
  }
  { // DIAGNOSTICO 2.0.35 (so log): o bloco vermelho de ~4 quadros na troca de destaque.
    // Registra os quadros em que o destaque existe mas a arte caiu no fundo da
    // Collection (arte==0), a troca em camadas foi recusada, a textura faltou
    // ou o destaque e o proprio poster. Uma linha a cada MUDANCA de situacao
    // e uma de fechamento com a duracao; no maximo 120 linhas por execucao.
    static unsigned long ultSig; static Uint32 desde; static unsigned quadros, linhas; static int ruimAntes;
    int ruim = hero && (semArte || fundoUsado || dTroca == 9 || dTroca == 10 || dAnt == 3 || dAnt == 4 ||
                        dHero == 3 || dHero == 4 || dHero == 6 || dAnt == 6);
    int pAnt = 0, pHero = 0;
    unsigned long hAnt = ant ? home_vitrine_diag_arte_info(ant, &pAnt) : 0;
    unsigned long hHero = hero ? home_vitrine_diag_arte_info(hero, &pHero) : 0;
    unsigned long sig = (unsigned long)((fundoUsado + 2 * semArte) * 1u) ^ ((unsigned long)dTroca << 3) ^ ((unsigned long)dAnt << 8) ^
                        ((unsigned long)dHero << 13) ^ (hAnt * 2654435761UL) ^ (hHero * 40503UL);
    if (ruim) {
      quadros++;
      if ((!ruimAntes || sig != ultSig) && linhas < 120) {
        long usadosKB = 0, limiteKB = 0;
        tex_diag_limites(&usadosKB, &limiteKB);
        printf("[diag-hero] ruim fundoColecao=%d semArte=%d troca=%d ant=%d hero=%d entra=%.2f a=%.2f sai=%.2f "
               "antHash=%08lx poster=%d heroHash=%08lx poster=%d tex=%ld/%ld KB\n",
               fundoUsado, semArte, dTroca, dAnt, dHero, vitHeroEntra, a, sai, hAnt, pAnt, hHero, pHero,
               usadosKB, limiteKB);
        fflush(stdout); linhas++;
      }
      if (!ruimAntes) { desde = SDL_GetTicks(); quadros = 1; }
    } else if (ruimAntes && linhas < 120) {
      printf("[diag-hero] normal de volta apos %u quadro(s) em %u ms\n", quadros, (unsigned)(SDL_GetTicks() - desde));
      fflush(stdout); linhas++;
    }
    ruimAntes = ruim; ultSig = sig;
  }
  degradeMarca(a, home_vitrine_layout() == HOME_LAYOUT_DINAMICA ? NV_TELA_H : NV_SHELF_TOP);
  // 2.0.45: o cartao da transformacao, por cima das artes e por baixo de todo texto (cabecalho e fileiras se
  // formam POR CIMA dele enquanto ele cede o lugar ao destaque).
  if (vooTem) { vooDesenhar(); vooDesenhado = 1; }
  // O bloco do destaque comeca abaixo do cabecalho (anel e legenda descem ate
  // ~34 px do centro da linha), com folga: logo largo, alto ou nome em texto.
  cabCy = vtIlhaY() + NV_ILHA_H + 14.0f + NV_CTRL_H * 0.5f;
  // O TEXTO DO DESTAQUE TROCA JUNTO COM A ARTE, SEM SALTO: o antigo sai subindo
  // um pouco e apagando na primeira metade; o novo entra de baixo para o lugar,
  // acendendo. So posicao e opacidade (o barato para a TV).
  if (hero) {
    float teto = cabCy + 34.0f + VIT_CAB_FOLGA;
    // Cada camada: entra com ent(p) subindo de baixo e sai quando a de cima
    // entra (sobe 14 px e apaga). Continuo na interrupcao: a camada que ja
    // estava a meio caminho segue de onde estava, sem voltar a 100%.
    for (int i = 0; i < camN; i++) {
      float p = fminf(1.0f, camP[i]);
      // Primeiro destaque (abrir ou trocar de Collection, sem camada abaixo): so entra, sem o atraso
      // (os elementos ainda escalonam: logo -> meta -> sinopse, herotexto.h; 2.0.42).
      float entEl[HT_N];
      for (int k = 0; k < HT_N; k++) entEl[k] = ht_vit_entra_de(k, p, camN == 1 ? 0.0f : HT_VIT_INI);
      float sai = 0.0f;
      if (i + 1 < camN) {
        float q = fminf(1.0f, fminf(1.0f, camP[i + 1]) / 0.45f);
        sai = q * q * (3.0f - 2.0f * q);
      }
      if (sai >= 1.0f) continue;
      // Piso so na camada de cima: ela rasteriza as linhas ja no quadro da troca (sem textura nova
      // no meio da animacao), como antes. O alfa e por elemento; a posicao Y tambem.
      float pisoA = i == camN - 1 ? 0.005f : 0.0f, fa = a * (1.0f - sai);
      float al0 = fa * fmaxf(entEl[0], pisoA), al1 = fa * fmaxf(entEl[1], pisoA), al2 = fa * fmaxf(entEl[2], pisoA);
      float ty = topo - VIT_TITULO_SOBE - 14.0f * sai;
      float topoBloco = home_vitrine_texto_el(camIt[i], ty, teto, al0, al1, al2,
                                              18.0f * (1.0f - entEl[0]), 18.0f * (1.0f - entEl[1]),
                                              18.0f * (1.0f - entEl[2]));
      if (i == camN - 1) vitHeroTopo = topoBloco;
    }
  }

  // As fileiras, recortadas abaixo do texto do destaque.
  gfx_recorte(0.0f, topo - 8.0f, NV_TELA_W, vitAlturaViewport());
  for (int r = 0; r < vitN; r++) {
    // TV: a fileira entra subindo 28 px e acendendo quando os cartazes chegam;
    // a saida da troca de Collection desce 20 px.
    float ent = vtTelefone() || !vitLin[r].n ? 1.0f : fmaxf(0.0f, fminf(1.0f, vitLinEntra[r]));
    // World 20/11: na transformacao (e depois dela, ate a fileira entrar) cada fileira acende quando a dela esta pronta.
    float ee = (vooTem || (vitRevelaEm && r < VIT_LIN_ESPERA_MAX)) ? (vooSaindo ? 1.0f : vitLinEntradaTr(r, vitAgora, w, passo))
             : vitEntradaT >= HT_ENT_TOTAL_MS ? 1.0f : ht_ent_lin(r, vitEntradaT);   // 2.0.43: entrada da Collection
    float y = topo + r * pas - vitScroll + 28.0f * (1.0f - ent) + 20.0f * sai + HT_ENT_LIN_PX * (1.0f - ee), cy = y + VIT_TITULO_H;
    float ac = a * ee * (vtTelefone() ? (r == vitFR ? 1.0f : 0.78f) : fmaxf(0.0f, fminf(1.0f, vitLinAlfa[r])) * ent);
    if (vooTem && r < 3 && ac < 0.006f && a > 0.01f && vitLin[r].n) ac = 0.006f;   // 2.0.44: rasteriza o texto e pede os cartazes antes de aparecer
    float xr = vtTelefone() ? x0 : vitAncora(w, passo);   // inicio da fileira (TV: a ancora)
    VitLin *l = &vitLin[r];
    TxtLinha tt;
    if (y > viewportBottom) break;
    if (y + pas < topo - 8.0f) continue;
    tt = txt_linha_corta(TXT_ROW_TITULO, l->nome[0] ? l->nome : "—", 245, 246, 249, 255, 900);
    txt_desenhar_alpha(tt, xr, y - VIT_TITULO_SOBE, ac);
    if (l->n > 0) {
#ifdef NV_TOUCH_UI
      if (vtTelefone()) {
        float top = fmaxf(cy, NV_SHELF_TOP - 8.0f), bottom = fminf(cy + h + rot, NV_TELA_H);
        toquerol_vincular(&vitToqueX[r], (GfxRect){x0, top, NV_TELA_W - x0, fmaxf(0.0f, bottom - top)},
                         vtEscala(), 0.0f, vitToqueMaxX(r), 0, &vitSx[r]);
      }
#endif
      // TV: a fileira ocupa a tela toda e as duas pontas esvanecem por pixel na
      // largura da espiada (meio cartaz), igual dos dois lados.
      if (!vtTelefone()) {
        // World 22/11: o anterior aparece (espiada); so a ponta esvanece, 40 px depois da rail/borda.
        float z0 = vitRailZero(), fimInt = xr + (float)(vitFixoN(w, passo) - 1) * passo + w;
        if (fimInt > NV_TELA_W - 1.0f) fimInt = NV_TELA_W - 1.0f;
        gfx_esvanecer_x(z0, z0 + 40.0f, fimInt, NV_TELA_W);
      }
      for (int c = 0; c < vitColunas(r); c++) {
        float cx = xr + c * passo - vitSx[r];
        float abre = exp_propria(&vitExp, r, c);
        float propria = 0.0f;
        cx += vitExpExtra(r, c, w, h, &propria);   // os da direita abrem (ou devolvem) espaco
        GfxRect base = { cx, cy, w + propria, h }, card = base;
        int sel = r == vitFR && c == vitFC && !tabFocus;
        float acc = ac;
        // Telefone: liga/desliga como sempre. TV: o foco entra e sai animado.
        float ff = vtTelefone() ? (sel ? 1.0f : 0.0f)
                 : sel ? vitFocoF : (r == vitFocoAntR && c == vitFocoAntC ? vitFocoAntF : 0.0f);
        if (cx + w < 0.0f) continue;
        if (cx > NV_TELA_W) {
          // Carregamento progressivo (18/11): PREVIA dos ate 6 cartazes alem da borda
          // na fileira em foco e na de baixo (um pedido por quadro, so com folga).
          if (!vtTelefone() && (r == vitFR || r == vitFR + 1))
            for (int k = c; k < vitColunas(r) && k < l->n && k < c + 6; k++) {
              const char *cam = home_vitrine_cartao_caminho(vitItem(r, k));
              if (cam && cam[0] && tex_previa_pedir(cam, w)) break;
            }
          break;
        }
        // 2.0.45: fileira ainda invisivel (antes da revelacao): nada desenhado, so o pedido do cartaz.
        if (acc < 0.01f) { if (c < l->n) (void)home_vitrine_cartao_aquecer(vitItem(r, c), w); continue; }
        // 2.0.45: CARTAZ QUE AINDA NAO CHEGOU NAO APARECE VAZIO. Nas 3 primeiras fileiras de uma abertura pela
        // transformacao ele fica fora ate chegar e entra sozinho com fade (revela_saida, 220 ms); passados
        // VIT_CART_SEGURA_MS da revelacao, um cartaz ainda a caminho volta ao esqueleto de sempre.
        if (c < l->n && r < VIT_CART_FIL && c < VIT_CART_COL && vitRevelaEm && !vtTelefone()) {
          int segura = vooTem || vitAgora - vitRevelaEm < VIT_CART_SEGURA_MS;
          if (segura || vitCartEsc[r][c]) {
            if (!home_vitrine_cartao_pronto(vitItem(r, c), w)) { if (segura) { vitCartEsc[r][c] = 1; continue; } }
            else if (vitCartEsc[r][c]) {
              if (!vitCartChegou[r][c]) vitCartChegou[r][c] = vitAgora ? vitAgora : 1u;
              float f = revela_saida((float)(vitAgora - vitCartChegou[r][c]) / NV_REVELA_MS);
              if (f >= 1.0f) { vitCartEsc[r][c] = 0; vitCartChegou[r][c] = 0; }
              acc *= f;
            }
          }
        }
        if (ff > 0.01f) {
          if (ajustes_borda_foco())
            // O raio da ARTE desta caixa (home_vitrine_cartao: raioDe(card)), pela altura. Era o
            // raio em px sobre o MENOR lado da caixa FECHADA: cartaz em pe e cartao aberto saiam com
            // o canto do anel mais redondo que o da arte, e o anel parecia cortado (TCL, 21/11).
            foco_anel(base, raioCard(base.w, base.h), ff, acc);   // 2.0.45: o anel entra com o cartaz (era `a`)
          else card = foco_zoom(base, ff);
        }
        if (c == l->n) {
          float raio = ajustes_raio_poster_px() / h;
          // O anel de foco sem vidro e um bloco SOLIDO atras do cartao; o "Ver tudo" e so
          // um veu translucido, entao o bloco aparecia inteiro (branco). Fundo escuro
          // opaco por baixo, como o do cartao de vidro.
          if (sel && ajustes_borda_foco() && !ajustes_vidro())
            gfx_cor(card, raio, 0.115f, 0.12f, 0.14f, acc);
          gfx_cor(card, raio, 1, 1, 1, acc * (sel ? .16f : .06f));
          gfx_rect(card, 0, GFX_ANEL, 0, 2.0f / h, 0, raio, 1, 1, 1, acc * (sel ? .82f : .12f));
          TxtLinha seta = txt_linha(TXT_TITULO2, "→", 236, 237, 242, 255);
          TxtLinha rotulo = txt_linha_corta(TXT_ROW_TITULO, i18n("Ver tudo"), 238, 240, 245, 255, w - 24.0f);
          float by = card.y + (card.h - seta.h - 14.0f - rotulo.h) * .5f;
          txt_desenhar_alpha(seta, card.x + (card.w - seta.w) * .5f, by, acc);
          txt_desenhar_alpha(rotulo, card.x + (card.w - rotulo.w) * .5f, by + seta.h + 14.0f, acc);
        } else if (abre > 0.001f) home_vitrine_cartao_aberto(vitItem(r, c), card, abre, acc,
                                                             w * (h > 0.0f ? card.h / h : 1.0f), sel);
        else home_vitrine_cartao(vitItem(r, c), base, card, acc);
#ifdef NV_TOUCH_UI
        if (vtTelefone()) {
          float left = fmaxf(base.x, x0), right = fminf(base.x + base.w, NV_TELA_W);
          if (right > left && base.y + base.h > topo - 8.0f && base.y < viewportBottom) {
            if (c == l->n) {
              vitPonteiroTudoVisivel[r] = 1;
              ponteiro_alvo_faixa(left, base.y, right - left, base.h,
                topo - 8.0f, viewportBottom, ponteiroVitrineTudo, abrirVitrineTudo,
                r, vitPonteiroGeracao);
              continue;
            }
            guardarChave(&vitPonteiroChave[r][c], vitItem(r, c));
            vitPonteiroVisivel[r][c] = 1;
            ponteiro_alvo_faixa(left, base.y, right - left, base.h,
              topo - 8.0f, viewportBottom, ponteiroVitrine, abrirVitrine,
              r * VIT_ITENS + c, vitPonteiroGeracao);
          }
        }
#endif
      }
      gfx_sem_esvanecer_x();
    } else if (l->estado == VIT_VAZIA || l->estado == VIT_PRONTA ||
               (vitLin[r].mais && !vitLin[r].n)) {
      const ColSource *s = vitMulti && collection && r < collection->nSources
                            ? &collection->sources[r] : NULL;
      const char *msg = l->semFonte ? mensagemSemFonte(s)
                                    : i18n("Nenhum título nesta lista.");
      TxtLinha m = txt_linha_corta(TXT_DET_META2, msg, 196, 202, 213, 255, 1200);
      txt_desenhar_alpha(m, x0, cy + 12.0f, ac * 0.9f);
      if (l->mais) botaoMaisVistos(ac, x0, cy + 60.0f, r);
    } else {
      // Esqueleto enquanto a fileira espera a vez ou a rede.
      if (!vtTelefone()) { float esp = fmaxf(1.0f, xr - (passo - w)), z0 = vitRailZero(); gfx_esvanecer_x(z0, fmaxf(esp, z0 + 1.0f), NV_TELA_W - esp, NV_TELA_W); }
      for (int c = 0; c < VIT_ITENS; c++) {
        GfxRect e = { xr + c * passo, cy, w, h };
        if (e.x > NV_TELA_W) break;
        gfx_cor(e, ajustes_raio_poster_px() / h, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G,
                NV_COR_ESQUELETO_B, ac * 0.7f);
      }
      gfx_sem_esvanecer_x();
    }
  }
  if (!vitN) {   // nada para repartir em fileiras ainda: diz por que
    const char *msg = semFonte
      ? mensagemSemFonte(collection && source < collection->nSources ? &collection->sources[source] : NULL)
      : desc_vertudo_erro() ? "Não foi possível carregar. OK para tentar novamente."
      : desc_vertudo_carregando() ? "Carregando títulos…" : "Nenhum título nesta lista.";
    txt_desenhar_alpha(txt_linha_corta(TXT_DET_META2, msg, 196, 202, 213, 255, 1200),
                       x0, topo + 20.0f, a);
  }
  gfx_sem_recorte();

  // 2.0.43: o cabecalho entra primeiro na abertura da Collection (aCab = a * ht_ent_cab).
  float aCab = a * (vooTem ? (vooSaindo ? 1.0f : fmaxf(tr_cabecalho(vitTr.p), vitTr.r))   // 2.0.45: durante o movimento
                           : vitEntradaT >= HT_ENT_TOTAL_MS ? 1.0f : ht_ent_cab(vitEntradaT));
  if (vooTem && !vooSaindo && aCab < 0.006f && a > 0.01f) aCab = 0.006f;   // o texto rasteriza antes de aparecer
  // A faixa de cima: a colecao a que isto pertence e o botao "Exibição".
  { TxtLinha eb = txt_linha(TXT_HERO_META, rotuloGrupo(), 214, 219, 227, 255);
    const char *wm = wordmarkServico();
    GLuint tl = wm[0] ? tex_obter_logo_larg(wm, 420.0f) : 0;
    float ap = tl ? tex_aspecto(wm) : 0.0f;
    // 2.0.45: A LOGO DA PASTA VAI DO HERO DA HOME AO CABECALHO (mesma imagem): sai do retangulo em que a
    // Home a desenhou e chega ao lugar dela na ilha em p = 0,60, inteira (sem apagar e reacender).
    int logoMove = vooTem && !vooSaindo && vooLogoTem && wm[0] && grupo("Streaming") && collection && collection->logo[0];
    GLuint tm = tl;
    float apm = ap;
    if (logoMove && !tm) { tm = tex_obter_logo_larg(vooLogoTex, NV_COLLECTION_HERO_LOGO_MAX_W + 40.0f); apm = tm ? tex_aspecto(vooLogoTex) : 0.0f; }
    if (logoMove && tm && apm > 0.0f) {
      float wl = 420.0f, hl = wl / apm, k = tr_saida(vitTr.p / 0.60f);
      if (hl > NV_ILHA_H) { hl = NV_ILHA_H; wl = hl * apm; }
      GfxRect de = vooLogoDe, rr = { de.x + (x0 - de.x) * k, de.y + (vtIlhaY() - de.y) * k,
                                     de.w + (wl - de.w) * k, de.h + (hl - de.h) * k };
      gfx_rect(rr, tm, tex_marca_escura(tm == tl ? wm : vooLogoTex) ? GFX_MARCA : GFX_TEXTO,
               0, 0, 0, 0, .96f, .97f, .98f, a);
    } else if (wm[0] && !tl && !tex_falhou(wm)) {
      // 2.0.45: logo do servico configurada e ainda a caminho: o lugar fica reservado (nunca o nome em texto
      // como substituto, a mesma regra da 2.0.44 para a logo da pasta na Home).
    } else if (tl && ap > 0.0f) {   // o wordmark cabe na linha da ilha
      float wl = 420.0f, hl = wl / ap;
      if (hl > NV_ILHA_H) { hl = NV_ILHA_H; wl = hl * ap; }
      gfx_rect((GfxRect){ x0, vtIlhaY(), wl, hl }, tl, tex_marca_escura(wm) ? GFX_MARCA : GFX_TEXTO,
               0, 0, 0, 0, .96f, .97f, .98f, aCab);
    } else {
      // UMA LINHA SO (dono, 14/10): sobretitulo e nome lado a lado, na mesma
      // linha de base, centrados na ilha do relogio; o nome corta antes dela.
      float ex = grupo("Streaming") ? x0 : x0 + eb.w + 20.0f;
      TxtLinha tt = txt_linha_corta(TXT_TITULO3, titulo, 246, 247, 250, 255,
                                    NV_TELA_W - NV_ILHA_MARGEM_D - NV_ILHA_REP_W - 40.0f - ex);
      float ty = vtIlhaY() + (NV_ILHA_H - tt.h) * 0.5f, base = ty + tt.h * 0.8f;
      if (!grupo("Streaming")) txt_desenhar_alpha(eb, x0, base - eb.h * 0.8f, aCab * 0.85f);
      if (vooNomeVoa() && vooTem && !vooSaindo && vooNomeDe.h > 1.0f && tt.h > 0) {
        // O NOME VIAJA: sai do retangulo em que a Home o escreveu (TITULO1) e assenta no do cabecalho
        // (TITULO3) em p = 0,60, como a logo. A linha grande e escalada no caminho e cede a do
        // cabecalho no ultimo quarto (cruzam no mesmo retangulo: sem salto de nitidez).
        float k = tr_saida(vitTr.p / 0.60f), sk = k;   // a mesma curva da logo
        float fim = k <= 0.75f ? 0.0f : (k - 0.75f) / 0.25f;
        fim = fim * fim * (3.0f - 2.0f * fim);
        GfxRect rr = { vooNomeDe.x + (ex - vooNomeDe.x) * sk, vooNomeDe.y + (ty - vooNomeDe.y) * sk,
                       vooNomeDe.w + ((float)tt.w - vooNomeDe.w) * sk, vooNomeDe.h + ((float)tt.h - vooNomeDe.h) * sk };
        TxtLinha big = txt_linha_corta(TXT_TITULO1, titulo, 241, 243, 247, 255, vooNomeDe.w + 8.0f);
        if (big.tex && fim < 0.999f)
          gfx_rect(rr, big.tex, GFX_TEXTO, 0, 0, 0, 0.0f, 1, 1, 1, a * (1.0f - fim));
        if (tt.tex && fim > 0.001f)
          gfx_rect(rr, tt.tex, GFX_TEXTO, 0, 0, 0, 0.0f, 1, 1, 1, a * fim);
      } else
      txt_desenhar_alpha(tt, ex, ty, aCab);
      cabEb = (GfxRect){ x0, base - eb.h * 0.8f, eb.w, eb.h }; cabTt = (GfxRect){ ex, ty, tt.w, tt.h };
    } }
  // Anel e seletor na linha de baixo, como nas outras exibicoes.
  cabFim = 0.0f;
  if (listaDePessoa() && vcTotal > 0 && !vtTelefone()) linhaVistos(x0, cabCy, aCab);
  botaoExibicao(aCab, cabCy);
#ifdef NV_TOUCH_UI
  if (vtTelefone()) vitPonteiroPronto = 1;
#endif
}

// O CARTAO DO VOO (2.0.44), por cima do conteudo: a capa da pasta cresce da posicao real na Home ate a tela
// cheia, cruza para o banner (que ja esta em cache, e o Hero da Home) e apaga dando lugar ao conteudo.
// Duas camadas no maximo (capa e banner), so posicao/tamanho/opacidade: nada alem do que ja esta em cache.
static void vooDesenhar(void) {
  if (!vooTem || !collection) return;
  // 2.0.45: o cartao cresce ate o destaque e, ja no lugar, CEDE a camada de destaque (mesma imagem, mesmo
  // retangulo): nao apaga sobre outra coisa. Voltar no meio: apaga com a Vitrine, sem mudar de lugar.
  const float al = tr_cartao_alfa(vitTr.r) * (vooSaindo ? anim : 1.0f);
  if (al <= 0.004f) return;
  const float p = vooP(), mb = tr_banner(vitTr.p);
  GfxRect r = vooRect();
  const float raio = r.h > 0.0f ? (vooRaioPx * (1.0f - p)) / r.h : 0.0f;
  const char *capa = col_capa(collection), *ban = col_banner(collection);
  GLuint tc = capa && capa[0] ? tex_obter_larg_qualquer(capa, vooOrigem.w) : 0;
  GLuint tb = ban && ban[0] && mb > 0.004f ? tex_obter_hero(ban) : 0;
  const float foco = 1.0f - p;   // o brilho do foco do cartao vai embora ao crescer
  // A capa so enquanto o banner nao a cobre inteira: depois disso o cartao e UMA camada (preenchimento da TV).
  if (!(tb && mb >= 0.999f)) {
    if (tc) { gfx_tex_aspect_atual = tex_aspecto(capa); gfx_rect(r, tc, GFX_CARD, foco, 0, 0, raio, 0, 0, 0, al); gfx_tex_aspect_atual = 0; }
    else gfx_cor(r, raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, al);
  }
  if (tb) {   // o banner SEMPRE preenche (cover): a capa e cartaz retrato e nao cabe numa tela 16:9
    gfx_tex_aspect_atual = tex_aspecto(ban); gfx_card_forcar_cover_atual = 1.0f;
    gfx_rect(r, tb, GFX_CARD, foco, 0, 0, raio, 0, 0, 0, al * mb);
    gfx_card_forcar_cover_atual = 0.0f; gfx_tex_aspect_atual = 0; }
}
static void vertudoCorpo(Uint32 agora);
void vertudo_desenhar(Uint32 agora) { vooDesenhado = 0; vertudoCorpo(agora); if (!vooDesenhado) vooDesenhar(); }
static void vertudoCorpo(Uint32 agora) {
  vitAgora = agora;
  float a = anim, x0 = vtInicio();
  int n = nItens(), i, lin0 = -1;
  float corteTopo = VT_TOPO - 12.0f;
  if (a < 0.01f) return;
  float escalaAnterior = gfx_escala();
  gfx_escala_sair(vtEscala());
#ifdef NV_TOUCH_UI
  toqueAbas.offset = NULL;
  ponteiro_rolagem(toqueVertudoRolar);
#endif
  if (n > VT_MAX) n = VT_MAX;
  if (lay == VT_LAY_VITRINE) {
    vitDesenhar(a, x0);
    gfx_escala_sair(escalaAnterior);
    return;
  }
  n = atualizarGrade();
  x0Atual = x0;
  if (ondaArmada && n > 0) { ondaEm = agora ? agora : 1u; ondaArmada = 0; }
  if (ondaEm && revela_onda_fim(ondaEm, agora)) ondaEm = 0;
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    gfx_cor(tela, 0.0f, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, a); }
  themeBackground(a);

  // DECADAS: o cabecalho da secao de cima fica PRESO no topo enquanto ela rola;
  // os cartazes passam a ser recortados abaixo dele. A secao seguinte, ao
  // chegar, apaga a copia presa (sem empurrar para cima das abas).
  int presa = -1; float presaA = 0.0f;
  if (lay == VT_LAY_DECADAS && decN) {
    for (int k = 0; k < decN; k++) if (VT_TOPO + decY[k] - scrollY <= VT_TOPO + 0.5f) presa = k;
    if (presa >= 0) {
      float prox = presa + 1 < decN ? VT_TOPO + decY[presa + 1] - scrollY : 1e9f;
      presaA = anim_clamp((prox - VT_TOPO) / VT_DEC_H - 0.25f, 0.0f, 1.0f);
      corteTopo = VT_TOPO + VT_DEC_H - 24.0f;
    }
  }

  // A GRADE E RECORTADA ABAIXO DO CABECALHO.
  //
  // O cabecalho ja era desenhado em posicao fixa, mas os cartazes passavam POR
  // TRAS dele ao rolar — o titulo ficava sobre imagem em movimento e virava
  // "fundo". O recorte resolve sem precisar de faixa opaca: o que sobe alem do
  // topo simplesmente nao e desenhado.
  if (lay == VT_LAY_DECADAS) {
    gfx_recorte(0.0f, VT_TOPO - 12.0f, NV_TELA_W, NV_TELA_H - VT_TOPO + 12.0f);
    for (int k = 0; k < decN; k++) {
      float hy = VT_TOPO + decY[k] - scrollY;
      if (k == presa || hy > NV_TELA_H || hy + VT_DEC_H < VT_TOPO) continue;
      cabecalhoDecada(k, x0, hy, a);
    }
    if (presa >= 0 && presaA > 0.01f) cabecalhoDecada(presa, x0, VT_TOPO, a * presaA);
    gfx_sem_recorte();
  }
  if (lay == VT_LAY_FLUXO) fluxoDesenhar(n, x0, a, agora);
  gfx_recorte(0.0f, corteTopo, NV_TELA_W, NV_TELA_H - corteTopo);
  gradePonteiroRevisao = gradeRevisao; gradePonteiroValido = 1;
  for (i = 0; lay != VT_LAY_FLUXO && i < n; i++) {
    float cx = x0 + celX[i], cy = VT_TOPO + celY[i] - scrollY;
    float w = celW[i], h = celH[i];
    CatItem it;
    int sel = (i == foco) && !tabFocus;
    if (cy > NV_TELA_H) { if (lay != VT_LAY_MURAL) break; else continue; }
    if (cy + h + 120.0f < corteTopo) continue;
    if (lin0 < 0) lin0 = celLin[i];
    // ONDA: cada cartaz sobe alguns pixels e ganha opacidade com atraso pela
    // coluna e pela fileira visiveis (revela.h). So na primeira aparicao.
    float entra = ondaEm ? revela_entra(ondaEm, revela_onda_atraso(celCol[i],
                                         celLin[i] - lin0), agora) : 1.0f;
    // PAGINAS (#205): a pagina nova entra deslizando do lado de onde veio.
    float desl = pgSlide * 72.0f;
    float ac = a * entra * (1.0f - 0.8f * fminf(1.0f, fabsf(pgSlide)));
    if (ac < 0.005f) continue;
    cx += desl;
    cy += (1.0f - entra) * NV_ENTRA_DY;
    if (lay == VT_LAY_LINHA) { GfxRect r=vtTimelineCard(cy); ponteiro_alvo_faixa(r.x,r.y,r.w,r.h,corteTopo,NV_TELA_H,ponteiroCartaz,NULL,i,0); }
    else ponteiro_alvo_faixa(cx,cy,w,h+40.0f,corteTopo,NV_TELA_H,ponteiroCartaz,NULL,i,0);
    if (lay == VT_LAY_LINHA) { timelineCard(i, cy, ac, x0 + desl); continue; }
    if (!viewItem(i, &it)) continue;
    switch (lay) {
      case VT_LAY_RANKING: rankingCard(i, &it, cx, cy, w, h, ac, agora, sel); break;
      case VT_LAY_SAGA:    sagaCard(i, &it, cx, cy, w, h, ac, agora, sel, n); break;
      case VT_LAY_MURAL:   muralTile(i, &it, cx, cy, w, h, ac, agora, sel); break;
      default:             cartazGrade(i, &it, cx, cy, w, h, ac, agora, sel);
    }
  }
  if (!n) botaoMaisVistos(a, x0, VT_TOPO + 32.0f, 0);
  if(!n&&desc_vertudo_carregando())for(int i=0;i<VT_COLS;i++) {
    float w=vtTelefone()&&!timeline?VT_CARD_W:248.0f;
    float h=vtTelefone()&&!timeline?VT_CARD_H:372.0f;
    gfx_cor((GfxRect){x0+i*(w+VT_GAP_X),VT_TOPO,w,h},.06f,.12f,.13f,.15f,a);
  }
  // O que passa da base apaga no fundo, nao e cortado seco na borda da tela.
  // O painel vem depois e fica por cima.
  if (n > 0 && lay != VT_LAY_FLUXO)
    gfx_rect((GfxRect){ 0, NV_TELA_H - 120.0f, NV_TELA_W, 120.0f }, 0, GFX_VEU_BAIXO,
             0, 0, 0, 0, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 0.92f * a);
  gfx_sem_recorte();

  // CABECALHO por cima do recorte, entao ele nunca compete com a arte.
  themeHeader(a,x0);
  if (collection) botaoExibicao(a, cabCy);

  if (n > 0 && !vtTelefone()) { if (lay == VT_LAY_FLUXO) fluxoInfo(a, x0); else painel(a); }
  gfx_escala_sair(escalaAnterior);
}

// ---- OPCOES DA COLLECTION NO PAINEL DO CH+ (salvospainel.c, aba Collection) ----
int vertudo_em_colecao(void) { return aberta && collection != NULL && !vtTelefone(); }
// Linhas: as exibicoes (na ordem do antigo botao) e, com abas fora da Vitrine,
// o estilo das abas.
static int opcEstilo(void) { return lay != VT_LAY_VITRINE && nAbas() > 0; }
int vertudo_opcoes_n(void) {
  if (!vertudo_em_colecao()) return 0;
  return (int)sizeof LAY_CICLO + opcEstilo();
}
const char *vertudo_opcao_rotulo(int i, int *marcada, const char **icone) {
  static char buf[96];
  int nl = (int)sizeof LAY_CICLO;
  if (marcada) *marcada = 0;
  if (icone) *icone = NULL;
  if (i < 0 || i >= vertudo_opcoes_n()) return "";
  if (i < nl) {
    int l = LAY_CICLO[i];
    int escolhida = layEscolha >= VT_LAY_AUTO && layEscolha < VT_LAY_N ? layEscolha : VT_LAY_AUTO;
    if (marcada) *marcada = vitTudo ? l == VT_LAY_VITRINE : l == escolhida;
    if (icone) *icone = LAY_ICONE[l];
    return i18n(LAY_NOME[l]);
  }
  { static const char *EST[] = { "Chips", "Menu suspenso", "Barra central", "Páginas", "Menu no topo" };
    int e = estiloAbas();
    snprintf(buf, sizeof buf, "%s: %s", i18n("Estilo das abas"), i18n(EST[e >= 0 && e <= VT_AB_TOPO ? e : 0]));
    if (icone) *icone = "aj_layout-list";
    return buf; }
}
// 1 = a escolha fecha o painel (a exibicao mudou); 0 = fica aberto.
int vertudo_opcao_ok(int i) {
  int nl = (int)sizeof LAY_CICLO;
  if (i < 0 || i >= vertudo_opcoes_n()) return 0;
  if (i < nl) { escolherExibicao(LAY_CICLO[i]); tabFocus = 0; exibAberta = 0; return 1; }
  ajustes_definir_abas_estilo((ajustes_abas_estilo() + 1) % (VT_AB_TOPO + 1));
  abasReiniciar(); tabRol = 0; tabFocus = 0;
  return 0;
}
