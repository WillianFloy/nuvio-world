// RITMO DE QUADROS por MODO DE DESEMPENHO (World, 20/11).
//
// O QUE O HARDWARE DEIXA. O painel da TCL e 60 Hz e o laco desenha com vsync
// (SDL_GL_SetSwapInterval(1)). 45 quadros num painel de 60 Hz nao tem cadencia
// uniforme (3 imagens a cada 4 varreduras: 16,7/16,7/33,3 ms), entao os modos
// sao 60 e 30. No ANDROID o eglSwapInterval(2) NAO da o vsync alternado: o
// Surface da plataforma prende o intervalo em [0, 1] (ANativeWindow
// maxSwapInterval = 1). O 30 estavel sai de outro jeito:
//   1. EGL_ANDROID_presentation_time: cada quadro diz QUANDO deve aparecer, num
//      grid de 2 periodos medidos (swapint.h) -- o compositor segura o quadro
//      ate la, e a cadencia fica 33,3 ms mesmo com a CPU adiantada;
//   2. ANativeWindow_setFrameRate(30, FIXED_SOURCE) (API 30+): avisa o
//      compositor que o conteudo e 30 (ele pode trocar o modo do painel);
//   3. a CPU espera a vez do proximo quadro (nao produz quadro descartado).
// Fora do Android tenta o SDL_GL_SetSwapInterval(2) de verdade.
// Tudo e resolvido em tempo de execucao e vai ao log ([ritmo]): e o log da TV
// que prova se o modo vale la. Player com UI: o ritmo dele (player_pacing.h).
#include "ritmo.h"
#include "ajustes.h"
#include <stdio.h>
#ifdef NV_ANDROID
int  android_ritmo_frame_rate(SDL_Window *win, float fps);   // android.c
int  android_ritmo_apresentar(double alvoMs);               // android.c (relogio monotonic, ms)
double android_ritmo_agora_ms(void);
#endif

static int aplicado = -1, metodo;   // metodo: 0 vsync, 1 present-time, 2 swap 2, 3 so CPU
static double ultAlvo, ultSwapFim;

double ritmo_proximo_alvo(double ultimoAlvo, double agora, double periodo) {
  double alvo = ultimoAlvo + 2.0 * periodo;
  if (ultimoAlvo <= 0.0 || alvo < agora + 0.5 * periodo) {
    // Atrasou (quadro pesado) ou comecou agora: o proximo slot de 2 periodos.
    alvo = agora + periodo;
  }
  return alvo;
}

static double agoraMs(void) {
#ifdef NV_ANDROID
  return android_ritmo_agora_ms();
#else
  return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency();
#endif
}

static void aplicar(SDL_Window *win, int modo) {
  int rc = 0;
  metodo = 0;
#ifdef NV_ANDROID
  rc = android_ritmo_frame_rate(win, modo == 2 ? 30.0f : 0.0f);
  if (modo == 2) metodo = android_ritmo_apresentar(-1.0) ? 1 : 3;   // -1 = so pergunta
#else
  (void)win;
#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
  rc = SDL_GL_SetSwapInterval(modo == 2 ? 2 : 1);
  if (modo == 2) metodo = rc == 0 ? 2 : 3;
  if (modo == 2 && rc != 0) SDL_GL_SetSwapInterval(1);
#endif
#endif
  ultAlvo = 0.0;
  printf("[ritmo] modo=%s metodo=%s rc=%d\n", modo == 0 ? "Desempenho" : modo == 2 ? "Qualidade" : "Equilibrado",
         ritmo_metodo(), rc);
  fflush(stdout);
}

void ritmo_antes_swap(SDL_Window *win, int playerUi, double periodoMs) {
  int modo = ajustes_fps_modo();
  if (modo != aplicado) { aplicar(win, modo); aplicado = modo; }
  if (modo != 2 || playerUi) return;
  ultAlvo = ritmo_proximo_alvo(ultAlvo, agoraMs(), periodoMs > 1.0 ? periodoMs : 16.7);
#ifdef NV_ANDROID
  if (metodo == 1) android_ritmo_apresentar(ultAlvo);
#endif
}

unsigned ritmo_depois_swap(int playerUi, double periodoMs) {
  double agora, comeca, p = periodoMs > 1.0 ? periodoMs : 16.7;
  if (aplicado != 2 || playerUi || metodo == 2) return 0;   // swap 2 ja segura sozinho
  agora = agoraMs();
  ultSwapFim = agora;
  // O proximo quadro comeca 1 periodo antes do alvo dele (alvo atual + 2p).
  comeca = ultAlvo + p;
  if (comeca > agora + 0.5) {
    unsigned ms = (unsigned)(comeca - agora);
    if (ms > 40u) ms = 40u;
    SDL_Delay(ms);
    return ms;
  }
  return 0;
}

int ritmo_modo_aplicado(void) { return aplicado; }
void ritmo_invalidar(void) { aplicado = -1; }
const char *ritmo_metodo(void) {
  return metodo == 1 ? "present-time" : metodo == 2 ? "swap-interval-2" : metodo == 3 ? "cpu" : "vsync";
}
