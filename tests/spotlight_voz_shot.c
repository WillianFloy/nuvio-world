// CAPTURAS DO SPOTLIGHT (spotlight.h), sem rede e sem interacao.
//
// O que cada foto prova:
//   -barra       sem pesquisa recente nenhuma: SO a barra (lupa, campo), sem
//                corpo — o desenho do dono, "so a barra e so crescer";
//   -entrada     a barra no meio da animacao de entrada;
//   -vazio       campo vazio com recentes: a lista curta logo abaixo;
//   -crescendo   logo depois de "the": o corpo no meio da mola, crescendo;
//   -teclado-app OK no campo onde nao ha teclado do sistema (LG/Samsung/Mac):
//                o teclado do app a esquerda do corpo, resultados a direita;
//   -android-ime o caminho do Android (sistexto em modo de teste): o teclado
//                do sistema "aberto", o texto vindo dele no campo;
//   -android-voz o microfone ouvindo, com a parcial no campo e o anel do som;
//   -android-semvoz  sem reconhecedor: o aviso e o teclado do sistema;
//   -digitado    "the" digitado: melhor resultado em destaque e Titulos, a
//                lista mudando a cada letra;
//   -foco        o foco na lista (direita a partir da ultima coluna): a linha
//                do melhor resultado acesa na cor do tema e o "OK Abrir";
//   -foco2       duas linhas abaixo;
//   -pessoa      a pessoa (elenco com id do TMDB) em foco, sem foto;
//   -colecao     uma pasta de colecao achada pelo nome;
//   -nada        um termo sem resultado: o aviso, nao uma lista vazia;
//   -vidro       a mesma busca com a Interface de vidro ligada;
//   -pessoa-tmdb "keanu": pessoas do /search/person do TMDB (resposta de
//                tests/fixtures, sem rede) com "Conhecido por";
//   -canal       "brasil": dois canais da Live TV de um guia carregado de um
//                addon falso local (tests/spotlight_canais_servidor.py);
//   -pessoa-tmdb-vivo  so com NUVIO_TMDB_DIR (pasta com tmdb.txt): o mesmo
//                pedido no TMDB de verdade, com as fotos de perfil.
// E confere, sem olho: digitar filtra, OK no titulo pede SPOT_TITULO com o
// indice certo e fecha, OK na pessoa pede SPOT_PESSOA com o tmdb, Voltar
// fecha, a busca feita entra nas recentes; a pessoa do TMDB so e pedida
// quando o texto para (um pedido para "keanu" inteiro, nenhum por letra), a
// URL leva o idioma, e OK nela pede SPOT_PESSOA com o titulo conhecido.
#include "spotlight.h"
#include "nvtmp.h"
#include "buscasrec.h"
#include "dados.h"
#include "ajustes.h"
#include "ajustes_ux.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "catalogo.h"
#include "colecoes.h"
#include "spotpessoa.h"
#include "guia.h"
#include "addons.h"
#include "descoberta.h"
#include "sistexto.h"
#include "shot_arte.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Tipos de linha de spotlight.c (enum interno L_*).
enum { T_AVISO = 1, T_TOPO = 2, T_PESSOA = 4, T_COLECAO = 5, T_RECENTE = 9, T_AJUSTE = 11 };

static SDL_Window *janela;
static char ultimaUrl[800];

// O TMDB de mentira: guarda a URL e devolve a resposta gravada.
static char *tmdbFalso(const char *url) {
  FILE *f = fopen("tests/fixtures/tmdb_search_person.json", "rb");
  char *b;
  long n;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", url);
  if (!f) return NULL;
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
  b = malloc((size_t)n + 1);
  if (b && fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); b = NULL; }
  if (b) b[n] = 0;
  fclose(f);
  return b;
}
static char dirArte[600];

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  spot_evento(&e);
  e.type = SDL_KEYUP;
  spot_evento(&e);
}
static void digitar(const char *s) { for (; *s; s++) tecla(*s == ' ' ? SDLK_SPACE : (SDL_Keycode)*s); }
// Da barra, BAIXO desce aos resultados (o teclado do app nao esta aberto).
static void paraLista(void) { tecla(SDLK_DOWN); }
// Voltar desfaz em ordem (lista -> campo -> limpa -> fecha): aperta ate fechar.
static void fechar(void) { int i; for (i = 0; i < 5 && spot_aberto(); i++) tecla(SDLK_ESCAPE); }

// O "fundo": a arte de tela cheia de tests/shot_arte.h, para o vidro das
// duas ilhas ser julgado sobre uma imagem de verdade (o mockup "ilha" tela 6
// e assim); com NUVIO_SHOT_ARTE=- as fileiras de posteres, como uma home.
static void fundo(void) {
  int r, c;
  char cam[700];
  const char *sa = getenv("NUVIO_SHOT_ARTE");
  if (!(sa && !strcmp(sa, "-"))) { shot_arte_desenhar(0.0f); return; }
  gfx_cor((GfxRect){ 0, 0, 1920, 1080 }, 0, 0.07f, 0.07f, 0.08f, 1.0f);
  for (r = 0; r < 3; r++)
    for (c = 0; c < 8; c++) {
      GfxRect p = { 104.0f + c * 232.0f, 120.0f + r * 330.0f, 208.0f, 312.0f };
      GLuint t;
      snprintf(cam, sizeof cam, "%s/poster/%02d.jpg", dirArte, (r * 8 + c) % 40);
      t = tex_obter_larg(cam, p.w);
      if (t) { gfx_tex_aspect_atual = tex_aspecto(cam);
               gfx_rect(p, t, GFX_CARD, 0, 0, 0, 0.06f, 0, 0, 0, 1.0f);
               gfx_tex_aspect_atual = 0; }
      else gfx_cor(p, 0.06f, 0.2f, 0.2f, 0.22f, 1.0f);
    }
}

static void quadro(void) {
  Uint32 agora = SDL_GetTicks();
  SDL_PumpEvents();
  txt_novo_quadro();
  tex_novo_quadro();
  tex_bombear(16);
  gfx_novo_quadro();
  spot_atualizar(1.0f / 60.0f, agora);
  glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  fundo();
  spot_desenhar(agora, 0);
  SDL_GL_SwapWindow(janela);
}

static void capturaEm(const char *saida, const char *nome, Uint32 ms) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  char arq[700];
  SDL_Surface *s;
  int y;
  Uint32 t0 = SDL_GetTicks();
  assert(pix);
  while (SDL_GetTicks() - t0 < ms) quadro();
  quadro();
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  assert(s);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  snprintf(arq, sizeof arq, "%s-%s.bmp", saida, nome);
  assert(SDL_SaveBMP(s, arq) == 0);
  SDL_FreeSurface(s);
  free(pix);
  printf("captura: %s\n", arq);
}
static void captura(const char *saida, const char *nome) { capturaEm(saida, nome, 900); }
// N quadros de 1/60 s: a mola anda pelo dt, nao pelo relogio (sem vsync o
// laco roda a centenas de quadros por segundo).
static void capturaQuadros(const char *saida, const char *nome, int n) {
  while (n-- > 1) quadro();
  capturaEm(saida, nome, 0);
}

static int achar(int tipo, const char *texto) {
  int i;
  for (i = 0; i < spot_n_linhas(); i++)
    if (spot_linha_tipo(i) == tipo && (!texto || strstr(spot_linha_texto(i), texto))) return i;
  return -1;
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : nvt("/nuvio-spotlight");
  const char *dir = getenv("NUVIO_DADOS");
  SDL_GLContext gl;
  SpotPedido p;
  char cwd[400];
  if (!dir || !*dir) return 2;
  assert(getcwd(cwd, sizeof cwd));
  snprintf(dirArte, sizeof dirArte, "%s/deploy/app/art", cwd);
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: Spotlight", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(160);
  gfx_icones_dir("deploy/app/art");
  dados_iniciar(dir);
  assert(!strcmp(dados_dir(), dir));
  ajustes_iniciar();
  // NUVIO_SHOT_PT=1: interface em portugues (a lingua dos mockups), para a
  // comparacao lado a lado medir o mesmo texto.
  if (getenv("NUVIO_SHOT_PT")) {
    char aj[700];
    FILE *f;
    snprintf(aj, sizeof aj, "%s/ajustes.txt", dados_dir());
    if ((f = fopen(aj, "w"))) { fprintf(f, "idioma 0\n"); fclose(f); }
    ajustes_dir(dados_dir());
  }
  ajustes_definir_vidro(0);   // as capturas comuns sao do solido; -vidro* liga

  // Catalogo do pacote em duas fileiras "de addon" (base preenchida: e o que
  // faz a primeira virar "Em alta").
  cat_carregar("deploy/app/art");
  { static CatFileira fl[2];
    int n = cat_n();
    snprintf(fl[0].chave, sizeof fl[0].chave, "shot.a");
    snprintf(fl[0].titulo, sizeof fl[0].titulo, "Top 10 de hoje - Filme");
    snprintf(fl[0].tipo, sizeof fl[0].tipo, "movie");
    snprintf(fl[0].base, sizeof fl[0].base, "https://addon.exemplo");
    snprintf(fl[0].catId, sizeof fl[0].catId, "top");
    fl[0].ini = 0; fl[0].n = n / 2;
    fl[1] = fl[0];
    snprintf(fl[1].chave, sizeof fl[1].chave, "shot.b");
    snprintf(fl[1].titulo, sizeof fl[1].titulo, "Lançamentos - Série");
    snprintf(fl[1].catId, sizeof fl[1].catId, "novos");
    fl[1].ini = n / 2; fl[1].n = n - n / 2;
    cat_republicar_fileiras(fl, 2); }
  // Elenco com id do TMDB num titulo (o pacote nao traz id): e o que a
  // pessoa precisa para abrir a filmografia.
  { int i;
    for (i = 0; i < cat_n(); i++) {
      const CatItem *ci = cat_item(i);
      if (ci && strstr(ci->titulo, "Prestige")) {
        static CatItem novo;
        novo = *ci;
        novo.nElenco = 2;
        snprintf(novo.elenco[0].nome, sizeof novo.elenco[0].nome, "Hugh Jackman");
        novo.elenco[0].tmdb = 6968;
        snprintf(novo.elenco[1].nome, sizeof novo.elenco[1].nome, "Christian Bale");
        novo.elenco[1].tmdb = 3894;
        novo.nota = 85;
        cat_atualizar_item(i, &novo);
      }
    } }
  { static ColFolder f[1];
    memset(f, 0, sizeof f);
    snprintf(f[0].id, sizeof f[0].id, "shot-col");
    snprintf(f[0].title, sizeof f[0].title, "The Dark Knight Trilogy");
    snprintf(f[0].group, sizeof f[0].group, "Sagas");
    snprintf(f[0].cover, sizeof f[0].cover, "%s/02.jpg", dirArte);
    f[0].extra = 1;
    col_extra_definir(f, 1); }

  st_teste_ligar(1);
  // VOZ (23/11): onda na barra + cartao "Ouvindo..." ate chegar o texto final.
  spot_abrir(1);
  assert(st_estado() == ST_OUVINDO);
  st_teste_evento("Souvindo");
  st_teste_evento("R10");
  captura(saida, "voz-1-silencio");
  st_teste_evento("R85");
  capturaQuadros(saida, "voz-2-falando", 20);
  st_teste_evento("Pfilmes de ac");
  captura(saida, "voz-3-parcial");
  st_teste_evento("V filmes de acao ");
  captura(saida, "voz-4-resultado");
  fechar();
  puts("capturas da voz: ok");
  return 0;
}
