// A TELA DOS LOGS DE DIAGNOSTICO NA TV, de verdade (registro_diag.inc + diaglog
// + logrede), pelas teclas do controle: OK em "Iniciar logs", Voltar (a pilula
// "Gravando logs" fica no alto), OK em "Finalizar logs". Captura cada fase em
// BMP; tests/diaglog_shot.py decodifica o QR da captura e abre o endereco.
// Sem o fio do stdout do Android: o caminho alternativo (o nuvio.log a partir
// do tamanho do inicio) e o que roda aqui. Uso: diaglog_shot <saida-prefixo>
#include "registro.h"
#include "diaglog.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "ajustes.h"
#include "shot_arte.h"
#include "dados.h"
#include "perfis.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned char *pix;
static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  registro_diag_evento(&e);
  e.type = SDL_KEYUP;
  registro_diag_evento(&e);
}
static void quadros(int n, SDL_Window *w, const char *salvar) {
  while (n-- > 0) {
    SDL_Delay(16);
    gfx_novo_quadro(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(6);
    diaglog_tick();
    glClearColor(0.05f, 0.05f, 0.06f, 1); glClear(GL_COLOR_BUFFER_BIT);
    shot_arte_desenhar(.30f);
    if (registro_diag_aberto()) registro_diag_desenhar();
    registro_diag_indicador();
    if (!n && salvar) {
      SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      int y;
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      for (y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      SDL_SaveBMP(s, salvar); SDL_FreeSurface(s);
      fprintf(stderr, "SHOT %s\n", salvar);
    }
    SDL_GL_SwapWindow(w);
  }
}
extern int ajustes_teste_op_por_chave(const char *chave);
extern void ajustes_abrir_opcao(int op);
// A tela de Ajustes de verdade, com o foco na linha "Logs de diagnostico".
static void quadrosAjustes(int n, SDL_Window *w, const char *salvar) {
  while (n-- > 0) {
    SDL_Delay(16);
    gfx_novo_quadro(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(6);
    diaglog_tick();
    ajustes_atualizar(1.0f / 60.0f, SDL_GetTicks());
    glClearColor(0.05f, 0.05f, 0.06f, 1); glClear(GL_COLOR_BUFFER_BIT);
    ajustes_desenhar(SDL_GetTicks());
    if (!n && salvar) {
      SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      int y;
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      for (y = 0; y < 1080; y++) memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      SDL_SaveBMP(s, salvar); SDL_FreeSurface(s);
      fprintf(stderr, "SHOT %s\n", salvar);
    }
    SDL_GL_SwapWindow(w);
  }
}
static const char *est(void) {
  DiagInfo i; diaglog_info(&i);
  return i.estado == DIAG_GRAVANDO ? "gravando" : i.estado == DIAG_SALVANDO ? "salvando" : i.estado == DIAG_PRONTO ? "pronto" : "parado";
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/diag-tv";
  const char *dir = getenv("NUVIO_DADOS"), *log = getenv("NUVIO_LOG");
  char nome[700], caminho[700];
  FILE *f;
  SDL_Window *w;
  int i;
  assert(dir && log);
  // O stdout no "nuvio.log", como main.c faz na TV.
  assert(freopen(log, "w", stdout));
  setvbuf(stdout, NULL, _IOLBF, 0);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  f = fopen(caminho, "w"); assert(f);
  fputs("idioma 0\nselected_theme 2\n", f);
  shot_arte_material(f);
  fclose(f);
  dados_iniciar("deploy/app/art");
  ajustes_dir(dir);
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("diag", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar()); assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  pix = malloc(1920 * 1080 * 4); assert(pix);

  printf("[tv] linha ANTES da sessao apikey=SEGREDOANTES123\n");
  // Pelos Ajustes: a linha "Logs de diagnostico" e o OK do controle abrem o painel.
  ajustes_abrir_opcao(ajustes_teste_op_por_chave("logRedeLocal"));   // como a busca dos Ajustes: vale na abertura
  ajustes_iniciar();
  snprintf(nome, sizeof nome, "%s-0-ajustes.bmp", saida); quadrosAjustes(40, w, nome);
  { SDL_Event e; memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN; ajustes_evento(&e); }
  fprintf(stderr, "PAINEL-PELOS-AJUSTES %d\n", registro_diag_aberto());
  snprintf(nome, sizeof nome, "%s-1-parado.bmp", saida); quadrosAjustes(20, w, nome);
  registro_diag_abrir();
  tecla(SDLK_RETURN);   // foco inicial: "Iniciar logs"
  fprintf(stderr, "ESTADO %s\n", est());
  for (i = 0; i < 40; i++) {
    printf("[ritmo] tela=60.00Hz ref=16.67ms(tela) | apresentados=108 fps=36.0 | fora-do-prazo=72(67%%) hist(1/2/3/4+)=36/72/0/0 n=%d\n", i);
    quadros(2, w, NULL);
  }
  snprintf(nome, sizeof nome, "%s-2-gravando.bmp", saida); quadros(60, w, nome);
  fprintf(stderr, "URL %s\n", diaglog_url());
  tecla(SDLK_ESCAPE);   // Voltar: fecha o painel, a gravacao continua
  fprintf(stderr, "ESTADO-depois-de-voltar %s aberto=%d\n", est(), registro_diag_aberto());
  snprintf(nome, sizeof nome, "%s-3-indicador-gravando.bmp", saida); quadros(30, w, nome);
  registro_diag_abrir();
  tecla(SDLK_RETURN);   // foco inicial: "Finalizar logs"
  for (i = 0; i < 400 && strcmp(est(), "pronto"); i++) quadros(1, w, NULL);
  fprintf(stderr, "ESTADO %s\n", est());
  snprintf(nome, sizeof nome, "%s-4-pronto.bmp", saida); quadros(90, w, nome);
  fprintf(stderr, "PRONTO %s\n", diaglog_url());
  tecla(SDLK_ESCAPE);
  snprintf(nome, sizeof nome, "%s-5-indicador-pronto.bmp", saida); quadros(30, w, nome);
  // Mantem o servidor vivo para o script ler o QR e abrir o endereco.
  fprintf(stderr, "ESPERANDO\n");
  {
    char l[16];
    while (fgets(l, sizeof l, stdin) && strncmp(l, "sair", 4)) quadros(1, w, NULL);
  }
  return 0;
}
