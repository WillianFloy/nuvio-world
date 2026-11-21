// Ver diaglog.h para o porque, o fluxo e as regras.
#include "diaglog.h"
#include "logrede.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
// No navegador nao ha porta nem arquivo: nada de sessao.
int  diaglog_iniciar(int pt) { (void)pt; return 0; }
void diaglog_finalizar(void) {}
void diaglog_tick(void) {}
void diaglog_info(DiagInfo *o) { memset(o, 0, sizeof *o); }
const char *diaglog_url(void) { return ""; }
int  diaglog_cabecalho_pendente(void) { return 0; }
void diaglog_bytes(const char *b, size_t n) { (void)b; (void)n; }
void diaglog_tap_registrar(void) {}
#else

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static DiagEstado estado;
static char codigo[16], marcaIni[80], marcaFim[80], aparelho[96], nomeDl[96];
static char brutoArq[640], finalArq[640];
static int pt, geracao, cabPend, limpando, fimConfirmado, cortado, tapAtivo;
static long iniMs, fimMs, prontoMs, prazoMs, duracaoMs;
static long brutoBytes, brutoLinhas, finalBytes, finalLinhas;
static long logOfs;
// O separador de linhas do fio do stdout. fase: 0 sem sessao, 1 esperando a
// marca de inicio, 2 gravando, 3 a marca de fim passou (bruto fechado).
static int fase, fdBruto = -1;
static char lb[16384];
static size_t ln;

static long agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (long)t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

static const char *pasta(void) {
  static char p[600];
  const char *e = getenv("NUVIO_DIAG_DIR"), *d = getenv("NUVIO_DADOS");
  if (e && *e) snprintf(p, sizeof p, "%s", e);
  else if (d && *d) snprintf(p, sizeof p, "%s/diag", d);
  else snprintf(p, sizeof p, "/tmp/nuvio-diag");
  mkdir(p, 0700);
  return p;
}

// Sessao nova nunca mistura: os arquivos da anterior (bruto e limpo) saem. Um
// download da anterior ainda em andamento tem o arquivo aberto e termina.
static void limparPasta(const char *dir) {
  DIR *d = opendir(dir);
  struct dirent *e;
  char c[900];
  if (!d) return;
  while ((e = readdir(d)) != NULL) {
    if (strncmp(e->d_name, "sessao-", 7) && strncmp(e->d_name, "nuvio-log-", 10)) continue;
    snprintf(c, sizeof c, "%s/%s", dir, e->d_name);
    unlink(c);
  }
  closedir(d);
}

static void gerarCodigo(char *dst, int n) {
  static const char A[] = "abcdefghjkmnpqrstuvwxyz23456789";
  unsigned char r[16];
  int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC), i, ok = 0;
  if (fd >= 0) { ok = read(fd, r, (size_t)n) == n; close(fd); }
  if (!ok) { srand((unsigned)(time(NULL) ^ (unsigned)agoraMs())); for (i = 0; i < n; i++) r[i] = (unsigned char)rand(); }
  for (i = 0; i < n; i++) dst[i] = A[r[i] % (sizeof A - 1)];
  dst[n] = 0;
}

// --- o bruto da sessao (sob a trava) ------------------------------------------
static void gravarBruto(const char *s, size_t k) {
  if (fdBruto < 0) return;
  if (brutoBytes + (long)k + 1 > DIAG_MAX_BRUTO) {
    if (!cortado) {
      static const char nota[] = "[diag] limite de 64 MB da sessao atingido: o resto nao foi gravado\n";
      if (write(fdBruto, nota, sizeof nota - 1) > 0) brutoBytes += (long)sizeof nota - 1;
      cortado = 1;
    }
    return;
  }
  while (k) {
    ssize_t w = write(fdBruto, s, k);
    if (w < 0 && errno == EINTR) continue;
    if (w <= 0) return;
    s += w; k -= (size_t)w; brutoBytes += (long)w;
  }
}
static void fecharBruto(void) {
  if (fdBruto < 0) return;
  fsync(fdBruto);
  close(fdBruto);
  fdBruto = -1;
}
static void linhaCompleta(void) {
  lb[ln] = 0;
  if (fase == 1 && strstr(lb, marcaIni)) fase = 2;
  if (fase == 2) {
    int fim = strstr(lb, marcaFim) != NULL;
    if (!cortado || fim) {
      gravarBruto(lb, ln);
      gravarBruto("\n", 1);
      brutoLinhas++;
    }
    if (fim) { fase = 3; fecharBruto(); }
  }
  ln = 0;
}
static void processar(const char *b, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    if (b[i] == '\n') { linhaCompleta(); continue; }
    if (b[i] == '\r') continue;
    if (ln + 1 >= sizeof lb) linhaCompleta();   // linha gigante: sai em pedacos
    lb[ln++] = b[i];
  }
}

void diaglog_bytes(const char *b, size_t n) {
  if (!__atomic_load_n(&fase, __ATOMIC_ACQUIRE) || !n) return;
  pthread_mutex_lock(&m);
  if (fase == 1 || fase == 2) processar(b, n);
  pthread_mutex_unlock(&m);
}
void diaglog_tap_registrar(void) { pthread_mutex_lock(&m); tapAtivo = 1; pthread_mutex_unlock(&m); }

// Sem o fio do stdout: o nuvio.log a partir do tamanho do inicio, pelo mesmo separador.
static void lerDoLog(void) {
  const char *log = getenv("NUVIO_LOG");
  FILE *f;
  long ofs;
  char buf[65536];
  size_t k;
  struct stat st;
  if (!log || !*log || (f = fopen(log, "rb")) == NULL) return;
  pthread_mutex_lock(&m);
  ofs = logOfs;
  pthread_mutex_unlock(&m);
  if (fstat(fileno(f), &st) == 0 && (long)st.st_size < ofs) ofs = 0;   // o log recomecou: a marca de inicio separa
  fseek(f, ofs, SEEK_SET);
  while ((k = fread(buf, 1, sizeof buf, f)) > 0) {
    pthread_mutex_lock(&m);
    if (fase == 1 || fase == 2) processar(buf, k);
    pthread_mutex_unlock(&m);
  }
  fclose(f);
}

// --- a limpeza (fio proprio: nao trava o desenho) --------------------------------
typedef struct { int ger; char bruto[640], fim[640], cod[16], apar[96]; long dur; int confirmado, cort; } Limpeza;
static void *limpar(void *u) {
  Limpeza *L = (Limpeza *)u;
  char tmp[700];
  FILE *in, *out;
  char *linha = NULL;
  size_t cap = 0;
  ssize_t k;
  long bytes = 0, linhas = 0;
  snprintf(tmp, sizeof tmp, "%s.parcial", L->fim);
  in = fopen(L->bruto, "rb");
  out = fopen(tmp, "wb");
  if (out) {
    char cab[700];
    time_t q = time(NULL);
    struct tm tmv;
    localtime_r(&q, &tmv);
    snprintf(cab, sizeof cab,
             "# Nuvio - sessao de diagnostico %s | %s | gravada por %.1f s | arquivo gerado em %04d-%02d-%02d %02d:%02d:%02d\n"
             "# Sem senhas nem chaves: URLs cortadas no host, apikey/token/senha/Authorization trocados por \"\xe2\x80\xa6\".%s\n",
             L->cod, L->apar[0] ? L->apar : "?", L->dur / 1000.0, tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
             L->confirmado ? "" : "\n# AVISO: a marca de fim nao chegou no prazo; o fim da sessao pode estar incompleto.");
    fputs(cab, out); bytes += (long)strlen(cab); linhas += L->confirmado ? 2 : 3;
  }
  if (in && out) {
    while ((k = getline(&linha, &cap, in)) > 0) {
      if (linha[k - 1] == '\n') linha[--k] = 0;
      logrede_limpar(linha);
      k = (ssize_t)strlen(linha);
      fwrite(linha, 1, (size_t)k, out); fputc('\n', out);
      bytes += k + 1; linhas++;
    }
  }
  free(linha);
  if (in) fclose(in);
  if (out) { fflush(out); fsync(fileno(out)); fclose(out); rename(tmp, L->fim); }
  unlink(L->bruto);
  pthread_mutex_lock(&m);
  if (L->ger == geracao) {
    limpando = 0;
    if (out) {
      estado = DIAG_PRONTO;
      prontoMs = agoraMs();
      prazoMs = prontoMs + DIAG_JANELA_MS;
      finalBytes = bytes; finalLinhas = linhas;
    } else { estado = DIAG_PARADO; codigo[0] = 0; }
  }
  pthread_mutex_unlock(&m);
  if (out) printf("[diag] sessao %s pronta: %ld linhas, %ld KB; download aberto por %ld s\n", L->cod, linhas, (bytes + 1023) / 1024, DIAG_JANELA_MS / 1000);
  else printf("[diag] sessao %s: nao foi possivel gravar o arquivo (%s)\n", L->cod, strerror(errno));
  fflush(stdout);
  free(L);
  return NULL;
}

// --- API -------------------------------------------------------------------
int diaglog_iniciar(int ptBr) {
  const char *dir, *log = getenv("NUVIO_LOG"), *tv = getenv("NUVIO_TV_INFO");
  char c[16], quando[32];
  struct stat st;
  time_t q = time(NULL);
  struct tm tmv;
  int fd;
  pthread_mutex_lock(&m);
  if (estado == DIAG_GRAVANDO || estado == DIAG_SALVANDO) { pthread_mutex_unlock(&m); return 0; }
  fecharBruto();
  dir = pasta();
  limparPasta(dir);
  gerarCodigo(c, 6);
  localtime_r(&q, &tmv);
  snprintf(quando, sizeof quando, "%04d%02d%02d-%02d%02d%02d", tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  snprintf(brutoArq, sizeof brutoArq, "%s/sessao-%s.bruto", dir, c);
  snprintf(nomeDl, sizeof nomeDl, "nuvio-log-%s.txt", quando);
  snprintf(finalArq, sizeof finalArq, "%.500s/%.90s", dir, nomeDl);
  fd = open(brutoArq, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND | O_CLOEXEC, 0600);
  if (fd < 0) {
    pthread_mutex_unlock(&m);
    printf("[diag] nao foi possivel criar a sessao em %s (%s)\n", dir, strerror(errno));
    fflush(stdout);
    return 0;
  }
  fdBruto = fd;
  snprintf(codigo, sizeof codigo, "%s", c);
  snprintf(marcaIni, sizeof marcaIni, "[diag] ==== inicio da sessao %s ====", c);
  snprintf(marcaFim, sizeof marcaFim, "[diag] ==== fim da sessao %s ====", c);
  { size_t i = 0; aparelho[0] = 0;
    if (tv) { while (tv[i] && tv[i] != '|' && i + 1 < sizeof aparelho) { aparelho[i] = tv[i]; i++; } aparelho[i] = 0; } }
  pt = ptBr;
  geracao++;
  limpando = 0; cortado = 0; fimConfirmado = 0;
  brutoBytes = brutoLinhas = finalBytes = finalLinhas = 0;
  logOfs = (log && *log && stat(log, &st) == 0) ? (long)st.st_size : 0;
  ln = 0;
  __atomic_store_n(&fase, 1, __ATOMIC_RELEASE);
  estado = DIAG_GRAVANDO;
  iniMs = agoraMs(); fimMs = prontoMs = prazoMs = duracaoMs = 0;
  cabPend = 1;
  pthread_mutex_unlock(&m);
  fflush(stdout);
  printf("%s\n", marcaIni);
  fflush(stdout);
  return 1;
}

void diaglog_finalizar(void) {
  int tap;
  long dur;
  pthread_mutex_lock(&m);
  if (estado != DIAG_GRAVANDO) { pthread_mutex_unlock(&m); return; }
  estado = DIAG_SALVANDO;
  fimMs = agoraMs();
  dur = duracaoMs = fimMs - iniMs;
  tap = tapAtivo;
  pthread_mutex_unlock(&m);
  printf("[diag] finalizada pelo usuario: %.1f s gravados\n", dur / 1000.0);
  printf("%s\n", marcaFim);
  fflush(stdout);
  if (!tap) lerDoLog();
}

void diaglog_tick(void) {
  long t = agoraMs();
  int servir, lancar = 0, expirou = 0;
  Limpeza *L = NULL;
  pthread_mutex_lock(&m);
  if (estado == DIAG_SALVANDO && !limpando && (fase == 3 || t - fimMs > DIAG_SALVAR_PRAZO_MS)) {
    L = (Limpeza *)calloc(1, sizeof *L);
    if (L) {
      fimConfirmado = fase == 3;
      fecharBruto();
      __atomic_store_n(&fase, 0, __ATOMIC_RELEASE);
      ln = 0;
      limpando = 1; lancar = 1;
      L->ger = geracao; L->dur = duracaoMs; L->confirmado = fimConfirmado; L->cort = cortado;
      snprintf(L->bruto, sizeof L->bruto, "%s", brutoArq);
      snprintf(L->fim, sizeof L->fim, "%s", finalArq);
      snprintf(L->cod, sizeof L->cod, "%s", codigo);
      snprintf(L->apar, sizeof L->apar, "%s", aparelho);
    }
  }
  if (estado == DIAG_PRONTO && t >= prazoMs) {
    estado = DIAG_PARADO;
    expirou = 1;
  }
  servir = estado != DIAG_PARADO;
  pthread_mutex_unlock(&m);
  if (lancar) {
    pthread_t th;
    pthread_attr_t at;
    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &at, limpar, L) != 0) limpar(L);
    pthread_attr_destroy(&at);
  }
  // Servidor primeiro, depois o codigo: nenhuma conexao nova chega depois do
  // prazo; a que ja estava baixando segue no fio dela ate o fim.
  logrede_servidor(servir);
  if (expirou) {
    char c[16];
    pthread_mutex_lock(&m);
    snprintf(c, sizeof c, "%s", codigo);
    if (estado == DIAG_PARADO) codigo[0] = 0;
    pthread_mutex_unlock(&m);
    printf("[diag] sessao %s: prazo de %ld s encerrado, endereco desligado\n", c, DIAG_JANELA_MS / 1000);
    fflush(stdout);
  }
}

void diaglog_info(DiagInfo *o) {
  long t = agoraMs();
  memset(o, 0, sizeof *o);
  pthread_mutex_lock(&m);
  o->estado = estado;
  snprintf(o->codigo, sizeof o->codigo, "%s", estado == DIAG_PARADO ? "" : codigo);
  o->pt = pt;
  snprintf(o->aparelho, sizeof o->aparelho, "%s", aparelho);
  o->cortado = cortado;
  o->fimConfirmado = fimConfirmado;
  if (estado == DIAG_GRAVANDO) { o->decorridoMs = t - iniMs; o->bytes = brutoBytes; o->linhas = brutoLinhas; }
  else if (estado == DIAG_SALVANDO) { o->decorridoMs = duracaoMs; o->bytes = brutoBytes; o->linhas = brutoLinhas; }
  else if (estado == DIAG_PRONTO) {
    o->decorridoMs = duracaoMs;
    o->restanteMs = prazoMs - t > 0 ? prazoMs - t : 0;
    o->bytes = finalBytes; o->linhas = finalLinhas;
    snprintf(o->arquivo, sizeof o->arquivo, "%s", finalArq);
    snprintf(o->nome, sizeof o->nome, "%s", nomeDl);
  }
  pthread_mutex_unlock(&m);
}

const char *diaglog_url(void) {
  static char u[200];
  DiagInfo i;
  const char *b = logrede_base();
  diaglog_info(&i);
  if (!i.codigo[0] || !b[0]) return "";
  snprintf(u, sizeof u, "%s/s/%s/", b, i.codigo);
  return u;
}

int diaglog_cabecalho_pendente(void) {
  int p;
  pthread_mutex_lock(&m);
  p = cabPend; cabPend = 0;
  pthread_mutex_unlock(&m);
  return p;
}
#endif
