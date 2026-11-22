// World 22/11: o cabecalho da colecao e o menu do topo TROCAM de lugar (o nome apaga enquanto a barra abre por cima).
// Teste de comportamento com o vertudo.c e o menu.c de verdade (GLES2 por software); so a faixa de cima e medida.
#define desc_vertudo_n shot_vt_n
#define desc_vertudo_item shot_vt_item
#include "../src/vertudo.c"
#include "nvtmp.h"
#undef desc_vertudo_n
#undef desc_vertudo_item
#include "corviva.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static CatItem semente[122];
static int nSemente;
int shot_vt_n(void) { return nSemente; }
int shot_vt_item(int i, CatItem *dst) {
  if (i < 0 || i >= nSemente) return 0;
  if (dst) *dst = semente[i];
  return 1;
}

static const struct { const char *t; int ano; } FILMES[] = {
  { "A Ameaça Fantasma", 1999 }, { "Ataque dos Clones", 2002 },
  { "A Vingança dos Sith", 2005 }, { "Uma Nova Esperança", 1977 },
  { "O Império Contra-Ataca", 1980 }, { "O Retorno de Jedi", 1983 },
  { "O Despertar da Força", 2015 }, { "Os Últimos Jedi", 2017 },
  { "A Ascensão Skywalker", 2019 }, { "Rogue One", 2016 },
  { "Solo", 2018 }, { "Clone Wars", 2008 },
  { "O Farol", 1994 }, { "Mata Fechada", 1991 }, { "Noite de Verão", 2011 },
  { "Rio Acima", 1987 }, { "Cartas de Inverno", 1975 }, { "Sal e Luz", 2021 },
};
#define NF (int)(sizeof FILMES / sizeof *FILMES)

static SDL_Window *janela;
static void captura(const char *nome, int quadros) {
  int i;
  for (i = 0; i < quadros; i++) {
    static Uint32 agora = 5000; agora += 16;
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(8);
    gfx_novo_quadro();
    glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    vertudo_atualizar(1.0f / 60.0f, agora);
    vertudo_desenhar(agora);
    // NV_MENU=1: a barra por cima, como app.c (rail fixa, menu do topo).
    { static int mi; if (!mi) mi = menu_iniciar(); menu_atualizar(1.0f / 60.0f, agora); }
    if (0) {
      unsigned char *pix = malloc(1920 * 1080 * 4);
      SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      int y;
      assert(pix && s);
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      for (y = 0; y < 1080; y++)
        memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      assert(SDL_SaveBMP(s, nome) == 0);
      SDL_FreeSurface(s);
      free(pix);
      printf("captura: %s (fill=%.2f rects=%d)\n", nome, gfx_fill, gfx_n_rect);
    }
    SDL_GL_SwapWindow(janela);
  }
}

// Fotografa a faixa de cima (a esquerda), onde mora o cabecalho da colecao, e conta os pixels que ESCURECERAM
// de uma foto para a outra (o banner e igual nas duas: so o texto do cabecalho muda).
#define BW 1100
#define BH 300
static void foto(unsigned char *dst) {
  static unsigned char px[1920 * 1080 * 4];
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, px);
  for (int y = 0; y < BH; y++) memcpy(dst + y * BW * 4, px + ((1079 - y) * 1920 + 100) * 4, BW * 4); }
static int escureceu(const unsigned char *antes, const unsigned char *depois) {
  int n = 0;
  for (int i = 0; i < BW * BH; i++) {
    const unsigned char *a = antes + i * 4, *b = depois + i * 4;
    if ((a[0] + a[1] + a[2]) - (b[0] + b[1] + b[2]) > 150) n++; }
  return n; }
static int mudou(const unsigned char *a, const unsigned char *b) {
  int n = 0;
  for (int i = 0; i < BW * BH; i++) {
    const unsigned char *p = a + i * 4, *q = b + i * 4;
    if (abs(p[0] - q[0]) + abs(p[1] - q[1]) + abs(p[2] - q[2]) > 30) n++; }
  return n; }
static unsigned char fA[BW * BH * 4], fB[BW * BH * 4], fC[BW * BH * 4], fD[BW * BH * 4], fK[BW * BH * 4], fE[BW * BH * 4];
static void config(const char *extra) {
  char cam[700]; snprintf(cam, sizeof cam, "%s/ajustes.txt", dados_dir());
  FILE *a = fopen(cam, "w"); assert(a); fprintf(a, "idioma 0\nhomeLayoutLocal 1\n%s\n", extra); fclose(a);
  ajustes_dir(dados_dir()); }
static int falhas;
static void ok(int c, const char *msg) { printf("%s: %s\n", c ? "PASS" : "FAIL", msg); falhas += !c; }
static ColFolder pasta;
static ColSource fontes[3];

int main(void) {
  char cam[700];
  SDL_GLContext gl;
  FILE *a;
  int i, L;

  dados_iniciar("deploy/app/art");
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dados_dir());
  a = fopen(cam, "w"); assert(a);
  fprintf(a, "idioma 0\n");
  if (getenv("NUVIO_SHOT_FONTE")) fprintf(a, "fonteInterface %d\n", atoi(getenv("NUVIO_SHOT_FONTE")));
  if (getenv("NV_AJ")) fprintf(a, "%s\n", getenv("NV_AJ"));   // ex.: "modernLandscapePostersEnabled 0"
  fclose(a);
  ajustes_dir(dados_dir());

  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  janela = SDL_CreateWindow("Nuvio: exibicoes da colecao", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela); assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(160);
  gfx_borrao_iniciar(480, 270);
  gfx_icones_dir("deploy/app/art");

  nSemente = NF;
  for (i = 0; i < NF; i++) {
    CatItem *c = &semente[i];
    int k = (i * 7 + 3) % 40;
    snprintf(c->imdb, sizeof c->imdb, "tt80%05d", i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    snprintf(c->titulo, sizeof c->titulo, "%s", FILMES[i].t);
    snprintf(c->genero, sizeof c->genero, "Ficção científica · Aventura");
    snprintf(c->meta, sizeof c->meta, "%d · 2 h 1%d min", FILMES[i].ano, i % 10);
    snprintf(c->sinopse, sizeof c->sinopse,
             "Uma sinopse de verdade tem duas ou tres linhas e ocupa o painel "
             "lateral sem empurrar a nota para fora da tela.");
    snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", k);
    snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", k);
    if (k < 10) snprintf(c->logo, sizeof c->logo, "deploy/app/art/logo/%02d.png", k);
    c->nota = 70 + i % 20;
    // Ficha do painel: classificacao, direcao e elenco em parte dos titulos.
    snprintf(c->classificacao, sizeof c->classificacao, "%s", i % 3 ? "14" : "L");
    if (i % 2 == 1) {
      snprintf(c->direcao, sizeof c->direcao, "Marta Albuquerque");
      snprintf(c->elenco[0].nome, sizeof c->elenco[0].nome, "Ana Ribeiro");
      snprintf(c->elenco[1].nome, sizeof c->elenco[1].nome, "Caio Monteiro");
      snprintf(c->elenco[2].nome, sizeof c->elenco[2].nome, "Lia Fontes");
      c->nElenco = 3;
    }
    // Saga: os tres primeiros lancamentos vistos, o quarto pela metade.
    if (FILMES[i].ano < 1984) c->progresso = 100;
    if (FILMES[i].ano == 1999) c->progresso = 46;
  }

  pasta.sources = fontes;
  pasta.nSources = 2;
  snprintf(pasta.id, sizeof pasta.id, "starwars");
  snprintf(pasta.title, sizeof pasta.title, "Star Wars");
  snprintf(pasta.group, sizeof pasta.group, "Film Collections");
  snprintf(fontes[0].title, sizeof fontes[0].title, "Filmes");
  snprintf(fontes[0].type, sizeof fontes[0].type, "movie");
  snprintf(fontes[1].title, sizeof fontes[1].title, "Séries");
  snprintf(fontes[1].type, sizeof fontes[1].type, "series");
  collection = &pasta;
  snprintf(titulo, sizeof titulo, "%s", pasta.title);
  aberta = 1; anim = 1.0f; source = 0; tabFocus = 0;

  // VITRINE: tres abas viram tres fileiras (semeadas direto; a fila de abas
  // precisa de rede). Topo, e rolada ate a terceira fileira.
  pasta.nSources = 3; tabFocus = 0;
  snprintf(pasta.group, sizeof pasta.group, "Film Collections");
  snprintf(pasta.title, sizeof pasta.title, "Star Wars");
  snprintf(titulo, sizeof titulo, "%s", pasta.title);
  snprintf(fontes[2].title, sizeof fontes[2].title, "Animações");
  snprintf(fontes[2].type, sizeof fontes[2].type, "movie");
  lay = VT_LAY_VITRINE; layEscolha = VT_LAY_VITRINE; celLay = -1;
  vitIniciar();
  { static const char *NOMES[3] = { "Filmes", "Séries", "Animações" };
    for (i = 0; i < 3; i++) {
      int k;
      snprintf(vitLin[i].nome, sizeof vitLin[i].nome, "%s", NOMES[i]);
      vitLin[i].estado = VIT_PRONTA; vitLin[i].n = vitLin[i].brutoN = 12; vitLin[i].semFonte = 0;
      // sem addon na harness: a fonte ja "conferida" como desligada nao zera a fileira semeada
      vitFontes[i] = collection->sources[i]; vitDesligada[i] = 1;
      snprintf(vitBases[i], sizeof vitBases[i], "%s", baseDaFonte(&collection->sources[i]));
      for (k = 0; k < 12; k++) vitLinAloc(i)[k] = semente[(i * 5 + k) % NF];
    }
    vitCarga = vitN; vitMexeu = 1; vitFR = 0; vitFC = 2; }

  // A. Sem menu no topo (rail): o cabecalho no lugar de sempre.
  config("menuTopoLocal 0");
  captura(NULL, 90); foto(fA);
  // B. Menu no topo, recolhido: o MESMO quadro (antes o cabecalho descia para baixo da barra).
  config("menuTopoLocal 1\nmenuTopoFixoLocal 0");
  captura(NULL, 90); foto(fB);
  int dAB = mudou(fA, fB);
  printf("[medida] rail x menu recolhido: %d pixels diferentes\n", dAB);
  ok(dAB < 40, "menu no topo recolhido: o cabecalho NAO desce (a faixa de cima e igual a da rail)");
  ok(menu_topo_abertura() < 0.01f, "recolhido: a abertura do menu e 0");
  // C. Menu aberto por cima: o nome da colecao APAGA (a barra ocupa o lugar dele), gradualmente.
  menu_abrir();
  int sobe = 1, ant = 0, meio = 0;
  for (int k = 0; k < 40; k++) { captura(NULL, 1); foto(fK); int n = escureceu(fB, fK);
    if (k == 6) meio = n;
    if (n + 25 < ant) sobe = 0;   // so apaga enquanto abre
    ant = n; }
  captura(NULL, 60); foto(fC); int nC = escureceu(fB, fC);
  printf("[medida] cabecalho apagado: meio=%d fim=%d pixels (abertura=%.2f)\n", meio, nC, menu_topo_abertura());
  ok(menu_topo_abertura() > 0.99f, "aberto: a abertura do menu chega a 1");
  ok(nC > 800, "menu aberto: o nome e a faixa do cabecalho apagam (centenas de pixels de texto)");
  ok(meio > 0 && meio < nC, "enquanto abre o cabecalho apaga aos poucos (nao some num corte)");
  ok(sobe, "enquanto abre o cabecalho so apaga");
  // D. Fecha: o cabecalho volta inteiro, no mesmo lugar.
  menu_fechar(); captura(NULL, 120); foto(fD);
  ok(menu_topo_abertura() < 0.01f && mudou(fB, fD) < 40, "menu recolhido de novo: o cabecalho volta igual ao de antes");
  // E. Sem menu no topo (rail) a abertura do menu nao mexe no cabecalho.
  config("menuTopoLocal 0"); menu_abrir(); captura(NULL, 90); foto(fE);
  ok(menu_topo_abertura() < 0.01f && mudou(fA, fE) < 40, "rail: abrir o menu lateral nao apaga o cabecalho da colecao");
  // F. (item 4) Na Padrao o menu do topo DESCE do alto enquanto acende (antes so mudava o alfa: "aparece de repente").
  { static unsigned char px[1920 * 1080 * 4];
    float cy[40]; int nb[40], i, c;
    for (c = 0; c < 2; c++) {
      config("menuTopoLocal 1\nmenuTopoFixoLocal 0"); anim_politica_reduzida = c;
      menu_fechar(); menu_pilula_mostrar(-1.0f);
      for (i = 0; i < 200; i++) menu_atualizar(1.0f / 60.0f, 80000 + i * 16);   // assenta o que a cena anterior deixou aberto
      for (i = 0; i < 40; i++) {
        static Uint32 ag = 90000; ag += 16;
        txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
        glClearColor(0.051f, 0.051f, 0.051f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        if (i == 4) menu_pilula_mostrar(1.0f);
        menu_atualizar(1.0f / 60.0f, ag); menu_desenhar(ag);
        glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, px);
        double sy = 0; int n = 0;
        for (int y = 0; y < 220; y++) for (int x = 0; x < 1920; x++) {
          const unsigned char *p = px + ((1079 - y) * 1920 + x) * 4;
          if (p[0] + p[1] + p[2] > 3 * 70) { sy += y; n++; } }
        nb[i] = n; cy[i] = n ? (float)(sy / n) : -1.0f; }
      int p0 = -1; for (i = 0; i < 40; i++) if (nb[i] > 1500) { p0 = i; break; }
      printf("[medida] n final=%d; y:", nb[39]); for (i = 0; i < 40; i += 3) printf(" %d:%d/%.0f", i, nb[i], cy[i]); printf("\n"); printf("[medida] menu do topo (%s): acende no quadro %d, y medio %.1f -> %.1f -> %.1f\n", c ? "animacoes reduzidas" : "animacoes ligadas",
             p0, p0 >= 0 ? cy[p0] : -1.0f, p0 >= 0 ? cy[p0 + 3 < 40 ? p0 + 3 : 39] : -1.0f, cy[39]);
      if (!c) {
        ok(nb[3] < 10, "antes de pedir, o menu do topo nao aparece");
        ok(p0 >= 0 && cy[39] - cy[p0] > 10.0f, "animacoes ligadas: o menu chega DESCENDO (comeca mais alto do que termina)");
        int mono = 1; for (i = p0 + 1; i < 40; i++) if (cy[i] + 1.5f < cy[i - 1]) mono = 0;
        ok(mono && fabsf(cy[39] - cy[38]) < 0.5f, "a descida e continua e assenta no lugar");
      } else {
        ok(p0 >= 0 && fabsf(cy[39] - cy[p0]) < 2.0f, "animacoes reduzidas: o menu nao se move (so acende)");
      } }
    anim_politica_reduzida = 0; }
  printf("colecao_cabecalho_menu: %s\n", falhas ? "FALHOU" : "ok");
  return falhas ? 1 : 0;
}
