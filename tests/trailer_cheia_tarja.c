// World (23/11): no Android o trailer em TELA CHEIA vai ENCAIXADO (com tarja), nunca numa janela maior que a tela.
// Log da TCL (Homem de Ferro, Apple 1180x500): "recorte 146,0 888x500 -> janela -316,0 2551x1080" nas 4 opcoes de
// Proporcao; a TCL nao respeita o que sai da tela e o 2,36:1 saia esticado na vertical. trailer.c real com -DNV_ANDROID,
// video simulado (as chamadas que iriam ao NvPlayer ficam registradas). Comportamento:
//   1. tela cheia: so janelas lisas (encaixe), nenhuma video_janela_fonte, nenhuma reaplicacao de recorte;
//   2. no fundo na tela inteira (esperar para tocar): idem;
//   3. destino menor (banner do Padrao): o recorte "cover" continua (controle).
#include "trailer.h"
#include "video.h"
#include <stdio.h>
#include <string.h>
static int falhas, nLisa, nFonte, nReaplica, tocando, pronto = 1;
static int lisaW, lisaH;
static Uint32 relogio = 1000;
int video_janela_pronta(void) { return 1; }
#define SDL_GetTicks() (relogio)
#undef __APPLE__
#include "../src/trailer.c"
#undef SDL_GetTicks
int  video_tocar(const char *u) { (void)u; return 1; }
int  video_pronto(void) { return pronto; }
int  video_largura(void) { return 1180; }
int  video_altura(void) { return 500; }
int  video_tocando(void) { return tocando; }
int  video_ativo(void) { return 1; }
void video_volume(int v) { (void)v; }
void video_janela(int x, int y, int w, int h) { (void)x; (void)y; nLisa++; lisaW = w; lisaH = h; }
void video_janela_fonte(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
  printf("  recorte pedido %d,%d %dx%d -> %d,%d %dx%d\n", sx, sy, sw, sh, dx, dy, dw, dh); nFonte++; }
void video_recorte_reaplicar(void) { nReaplica++; }
float ajustes_trailer_zoom(void) { return 1.34f; }
int  video_recorte_fonte(void) { return 1; }   // como o video_android.c
void video_parar(void) {}
int  video_iniciar_auto(void) { return 1; }
int  video_conflito_recurso(void) { return 0; }
int  video_falhou(void) { return 0; }
int  video_terminou(void) { return 0; }
void video_bombear(void) {}
void video_pausar(int p) { (void)p; }
double video_pos(void) { return 0; }
double video_duracao(void) { return 0; }
double video_buffer_fim(void) { return 0; }
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
// O trailer do YouTube (World 23/11) nao entra aqui: so URLs, que vao ao ExoPlayer.
int  video_yt_disponivel(void) { return 0; }
void video_yt_abrir(const char *id, int mudo, const char *leg) { (void)id; (void)mudo; (void)leg; }
void video_yt_cmd(int c) { (void)c; }
int  video_yt_estado(void) { return -1; }
int  video_yt_pos_ms(void) { return 0; }
int  video_yt_dur_ms(void) { return 0; }
int  extras_n_trailers(void) { return 0; }
const char *extras_trailer_yt(int i) { (void)i; return ""; }
int  extras_trailer_idioma_preferido(int i) { (void)i; return 0; }
int  extras_trailer_legendado(int i) { (void)i; return 0; }
void extras_trailer_audio_iso(char *d) { d[0] = 0; }
const char *ling_legenda(void) { return ""; }
void ling_normalizar(const char *in, char *out, unsigned cap) { (void)in; if (cap) out[0] = 0; }
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
static void correr(int ms) { for (int t = 0; t < ms; t += 16) { relogio += 16; nativoAplicar(); } }
static void cena(int modoCheia, GfxRect tela) {
  nLisa = nFonte = nReaplica = 0; tocando = 0;
  trailer_abrir("file:///trailer-play.m3u8", tela, 1, modoCheia);
  correr(200);              // pronto, ainda nao tocando: era aqui o "quadro inteiro" esticado
  tocando = 1; correr(9000); // tocando: o recorte saia ~800 ms depois e era reaplicado por 6 s
  trailer_fechar();
}
int main(void) {
  // ajustes_trailer_zoom() = 1,34 ("Zoom cinema", o padrao)
  GfxRect inteira = { 0, 0, NV_TELA_W, NV_TELA_H }, banner = { 0, 0, NV_TELA_W, 528 };
  printf("tela cheia:\n"); cena(1, inteira);
  ok(nLisa >= 1 && lisaW == (int)NV_TELA_W && lisaH == (int)NV_TELA_H, "tela cheia: janela lisa na tela inteira (o NvPlayer encaixa, com tarja)");
  ok(nFonte == 0, "tela cheia: nenhuma janela maior que a tela (sem recorte)");
  ok(nReaplica == 0, "tela cheia: nenhuma reaplicacao de recorte");
  printf("no fundo, tela inteira (esperar para tocar na pagina do titulo):\n"); cena(0, inteira);
  ok(nFonte == 0 && nReaplica == 0, "no fundo na tela inteira: encaixado, sem recorte (o log da TCL pedia o mesmo -316,0 2551x1080)");
  printf("no fundo, banner 1920x528 (destaque do Padrao):\n"); cena(0, banner);
  ok(nFonte >= 1, "destino menor que a tela: o recorte cover continua (controle)");
  printf("%s\n", falhas ? "trailer_cheia_tarja: FALHOU" : "trailer_cheia_tarja: ok");
  return falhas != 0;
}
