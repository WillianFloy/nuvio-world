// LOGS DE DIAGNOSTICO (dono, 20/11/2026): Ajustes > Sobre e ajuda > Logs de
// diagnostico. Ilha modal no mesmo desenho do painel de envio (registro_envio.inc):
// a esquerda o QR do endereco da sessao (abre a pagina no celular ou no
// computador da mesma rede), a direita o estado, o tempo e os botoes.
//   Parado:   [Iniciar logs] [Fechar]
//   Gravando: [Finalizar logs] [Continuar usando]   (o tempo correndo)
//   Salvando: (espera curta, sem botao)
//   Pronto:   [Fechar] [Nova sessao]                (contagem de 2:00, QR ainda vale)
// Fora do painel, uma pilula no alto da tela diz que a gravacao esta ativa
// (registro_diag_indicador). Incluido por registro.c.
#include "diaglog.h"
#include "logrede.h"
#include "qrtex.h"

static int dgAberto, dgFoco;
static GLuint dgQrTex;
static char dgQrUrl[200];
static DiagEstado dgUltEstado;
static Uint32 dgFimAvisoEm;   // "Sessao encerrada" por alguns segundos depois do prazo

int registro_diag_aberto(void) { return dgAberto; }
void registro_diag_abrir(void) { dgAberto = 1; dgFoco = 0; }

static int dgPt(void) {
  int i = ajustes_idioma();
  return i == IDIOMA_PT || i == IDIOMA_PTPT;
}

// Os botoes de cada estado, na ordem das setas. Devolve quantos.
static int dgBotoes(DiagEstado e, const char **rot, const char **ic) {
  switch (e) {
    case DIAG_GRAVANDO:
      rot[0] = "Finalizar logs"; ic[0] = "aj_check";
      rot[1] = "Continuar usando"; ic[1] = NULL;
      return 2;
    case DIAG_SALVANDO: return 0;
    case DIAG_PRONTO:
      rot[0] = "Fechar"; ic[0] = NULL;
      rot[1] = "Nova sessão"; ic[1] = "aj_rotate-cw";
      return 2;
    default:
      rot[0] = "Iniciar logs"; ic[0] = "aj_scroll-text";
      rot[1] = "Fechar"; ic[1] = NULL;
      return 2;
  }
}

static void dgAcionar(DiagEstado e, int i) {
  switch (e) {
    case DIAG_GRAVANDO:
      if (i == 0) diaglog_finalizar(); else dgAberto = 0;   // gravando continua
      dgFoco = 0;
      break;
    case DIAG_PRONTO:
      if (i == 0) dgAberto = 0;
      else { diaglog_iniciar(dgPt()); dgFoco = 0; }
      break;
    case DIAG_SALVANDO: break;
    default:
      if (i == 0) { diaglog_iniciar(dgPt()); dgFoco = 0; }
      else dgAberto = 0;
      break;
  }
}

int registro_diag_evento(const SDL_Event *e) {
  DiagInfo info;
  const char *rot[2], *ic[2];
  int n;
  SDL_Keycode k;
  if (!dgAberto) return 0;
  if (e->type == SDL_KEYUP &&
      ((soltarSym && e->key.keysym.sym == soltarSym) ||
       (soltarScan >= 0 && (int)e->key.keysym.scancode == soltarScan))) {
    soltarSym = 0; soltarScan = -1;
    return 1;
  }
  if (e->type != SDL_KEYDOWN) return e->type == SDL_KEYUP || e->type == SDL_TEXTINPUT;
  k = e->key.keysym.sym;
  if (nv_tecla_voltar(e)) { dgAberto = 0; engolirSoltura(e); return 1; }   // a sessao segue
  diaglog_info(&info);
  n = dgBotoes(info.estado, rot, ic);
  if (k == SDLK_LEFT || k == SDLK_RIGHT) {
    if (n > 1) dgFoco = k == SDLK_RIGHT ? (dgFoco + 1 < n ? dgFoco + 1 : dgFoco) : (dgFoco > 0 ? dgFoco - 1 : 0);
    return 1;
  }
  if (ehOk(k) && n) { dgAcionar(info.estado, dgFoco < n ? dgFoco : 0); engolirSoltura(e); }
  return 1;
}

static void dgPonteiro(int i, int b) {
  (void)b;
  if (dgAberto && i >= 0 && i < 2) dgFoco = i;
}

static void dgTempo(char *d, size_t n, long ms, int contagem) {
  long s = contagem ? (ms + 999) / 1000 : ms / 1000;
  if (s < 0) s = 0;
  if (contagem) snprintf(d, n, "%ld:%02ld", s / 60, s % 60);
  else if (s >= 3600) snprintf(d, n, "%ld:%02ld:%02ld", s / 3600, (s / 60) % 60, s % 60);
  else snprintf(d, n, "%02ld:%02ld", s / 60, s % 60);
}

// O QR: textura refeita so quando o endereco muda (IP novo, sessao nova).
static int dgQr(const char *url) {
  if (!url[0]) return 0;
  if (dgQrTex && !strcmp(url, dgQrUrl)) return 1;
  if (dgQrTex) { glDeleteTextures(1, &dgQrTex); dgQrTex = 0; }
  dgQrUrl[0] = 0;
  if (!qr_textura(&dgQrTex, url)) return 0;
  snprintf(dgQrUrl, sizeof dgQrUrl, "%s", url);
  return 1;
}

// A caixa do QR: cartao claro (o leitor precisa de contraste), o codigo grande
// e o endereco por extenso embaixo para quem quiser digitar.
static void dgCaixaQr(float x, float y, float w, float h, const char *url, int ativo) {
  float lado = 300, qx = x + (w - lado) * 0.5f, qy = y + 34;
  if (ativo && dgQr(url)) {
    // Como o QR do login: moldura clara e GFX_SNAP (modulos nitidos, sem filtro).
    gfx_cor((GfxRect){ qx - 18, qy - 18, lado + 36, lado + 36 }, 22.0f / (lado + 36), 1, 1, 1, 1);
    gfx_tex_aspect_atual = 0.0f;
    gfx_rect((GfxRect){ qx, qy, lado, lado }, dgQrTex, GFX_SNAP, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 1.0f);
    { TxtLinha t = rgT(TXT_AJ_ESTADO, "Aponte a câmera do celular");
      txt_desenhar_alpha(t, x + (w - t.w) * 0.5f, qy + lado + 34, 0.62f); }
    { TxtLinha t = rgTC(TXT_AJ_ESTADO, url, w - 12);
      txt_desenhar_alpha(t, x + (w - t.w) * 0.5f, qy + lado + 70, 0.92f); }
    return;
  }
  rgNeutro((GfxRect){ x, y + 20, w, h - 40 }, 24, 0.06f, RG_CART_SOL, 1);
  rgIcone("aj_wifi", x + (w - 48) * 0.5f, y + h * 0.5f - 70, 48, 0.35f);
  rgBloco(TXT_AJ_TEXTO, ativo ? "Procurando o endereço desta TV na rede de casa…"
                              : "O endereço e o QR aparecem aqui ao iniciar. Celular ou computador precisam estar na mesma rede da TV.",
          x + 40, y + h * 0.5f - 6, w - 80, 30, 0.6f, 4);
}

static void registro_diag_desenharCorpo_(void) {
  DiagInfo info;
  const char *rot[2], *ic[2], *url;
  float x, y, w, base = RE_Y + RE_H - 56, qrW = 420;
  char v[220], t[48];
  int n, i;
  if (!dgAberto) return;
  ponteiro_camada();
  diaglog_info(&info);
  if (info.estado != dgUltEstado) { if (dgFoco > 1) dgFoco = 0; dgUltEstado = info.estado; }
  url = diaglog_url();
  // Veu de tela cheia: a lista dos Ajustes nao atravessa o painel (texto sobre
  // texto ilegivel a 3 m); o vidro da ilha passa a desfocar o fundo escurecido.
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W + 4.0f, NV_TELA_H + 4.0f }, 0, 0, 0, 0, 0.72f);
  gfx_vidro_cobrir();
  ajustes_ui_ilha((GfxRect){ RE_X, RE_Y, RE_W, RE_H }, 36, 1);
  dgCaixaQr(RE_X + 50, RE_Y + 40, qrW, RE_H - 80, url, info.estado != DIAG_PARADO);
  x = RE_X + 50 + qrW + 56; w = RE_X + RE_W - 60 - x; y = RE_Y + 56;
  switch (info.estado) {
    case DIAG_GRAVANDO:
      rgRespira(x + 4, y + 9, RG_VERM);
      rgCaps(TXT_MINI, "Gravando", 2.1f, x + 28, y, 242, 140, 130, 1);
      y += 18 + 10;
      dgTempo(t, sizeof t, info.decorridoMs, 0);
      { TxtLinha l = rgT(TXT_LOG_COD, t); txt_desenhar(l, x - 4, y); y += l.h + 6; }
      y += rgBloco(TXT_AJ_TEXTO, "Use o app normalmente: tudo a partir de agora entra no arquivo. Volte aqui e escolha Finalizar logs.",
                   x, y, w, 30, 0.66f, 3) + 22;
      snprintf(v, sizeof v, i18n("%ld linhas · %ld KB"), info.linhas, (info.bytes + 1023) / 1024);
      y += rgStat(i18n("Gravado"), v, x, y, w);
      break;
    case DIAG_SALVANDO:
      rgRespira(x + 4, y + 9, RG_AMBAR);
      rgCaps(TXT_MINI, "Salvando", 2.1f, x + 28, y, 233, 194, 122, 1);
      y += 18 + 10;
      y += rgBloco(TXT_ILHA_TITULO, "Salvando o arquivo…", x, y, w, 48, 1, 2) + 14;
      rgBloco(TXT_AJ_TEXTO, "Fechando a gravação e tirando senhas e chaves. Leva poucos segundos.", x, y, w, 30, 0.62f, 3);
      break;
    case DIAG_PRONTO:
      rgIconeCor("aj_check", x, y - 1, 18, RG_VERDE, 1);
      rgCaps(TXT_MINI, "Pronto para baixar", 2.1f, x + 28, y, 76, 195, 138, 1);
      y += 18 + 10;
      dgTempo(t, sizeof t, info.restanteMs, 1);
      { TxtLinha l = rgT(TXT_LOG_COD, t); txt_desenhar(l, x - 4, y);
        TxtLinha k2 = rgT(TXT_AJ_ESTADO, "para o endereço fechar");
        txt_desenhar_alpha(k2, x + l.w + 14, y + l.h - k2.h - 14, 0.55f); y += l.h + 6; }
      y += rgBloco(TXT_AJ_TEXTO, "No celular ou no computador, abra o endereço e toque em Baixar log.txt antes de a contagem acabar.",
                   x, y, w, 30, 0.66f, 3) + 22;
      snprintf(v, sizeof v, i18n("%ld linhas · %ld KB"), info.linhas, (info.bytes + 1023) / 1024);
      y += rgStat(i18n("Arquivo"), v, x, y, w);
      dgTempo(t, sizeof t, info.decorridoMs, 0);
      y += rgStat(i18n("Gravado por"), t, x, y, w);
      break;
    default:
      ajustes_ui_kicker("Sobre e ajuda", x, y, 1);
      y += 18 + 10;
      y += rgBloco(TXT_ILHA_TITULO, dgFimAvisoEm && SDL_GetTicks() - dgFimAvisoEm < 8000 ? "Sessão encerrada" : "Logs de diagnóstico",
                   x, y, w, 48, 1, 2) + 14;
      y += rgBloco(TXT_AJ_TEXTO, "Grava só o que acontecer a partir de agora. Ao finalizar, o arquivo fica 2 minutos disponível para baixar pelo celular ou computador da mesma rede, pelo QR. Sem senhas nem chaves.",
                   x, y, w, 30, 0.66f, 5) + 22;
      break;
  }
  if (info.estado != DIAG_PARADO) {
    if (url[0]) y += rgStat(i18n("Endereço"), url, x, y, w);
    else y += rgStat(i18n("Endereço"), logrede_ativo() ? i18n("sem rede local") : i18n("abrindo…"), x, y, w);
  }
  (void)y;
  n = dgBotoes(info.estado, rot, ic);
  { float bx = x;
    for (i = 0; i < n; i++) {
      float bw = rgBotao(rot[i], ic[i], bx, base - 56, dgFoco == i, 1);
      ponteiro_alvo(bx, base - 56, bw, 56, dgPonteiro, NULL, i, 0);
      bx += bw + 12;
    } }
  if (info.estado == DIAG_SALVANDO) {
    float ar, ag, ab;
    TxtLinha l = rgT(TXT_AJ_SUB, "Salvando…");
    ajustes_acento(&ar, &ag, &ab);
    rgRespira(x, base - 28, ar, ag, ab);
    txt_desenhar_alpha(l, x + 22, base - 28 - l.h * 0.5f, 0.7f);
  }
  rgKbdTexto("Voltar", info.estado == DIAG_GRAVANDO ? "Fechar e continuar gravando" : "Fechar", x + w, base - 84);
}
void registro_diag_desenhar(void) {
  ESCALA_INI();
  registro_diag_desenharCorpo_();
  ESCALA_FIM();
}

// A PILULA NO ALTO DA TELA, fora do painel: "Gravando logs 02:15" com o ponto
// vermelho; "Logs prontos · 1:45" na janela do download. Some com o painel
// aberto (ele ja diz) e parado.
void registro_diag_indicador(void) {
  DiagInfo info;
  char t[48], s[120];
  float pw, px, py = 18;
  TxtLinha l;
  diaglog_info(&info);
  if (info.estado != dgUltEstado) {
    if (dgUltEstado == DIAG_PRONTO && info.estado == DIAG_PARADO) dgFimAvisoEm = SDL_GetTicks();
    if (!dgAberto) dgUltEstado = info.estado;
  }
  if (info.estado == DIAG_PARADO || dgAberto) return;
  if (info.estado == DIAG_GRAVANDO) { dgTempo(t, sizeof t, info.decorridoMs, 0); snprintf(s, sizeof s, "%s  %s", i18n("Gravando logs"), t); }
  else if (info.estado == DIAG_SALVANDO) snprintf(s, sizeof s, "%s", i18n("Salvando logs…"));
  else { dgTempo(t, sizeof t, info.restanteMs, 1); snprintf(s, sizeof s, "%s · %s", i18n("Logs prontos para baixar"), t); }
  ESCALA_INI();
  l = txt_linha(TXT_AJ_ESTADO, s, RG_TX, 255);
  pw = 20 + 12 + 10 + l.w + 22;
  px = (NV_TELA_W - pw) * 0.5f;
  gfx_cor((GfxRect){ px, py, pw, 44 }, 0.5f, 0.04f, 0.04f, 0.05f, 0.88f);
  gfx_anel((GfxRect){ px, py, pw, 44 }, 0.5f, 1.0f, 1, 1, 1, 0.10f);
  if (info.estado == DIAG_GRAVANDO) rgRespira(px + 20, py + 22, RG_VERM);
  else if (info.estado == DIAG_SALVANDO) rgPonto(px + 20, py + 22, 10, RG_AMBAR);
  else rgPonto(px + 20, py + 22, 10, RG_VERDE);
  txt_desenhar_alpha(l, px + 20 + 10 + 12, py + 22 - l.h * 0.5f, 0.95f);
  ESCALA_FIM();
}
