// World (23/11): faixas do trailer da Apple no Android (trailer.c real com -DNV_ANDROID; o player e simulado).
// Pedido do dono: audio e legenda do trailer escolhidos SEPARADAMENTE; em portugues quando ha.
//   1. ha audio em portugues (dublado): escolhe esse audio e nao liga legenda;
//   2. so audio original: em tela cheia liga a legenda no idioma de legenda (pt);
//   3. no fundo (fora da tela cheia) nao liga legenda;
//   4. pausado: direita vai a Audio, OK troca o audio; direita vai a Legenda, OK anda pelas legendas e desliga;
//   5. sem pausa, OK continua sendo pausar (nao troca faixa); a escolha automatica e uma vez por fonte.
#include "trailer.h"
#include "video.h"
#include <stdio.h>
#include <string.h>
static int falhas, nTocar, nParar, nYtAbrir, ytMudo, ytEstado = -1, pagPos, pagDur, ytDisp = 1;
static int cmds[32], nCmd;
static char ytId[32], ytLeg[8];
static Uint32 relogio = 1000;
int video_janela_pronta(void) { return 1; }
#define SDL_GetTicks() (relogio)
#undef __APPLE__
#include "../src/trailer.c"
#undef SDL_GetTicks
int  video_yt_disponivel(void) { return ytDisp; }
void video_yt_abrir(const char *id, int mudo, const char *leg) {
  nYtAbrir++; ytMudo = mudo; snprintf(ytId, sizeof ytId, "%s", id); snprintf(ytLeg, sizeof ytLeg, "%s", leg); }
void video_yt_cmd(int c) { if (nCmd < 32) cmds[nCmd++] = c; }
int  video_yt_estado(void) { return ytEstado; }
int  video_yt_pos_ms(void) { return pagPos; }
int  video_yt_dur_ms(void) { return pagDur; }
int  video_tocar(const char *u) { (void)u; nTocar++; return 1; }
int  video_pronto(void) { return 1; }
int  video_largura(void) { return 1920; }
int  video_altura(void) { return 1080; }
int  video_tocando(void) { return 1; }
int  video_ativo(void) { return 1; }
void video_volume(int v) { (void)v; }
void video_janela(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void video_janela_fonte(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
  (void)sx; (void)sy; (void)sw; (void)sh; (void)dx; (void)dy; (void)dw; (void)dh; }
void video_recorte_reaplicar(void) {}
float ajustes_trailer_zoom(void) { return 1.0f; }
int  video_recorte_fonte(void) { return 1; }
void video_parar(void) { nParar++; }
int  video_iniciar_auto(void) { return 1; }
int  video_conflito_recurso(void) { return 0; }
int  video_falhou(void) { return 0; }
int  video_terminou(void) { return 0; }
void video_bombear(void) {}
void video_pausar(int p) { (void)p; }
double video_pos(void) { return 99; }
double video_duracao(void) { return 99; }
double video_buffer_fim(void) { return 99; }
int  trailerfonte_com_som(int t) { (void)t; return 1; }
int  trailerfonte_tizen(void) { return 0; }
float gfx_escala_tela_txt(void) { return 1.0f; }
static VideoFaixa AUD[3], LEG[3];
static int nAud, nLeg, audAt, legAt = -1, nEscA, nEscL;
int  video_n_audio(void) { return nAud; }
int  video_n_legenda(void) { return nLeg; }
const VideoFaixa *video_audio(int i) { return i >= 0 && i < nAud ? &AUD[i] : NULL; }
const VideoFaixa *video_legenda(int i) { return i >= 0 && i < nLeg ? &LEG[i] : NULL; }
int  video_audio_atual(void) { return audAt; }
int  video_legenda_atual(void) { return legAt; }
void video_escolher_audio(int i) { audAt = i; nEscA++; }
void video_escolher_legenda(int i) { legAt = i; nEscL++; }
// TMDB: 0 dublado (pt, preferido), 1 legendado, 2 ingles.
static const char *TY[3] = { "dubladoAAA1", "legendaBBB2", "inglesCCCC3" };
int extras_n_trailers(void) { return 3; }
const char *extras_trailer_yt(int i) { return i >= 0 && i < 3 ? TY[i] : ""; }
int extras_trailer_idioma_preferido(int i) { return i == 0; }
int extras_trailer_legendado(int i) { return i == 1; }
void extras_trailer_audio_iso(char *d) { strcpy(d, "pt"); }
const char *ling_legenda(void) { return "pt-BR"; }
void ling_normalizar(const char *in, char *out, unsigned cap) { snprintf(out, cap, "%s", in); }

static int ytPos_teste_ok(void) { return ytPos() > 30.4 && ytPos() < 30.6 && ytDur() > 119.9 && ytDur() < 120.1; }
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
static const GfxRect TELA = { 0, 0, NV_TELA_W, NV_TELA_H };
static void correr(int ms) { for (int t = 0; t < ms; t += 16) { relogio += 16; trailer_atualizar(relogio); } }
static SDL_Event tecla(SDL_Keycode k) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = k; return e; }
static void faixas(int dub) {
  memset(AUD, 0, sizeof AUD); memset(LEG, 0, sizeof LEG);
  strcpy(AUD[0].idioma, "en"); nAud = 1;
  if (dub) { strcpy(AUD[1].idioma, "pt-BR"); nAud = 2; }
  strcpy(LEG[0].idioma, "en"); strcpy(LEG[1].idioma, "pt-BR"); strcpy(LEG[2].idioma, "es"); nLeg = 3;
  audAt = 0; legAt = -1; nEscA = nEscL = 0;
}
int main(void) {
  SDL_Event okk = tecla(SDLK_RETURN), dir = tecla(SDLK_RIGHT), esq = tecla(SDLK_LEFT);
  faixas(1);
  trailer_abrir("file:///a-play.m3u8", TELA, 1, 1); correr(100);
  ok(audAt == 1 && legAt == -1, "1. audio em portugues existe: escolhido, sem legenda");
  trailer_fechar();

  faixas(0);
  trailer_abrir("file:///b-play.m3u8", TELA, 1, 1); correr(100);
  ok(audAt == 0 && legAt == 1, "2. so audio original: legenda pt ligada na tela cheia");
  correr(500);
  ok(nEscL == 1, "5. a escolha automatica e uma vez por fonte");
  trailer_evento(&okk);
  ok(trailer_pausado(), "pausa");
  trailer_evento(&dir); trailer_evento(&okk);
  ok(audAt == 0 && legAt == 2, "4. um audio so: a direita vai direto a Legenda e OK anda (pt -> es), audio intacto");
  trailer_evento(&okk);
  ok(legAt == -1, "4. depois da ultima, desligada");
  trailer_evento(&okk);
  ok(legAt == 0, "4. e volta a primeira");
  trailer_evento(&esq); trailer_evento(&okk);
  ok(!trailer_pausado(), "4. esquerda volta a Continuar e OK continua");
  trailer_fechar();

  faixas(1);
  trailer_abrir("file:///c-play.m3u8", TELA, 1, 1); correr(100);
  trailer_evento(&okk);
  trailer_evento(&dir); trailer_evento(&okk);
  ok(audAt == 0 && trailer_pausado(), "4. com dois audios: Audio e o primeiro a direita e OK troca (pt -> en)");
  trailer_evento(&dir); trailer_evento(&okk);
  ok(legAt == 0, "4. Legenda logo depois: OK liga a primeira");
  trailer_fechar();

  faixas(0);
  trailer_abrir("file:///d-play.m3u8", TELA, 1, 0); correr(300);
  ok(legAt == -1, "3. no fundo: sem legenda");
  trailer_fechar();
  printf("%s\n", falhas ? "trailer_faixas: FALHOU" : "trailer_faixas: ok");
  return falhas != 0;
}
