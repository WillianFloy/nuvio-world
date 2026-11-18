// 2.0.45: TRANSFORMACAO HOME -> COLLECTION. home.c + vertudo.c reais, GLES2 por software (logica, pixels e
// preenchimento submetido; NAO prova o tempo de GPU da Mali-G31). Fileiras reais: o colcache e semeado com as
// listas das 3 fontes de cada pasta (TMDB com chave falsa: sem rede, o colcache responde). Cartazes locais.
// Uso: vit_transforma [layout 0|1|2] [prefixo-de-capturas]
#include "../src/home.c"
#include "home_colfix.h"
#include "../src/colcache.h"
#include "../src/transicao.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
// CACHE FRIO DE VERDADE: os cartazes das pastas 4 e 7 sao FIFOs (named pipes). O fio de decodificacao do cache
// abre e FICA ESPERANDO, como numa rede lenta, ate o teste "entregar" o arquivo (liberar) — no ms que quiser.
static char fifoDir[900]; static int fifoFeito[2][10]; static int friaA = -1, friaB = -1;   // ids das pastas das colunas 4 e 7
static void fifoCaminho(int f, int k, char *out, size_t n) { snprintf(out, n, "%s/f%d_%02d.jpg", fifoDir, f, k); }
static int liberar(int f) {   // entrega o que o decode ja pediu; devolve quantos faltam
  int falta = 0, s = f == friaA ? 0 : 1;
  for (int k = 0; k < 10; k++) {   // entrega a QUALQUER leitor esperando (um novo decode do mesmo caminho tambem)
    char cam[1024]; fifoCaminho(f, k, cam, sizeof cam);
    int fd = open(cam, O_WRONLY | O_NONBLOCK);
    if (fd < 0) { falta++; continue; }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    char src[256]; snprintf(src, sizeof src, "deploy/app/art/poster/%02d.jpg", (f == friaA ? 20 : 30) + k);
    FILE *in = fopen(src, "rb"); char buf[65536]; size_t m;
    while (in && (m = fread(buf, 1, sizeof buf, in)) > 0) { if (write(fd, buf, m) < 0) break; }
    if (in) fclose(in); close(fd); fifoFeito[s][k]++; }
  return falta; }
static int homeDesenhos, bombear = 16, medePx, libA, libB;   // medePx: le os pixels (caro); libA/B: a "rede" das pastas frias ja responde
static double fillQ, fillHomeQ;
static unsigned char *px, *pxAnt; static double lumQ, difQ;
// SALTO = um quadro que muda muito mais que os vizinhos (o pop de uma imagem que aparece de uma vez). Movimento
// continuo (o cartao crescendo) muda bastante em TODO quadro e nao conta.
// Um fade que comeca rapido (ease-out) tambem muda bastante no 1o quadro, mas o seguinte continua mudando; no
// salto a mudanca se completa num quadro so. Por isso: o quadro contra o DOBRO do maior vizinho.
static double salto(const double *d, int n) { double m = 0; for (int i = 1; i + 1 < n; i++) { double v = d[i] - 2.0 * fmax(d[i - 1], d[i + 1]); if (v > m) m = v; } return m; }
// Um quadro do app (app.c): Home so se a Collection nao cobre; `bombear` = uploads por quadro (0 = cache frio parado).
static void quadro(void) {
  now += 16; SDL_PumpEvents(); tex_bombear(bombear); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
  gfx_sem_recorte(); glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
  if (!vertudo_aberta()) home_atualizar(1.f / 60, now);
  vertudo_atualizar(1.f / 60, now);
  if (!vertudo_cobre_tela()) { home_desenhar(now); homeDesenhos++; }
  fillHomeQ = gfx_fill;
  vertudo_desenhar(now); fillQ = gfx_fill; gfx_ambiente_descarregar();
  if (libA) liberar(friaA);
  if (libB) liberar(friaB);
  if (!medePx) return;
  // luminancia media e diferenca media para o quadro anterior (1/8 da resolucao): pegam tela preta e saltos
  if (!px) { px = malloc(1920 * 1080 * 4); pxAnt = calloc(1, 1920 * 1080 * 4); }
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, px);
  double l = 0, d = 0; int n = 0;
  for (int y = 0; y < 1080; y += 8) for (int x = 0; x < 1920; x += 8) { size_t i = ((size_t)y * 1920 + x) * 4;
    double v = 0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2]; l += v;
    d += (fabs((double)px[i] - pxAnt[i]) + fabs((double)px[i + 1] - pxAnt[i + 1]) + fabs((double)px[i + 2] - pxAnt[i + 2])) / 3.0; n++; }
  lumQ = l / n; difQ = d / n; memcpy(pxAnt, px, 1920 * 1080 * 4); }
static void quadros(int n) { for (int i = 0; i < n; i++) quadro(); }
static void ajuste(const char *k, int v) { char path[1024]; snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS")); FILE *f = fopen(path, "a"); assert(f);
  fprintf(f, "%s %d\n", k, v); fclose(f); ajustes_dir(getenv("NUVIO_DADOS")); }
static void esc(void) { SDL_Event e = {.type = SDL_KEYDOWN}; e.key.keysym.sym = SDLK_ESCAPE; vertudo_evento(&e); e.type = SDL_KEYUP; vertudo_evento(&e); }
static void ir_rapido(int col) { while (foco.coluna < col) { key(SDLK_RIGHT); frames(8); } while (foco.coluna > col) { key(SDLK_LEFT); frames(8); } }
static int ate_pasta(int rc, int col) { for (int i = 0; i < 12 && foco.fileira != rc; i++) { key(SDLK_DOWN); frames(40); }
  for (int i = 0; i < col; i++) { key(SDLK_RIGHT); frames(30); } frames(90); return foco.fileira == rc; }
// As 3 fontes de cada pasta no colcache, com cartazes locais distintos por pasta (cache frio de verdade por pasta).
static void semear(void) {
  snprintf(fifoDir, sizeof fifoDir, "%s/fifos", getenv("NUVIO_DADOS")); mkdir(fifoDir, 0700);
  for (int z = 0; z < 2; z++) for (int k = 0; k < 10; k++) { int f = z ? friaB : friaA; { char cam[1024]; fifoCaminho(f, k, cam, sizeof cam); unlink(cam); assert(mkfifo(cam, 0600) == 0); } }
  int cap = ajustes_col_itens_fileira(); if (cap > 42) cap = 42;
  static CatItem it[42];
  for (int f = 0; f < col_n(); f++) { const ColFolder *fo = col_folder(f);
    for (int r = 0; r < fo->nSources; r++) {
      memset(it, 0, sizeof it);
      for (int c = 0; c < cap; c++) { CatItem *ci = &it[c]; int k = (f * 7 + r * 5 + c) % 40;
        snprintf(ci->imdb, sizeof ci->imdb, "tt7%02d%02d%03d", f, r, c); snprintf(ci->tipo, sizeof ci->tipo, "movie");
        snprintf(ci->titulo, sizeof ci->titulo, "Titulo %d-%d-%d", f, r, c);
        // pastas 4 e 7 (os testes de cache frio) usam arquivos que nada mais na tela pediu (20-29 e 30-39)
        int fria = f == friaA || f == friaB, kk = k % 20;
        if (fria) fifoCaminho(f, c % 10, ci->poster, sizeof ci->poster);
        else snprintf(ci->poster, sizeof ci->poster, "deploy/app/art/poster/%02d.jpg", kk);
        if (fria) snprintf(ci->backdrop, sizeof ci->backdrop, "%s", ci->poster);
        else snprintf(ci->backdrop, sizeof ci->backdrop, "deploy/app/art/%02d.jpg", kk); }
      ColCacheMeta m = { cap, 0, 1, 0 };
      colcache_gravar(colcache_chave(&fo->sources[r], "", cap), it, &m); } } }
typedef struct { int quadros, homeAposCobrir, mono, vivo, vazios; double fillMax, lumMin, difMax, salto; float msRevela, msFim, pRevela; } Medida;
// Roda uma abertura ja disparada ate o fim (ou `max` quadros) medindo tudo.
static Medida medir(int max, double homeFill, const char *pre, const char *nome) {
  Medida m = {0}; m.mono = m.vivo = 1; m.lumMin = 1e9; m.msRevela = -1; medePx = 1;
  float pAnt = -1; int coberto = 0; unsigned long vaz0 = homeTesteCartaoVazio; static double ds[200]; int nd = 0;
  ds[nd++] = 0.0;   // o quadro antes do clique (parado): um salto no 1o quadro da transformacao tambem conta
  for (int q = 0; q < max; q++) {
    int hd = homeDesenhos; quadro();
    float p, r, rev, ms; int fase = vertudo_tr_estado(&p, &r, &rev, &ms);
    if (vertudo_cobre_tela()) coberto = 1; else if (coberto && homeDesenhos > hd) m.homeAposCobrir = 1;
    if (fase != TR_PARADO || m.msFim > 0) { if (fase != TR_PARADO && p < pAnt - 1e-5f) m.mono = 0; pAnt = p; if (fase != TR_PARADO || ms > m.msFim) m.msFim = ms; if (rev >= 0) { m.msRevela = rev; } }
    if (fillQ > m.fillMax) m.fillMax = fillQ;
    if (lumQ < m.lumMin) m.lumMin = lumQ;
    if (difQ > m.difMax) m.difMax = difQ;
    if (nd < 200) ds[nd++] = difQ;
    if (getenv("NV_TRACE")) printf("[q] %s %02d fase=%d p=%.3f r=%.2f home=%d cobre=%d fill=%.2f (home %.2f) lum=%.1f dif=%.1f\n", nome, q, fase, p, r, homeDesenhos > hd, vertudo_cobre_tela(), fillQ, fillHomeQ, lumQ, difQ);
    if (pre && (q % 6 == 0)) { char f[64]; snprintf(f, sizeof f, "%s-%02d", nome, q); shot(pre, f); }
    m.quadros = q + 1;
    if (fase == TR_PARADO && q > 2) break;
  }
  m.vazios = (int)(homeTesteCartaoVazio - vaz0); m.salto = salto(ds, nd); medePx = 0;
  printf("[medida] %s: quadros=%d revela=%.0f ms fim=%.0f ms fill max %.2f (Home parada %.2f) lum min %.1f dif max %.1f salto max %.1f vazios=%d\n",
         nome, m.quadros, m.msRevela, m.msFim, m.fillMax, homeFill, m.lumMin, m.difMax, m.salto, m.vazios);
  return m;
}
static void fechar(void) { vertudo_fechar_seco(); quadros(5); frames(40); }
// O fixture da 2.0.43 tem uma fonte por pasta; aqui cada pasta tem 3 (uma fileira por fonte na Vitrine, lidas do
// colcache). A 1a pasta do grupo "Themes" tem logo (mesma logo do fixture).
static int fixture3(void) {
  static char json[32768]; int n = 0;
  n += snprintf(json + n, sizeof json - n, "{\"collections\":[{\"id\":\"cx\",\"title\":\"Themes\",\"folders\":[");
  for (int i = 0; i < 12; i++) {
    n += snprintf(json + n, sizeof json - n,
      "%s{\"id\":\"f%d\",\"title\":\"Pasta %d\",\"coverImageUrl\":\"deploy/app/art/poster/%02d.jpg\",\"heroBackdropUrl\":\"deploy/app/art/%02d.jpg\"%s,\"sources\":[",
      i ? "," : "", i, i + 1, i % 8, (i * 3) % 20, i % 3 == 0 ? ",\"titleLogoUrl\":\"deploy/app/art/marcas/logo-novo-horizontal.png\"" : "");
    for (int r = 0; r < 3; r++)
      n += snprintf(json + n, sizeof json - n, "%s{\"provider\":\"tmdb\",\"tmdbSourceType\":\"DISCOVER\",\"mediaType\":\"MOVIE\",\"sortBy\":\"popularity.desc.%d.%d\",\"title\":\"Lista %d-%d\"}", r ? "," : "", i, r, i, r);
    n += snprintf(json + n, sizeof json - n, "]}");
  }
  snprintf(json + n, sizeof json - n, "]}]}");
  assert(colfileiras_receber(json) > 0); colfileiras_sincronizar(); home_ir_topo(); frames(120);
  for (int q = 0; q < nFileiras; q++) if (fileiras[q].tipo == FILEIRA_CATALOGOS) return q;
  return -1;
}
int main(int argc, char **argv) {
  int layout = argc > 1 ? atoi(argv[1]) : 0; const char *pre = argc > 2 ? argv[2] : NULL;
  setvbuf(stdout, NULL, _IOLBF, 0);
  signal(SIGPIPE, SIG_IGN);   // um leitor que fecha o FIFO cedo (le so a assinatura) nao derruba o teste
  desc_tmdb_definir("0123456789abcdef0123456789abcdef");
  (void)colfix_iniciar(layout, 12, 1);
  int rc = fixture3();
  ajuste("colEntradaLocal", 0);
  friaA = fileiras[rc].folders[4]; friaB = fileiras[rc].folders[7];
  semear();
  ok(rc >= 0 && ate_pasta(rc, 1), "o foco esta numa pasta das Collections da Home");
  if (rc < 0) { puts("vit_transforma: FALHOU"); return 1; }
  medePx = 1; double homeFill = 0; for (int q = 0; q < 8; q++) { quadro(); homeFill += fillQ; } homeFill /= 8; medePx = 0;
  double lumHome = lumQ;
  // ---- D2: pre-carga por permanencia (so colcache, cache ocioso)
  { int f = fileiras[rc].folders[foco.coluna]; (void)f; frames(30);
    int pend = vertudo_prever_pendentes(), q = 0; while (vertudo_prever_pendentes() > 0 && q < 600) { quadro(); q++; }
    printf("[prever] pendentes no repouso=%d, fila vazia depois de %d quadros\n", pend, q);
    ok(pend > 0, "D2: parado na pasta, a Collection enfileira os cartazes que o colcache ja tem (sem rede)");
    ok(vertudo_prever_pendentes() == 0, "D2: a fila anda (um pedido por quadro, so com o cache ocioso) e nunca trava"); }
  // ---- 1. Quente: tudo em cache
  GfxRect orig; ok(home_ancora_foco(&orig) && orig.w > 8 && orig.h > 8, "a Home publica o retangulo real do cartao focado");
  medePx = 1; quadro(); medePx = 0;   // o buffer de comparacao = a Home parada no instante do clique
  key(SDLK_RETURN);
  float p0; GfxRect r0; ok(vertudo_aberta() && vertudo_voo_ativo(&p0, &r0), "OK abre a Collection e a transformacao comeca no clique");
  printf("[continuidade] origem (%.1f,%.1f,%.1f,%.1f) cartao (%.1f,%.1f,%.1f,%.1f)\n", orig.x, orig.y, orig.w, orig.h, r0.x, r0.y, r0.w, r0.h);
  { float e = gfx_escala_txt(); r0.x *= e; r0.y *= e; r0.w *= e; r0.h *= e; }   // a Vitrine desenha na escala de texto
  ok(fabsf(r0.x - orig.x) < 1 && fabsf(r0.y - orig.y) < 1 && fabsf(r0.w - orig.w) < 1 && fabsf(r0.h - orig.h) < 1,
     "continuidade: o cartao parte exatamente de onde estava na Home");
  homeDesenhos = 0;
  Medida q = medir(90, homeFill, pre, "quente");
  ok(q.msRevela >= TR_SAIDA_MS - 17 && q.msRevela <= TR_SAIDA_MS + 34, "quente: o conteudo pronto revela assim que o cartao chega (~300 ms)");
  ok(q.msFim >= 590 && q.msFim <= 640, "quente: ~600 ms no total");
  ok(q.mono, "o progresso so avanca");
  ok(!q.homeAposCobrir, "depois que o destino cobre, a Home nao e mais desenhada");
  ok(q.fillMax <= homeFill + 1.5, "preenchimento por quadro <= Home parada + 1,5 tela (2.0.44: 7,2-8,8 telas no pico)");
  printf("[medida] luminancia da Home parada %.1f\n", lumHome);
  ok(q.lumMin >= 0.6 * lumHome, "nenhum quadro escuro/morto no caminho (luminancia >= 60 por cento da Home)");
  ok(q.salto < 8.0, "nenhuma troca brusca: nenhum quadro muda muito mais que os vizinhos (pico isolado < 8/255)");
  ok(q.vazios == 0, "nenhum cartao vazio aparece na revelacao");
  ok(vertudo_cobre_tela() && !vertudo_voo_ativo(NULL, NULL), "no fim nada fica animando e a Collection cobre a tela");
  if (pre) shot(pre, "quente-fim");
  // ---- 2. Frio: os cartazes chegam aos 500 ms -> a duracao se adapta
  fechar(); ir_rapido(4); quadros(2);
  printf("[frio] coluna=%d pasta=%d friaA=%d\n", foco.coluna, fileiras[rc].folders[foco.coluna], friaA);
  key(SDLK_RETURN);
  { Medida m = {0}; m.mono = 1; float pAnt = -1, vMinCruz = 1, pCruz0 = -1; int liberado = 0, nq = 0, fase = 0; double fmax = 0;
    unsigned long vaz0 = homeTesteCartaoVazio;
    for (; nq < 90; nq++) { float p, r, rev, ms; quadro(); fase = vertudo_tr_estado(&p, &r, &rev, &ms);
      if (ms >= 400) { liberado = 1; libA = 1; } if (liberado) SDL_Delay(25);   // a "rede" responde a partir dos 400 ms
      if (fase == TR_CRUZEIRO) { if (pCruz0 < 0) pCruz0 = p; }
      if (fase == TR_PARADO) { m.msFim = ms; }
      else { if (p < pAnt - 1e-5f) m.mono = 0; if (fase == TR_CRUZEIRO && p > pAnt && pAnt >= 0 && p - pAnt < vMinCruz) vMinCruz = p - pAnt; pAnt = p; m.msFim = ms; m.msRevela = rev; }
      if (fillQ > fmax) fmax = fillQ;
      if (fase == TR_PARADO) break; }
    printf("[medida] frio: revela=%.0f fim=%.0f fill max %.2f menor passo no cruzeiro %.5f vazios=%lu\n", m.msRevela, m.msFim, fmax, vMinCruz, homeTesteCartaoVazio - vaz0);
    ok(m.msRevela > 400 && m.msRevela <= TR_TETO_MS - TR_REVELA_MS + 17, "frio: a revelacao espera os cartazes (so depois que eles chegam, aos 400 ms)");
    ok(m.msFim - m.msRevela >= TR_REVELA_MS - 17 && m.msFim - m.msRevela <= TR_REVELA_MS + 34, "frio: a revelacao em si dura o mesmo (a espera e o cruzeiro, nao uma animacao a mais)");
    ok(m.msFim <= TR_TETO_MS + 17, "frio: nunca passa do teto de 1000 ms");
    ok(m.mono && vMinCruz > 0, "frio: no cruzeiro a animacao desacelera, mas nunca para");
    ok(homeTesteCartaoVazio - vaz0 == 0, "frio: nenhum cartao vazio aparece"); }
  // ---- 3. Nunca chega: teto, e o que falta fica fora (entra com fade quando chegar)
  fechar(); ir_rapido(7); quadros(2);
  medePx = 1; quadro(); medePx = 0;
  key(SDLK_RETURN);
  { Medida m = medir(80, homeFill, NULL, "teto");
    ok(m.msRevela >= TR_TETO_MS - TR_REVELA_MS - 1 && m.msRevela <= TR_TETO_MS - TR_REVELA_MS + 17, "teto: a revelacao comeca em 700 ms");
    ok(m.msFim <= TR_TETO_MS + 17, "teto: fim em 1000 ms");
    ok(m.vazios == 0, "teto: cartaz que nao chegou NAO aparece vazio");
    unsigned long v0 = homeTesteCartaoVazio; double difMax = 0, dd[60];
    libB = 1; medePx = 1;
    for (int i = 0; i < 60; i++) { SDL_Delay(25); quadro(); dd[i] = difQ; if (difQ > difMax) difMax = difQ;
      if (pre && getenv("NV_CHEGADA")) { char f[32]; snprintf(f, sizeof f, "chegada-%02d", i); shot(pre, f); printf("[chegada] %02d dif=%.1f\n", i, difQ); } }
    medePx = 0; int entregues = 0; for (int k = 0; k < 10; k++) entregues += fifoFeito[1][k] > 0;
    printf("[medida] chegada depois do teto: %d arquivos entregues, dif max %.1f salto max %.1f vazios=%lu\n", entregues, difMax, salto(dd, 60), homeTesteCartaoVazio - v0);
    ok(entregues >= 5, "teto: os cartazes de fato chegaram depois (o teste mede algo)");
    ok(homeTesteCartaoVazio - v0 == 0 && salto(dd, 60) < 8.0, "teto: o que chega depois (cartazes e a arte do titulo) entra com fade, sem salto"); }
  // ---- 4. Voltar no meio: sem salto, a Home volta
  fechar(); ate_pasta(rc, 1); bombear = 16;
  medePx = 1; key(SDLK_RETURN); quadros(10);   // ja mede: o 1o quadro do Voltar compara com o anterior real
  ok(vertudo_voo_ativo(NULL, NULL), "reabrir: a transformacao recomeca");
  esc();
  { double difMax = 0, dd[60]; int volta = 0; medePx = 1; for (int i = 0; i < 60; i++) { int hd = homeDesenhos; quadro(); dd[i] = difQ; if (difQ > difMax) difMax = difQ; if (homeDesenhos > hd) volta = 1; }
    medePx = 0; printf("[medida] voltar no meio: dif max %.1f salto max %.1f\n", difMax, salto(dd, 60));
    ok(volta && !vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL), "Voltar no meio: a Collection fecha e a Home volta");
    ok(salto(dd, 60) < 8.0, "Voltar no meio: sem salto (tudo apaga junto)"); }
  // ---- 5. Recuo da Home: so escala e opacidade
  { float tx; ok(vertudo_home_recuo(&tx) == 0.0f && tx == 1.0f, "fora da transformacao a Home nao recua"); }
  // ---- 6. Ajustes: desligado / reduzidas / efeitos minimos = instantaneo
  fechar(); ajuste("colEntradaLocal", 1); key(SDLK_RETURN);
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL) && vertudo_cobre_tela(), "ajuste DESLIGADO: abre direto");
  fechar(); ajuste("colEntradaLocal", 0); gfx_definir_efeitos_minimos(1); key(SDLK_RETURN);
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL), "efeitos minimos: abre direto");
  fechar(); gfx_definir_efeitos_minimos(0); ajuste("animacoes", 1); key(SDLK_RETURN);
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL), "animacoes reduzidas: abre direto");
  fechar(); ajuste("animacoes", 0);
  // ---- 7. Sem cartao de origem (Spotlight/menu): a revelacao da 2.0.43, sem transformacao
  { const ColFolder *fo = col_folder(fileiras[rc].folders[0]); vertudo_colecao(fo); }
  ok(vertudo_aberta() && !vertudo_voo_ativo(NULL, NULL) && fabsf(vertudo_entrada_ms() - HT_VOO_SEM_CARTAO_MS) < 1, "origem sem cartao: so a revelacao, como antes");
  puts(falhas ? "vit_transforma: FALHOU" : "vit_transforma: ok");
  fflush(stdout); _exit(falhas ? 1 : 0); }   // sem tex_encerrar: um fio de decode pode estar parado num FIFO
