// World (23/11): trailer do YouTube no botao, no Android (YtTrailer.kt numa WebView). trailer.c real com -DNV_ANDROID;
// as chamadas que iriam ao NvPlayer (video_yt_* e o ExoPlayer) ficam registradas e o estado da pagina e simulado.
// Comportamento conferido:
//   1. id do YouTube em tela cheia abre a WebView (video_yt_abrir), nunca o ExoPlayer, com som pelo ajuste;
//   2. fora da tela cheia (fundo/destaque) o YouTube nao abre nada;
//   3. a legenda: trailer dublado -> sem legenda; trailer em ingles -> legenda no idioma de legenda; "Legendado" -> sem;
//   4. tocando so depois do estado 1 da pagina; OK pausa (cmd 1) e continua (cmd 2), e a OSD le o tempo da pagina;
//   5. erro da pagina (-3) ou 15 s sem tocar: fecha (cmd 0) e marca falha (o detalhe recua para Apple/IMDb);
//   6. fim (0) e Voltar: fecham SEM falha;
//   7. uma URL (Apple) depois do YouTube fecha a WebView e vai ao ExoPlayer.
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
// Faixas do trailer (World 23/11): nenhuma neste teste.
int  video_n_audio(void) { return 0; }
int  video_n_legenda(void) { return 0; }
const VideoFaixa *video_audio(int i) { (void)i; return NULL; }
const VideoFaixa *video_legenda(int i) { (void)i; return NULL; }
int  video_audio_atual(void) { return -1; }
int  video_legenda_atual(void) { return -1; }
void video_escolher_audio(int i) { (void)i; }
void video_escolher_legenda(int i) { (void)i; }
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
static void zerar(void) { nTocar = nParar = nYtAbrir = nCmd = 0; ytEstado = -1; ytId[0] = ytLeg[0] = 0; }
static void correr(int ms) { for (int t = 0; t < ms; t += 16) { relogio += 16; trailer_atualizar(relogio); } }
static SDL_Event tecla(SDL_Keycode k) { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = k; return e; }

int main(void) {
  SDL_Event okk = tecla(SDLK_RETURN), volta = tecla(SDLK_ESCAPE);
  zerar();
  trailer_abrir(TY[0], TELA, 1, 1);
  ok(nYtAbrir == 1 && !strcmp(ytId, TY[0]) && nTocar == 0, "1. id do YouTube em tela cheia abre a WebView, nao o ExoPlayer");
  ok(!ytMudo && trailer_cheia(), "1. com som (ajuste) e em tela cheia");
  ok(ytLeg[0] == 0, "3. trailer dublado: sem legenda");
  correr(400);
  ok(!trailer_tocando(), "4. carregando: ainda nao 'tocando'");
  ytEstado = 1; pagPos = 30500; pagDur = 120000; correr(100);
  ok(trailer_tocando(), "4. estado 1 da pagina: tocando");
  trailer_evento(&okk);
  ok(trailer_pausado() && nCmd >= 1 && cmds[nCmd - 1] == 1, "4. OK pausa (cmd 1)");
  ok(ytPos_teste_ok(), "4. a OSD le o tempo da pagina (30,5 s de 120 s), nao o do ExoPlayer");
  trailer_evento(&okk);
  ok(!trailer_pausado() && cmds[nCmd - 1] == 2, "4. OK de novo continua (cmd 2)");
  trailer_evento(&volta);
  ok(!trailer_aberto() && cmds[nCmd - 1] == 0 && !trailer_falhou() && nParar == 0, "6. Voltar fecha a WebView (cmd 0), sem falha, sem mexer no ExoPlayer");

  zerar();
  trailer_abrir(TY[0], TELA, 1, 0);
  ok(nYtAbrir == 0 && nTocar == 0 && !trailer_aberto(), "2. fora da tela cheia o YouTube nao abre nada");
  zerar(); ytDisp = 0;
  trailer_abrir(TY[0], TELA, 1, 1);
  ok(nYtAbrir == 0 && !trailer_aberto(), "2. casca sem o player do YouTube: nao abre");
  ytDisp = 1;

  zerar();
  trailer_abrir(TY[2], TELA, 1, 1);
  ok(!strcmp(ytLeg, "pt"), "3. trailer em ingles: legenda no idioma de legenda (pt)");
  trailer_fechar();
  zerar();
  trailer_abrir(TY[1], TELA, 1, 1);
  ok(ytLeg[0] == 0, "3. trailer 'Legendado' (legenda na imagem): sem legenda do YouTube");
  ytEstado = 3; correr(200); ytEstado = -3; correr(50);
  ok(!trailer_aberto() && trailer_falhou() && cmds[nCmd - 1] == 0, "5. erro da pagina: fecha (cmd 0) e marca falha");

  zerar();
  trailer_abrir(TY[0], TELA, 1, 1);
  correr(14000);
  ok(trailer_aberto(), "5. ainda dentro do prazo de 15 s");
  correr(1500);
  ok(!trailer_aberto() && trailer_falhou(), "5. 15 s sem tocar: falha");

  zerar();
  trailer_abrir(TY[0], TELA, 1, 1);
  ytEstado = 1; correr(100); ytEstado = 0; correr(50);
  ok(!trailer_aberto() && !trailer_falhou(), "6. fim do video: fecha sem falha");

  zerar();
  trailer_abrir(TY[0], TELA, 1, 1);
  trailer_abrir("https://play-edge.itunes.apple.com/x.m3u8", TELA, 1, 1);
  ok(nCmd >= 1 && cmds[nCmd - 1] == 0 && nTocar == 1 && trailer_aberto(), "7. URL depois do YouTube: fecha a WebView e abre o ExoPlayer");
  trailer_fechar();
  ok(nParar == 1, "7. e fechar depois para o ExoPlayer");

  ok(trailer_eh_youtube("dQw4w9WgXcQ") && !trailer_eh_youtube("https://x/y.mp4") && !trailer_eh_youtube("a'b\"c<d>"),
     "id: so o formato de id do YouTube");
  printf("%s\n", falhas ? "trailer_youtube: FALHOU" : "trailer_youtube: ok");
  return falhas != 0;
}
