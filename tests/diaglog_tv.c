// TV SIMULADA PARA AS SESSOES DE DIAGNOSTICO (diaglog.h + logrede.c de verdade).
//
// O stdout vai para um pipe e um fio copia os bytes para o "nuvio.log" e para
// diaglog_bytes -- o mesmo caminho do espelho do Android (android.c). O laco
// chama diaglog_tick a cada 20 ms, como o app a cada quadro. Comandos chegam
// por stdin (um por linha), e o estado sai por stderr para o script:
//   iniciar | finalizar | linha <texto> | volume <MB> | estado | sair
// tests/diaglog.sh dirige isto com curl e Chrome.
#include "diaglog.h"
#include "logrede.h"
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int fdLog = -1, fdLeitura = -1;

static void *espelho(void *u) {
  char buf[4096];
  ssize_t n;
  (void)u;
  while ((n = read(fdLeitura, buf, sizeof buf)) > 0) {
    if (fdLog >= 0 && write(fdLog, buf, (size_t)n) < 0) { /* so o log geral */ }
    diaglog_bytes(buf, (size_t)n);
  }
  return NULL;
}

static const char *nomeEstado(DiagEstado e) {
  return e == DIAG_GRAVANDO ? "gravando" : e == DIAG_SALVANDO ? "salvando" : e == DIAG_PRONTO ? "pronto" : "parado";
}

static void relatar(const char *ev) {
  DiagInfo i;
  struct timespec t;
  diaglog_info(&i);
  clock_gettime(CLOCK_MONOTONIC, &t);
  fprintf(stderr, "EV %s t=%.3f estado=%s codigo=%s restante=%ld linhas=%ld bytes=%ld url=%s arquivo=%s confirmado=%d\n",
          ev, t.tv_sec + t.tv_nsec / 1e9, nomeEstado(i.estado), i.codigo, i.restanteMs, i.linhas, i.bytes,
          diaglog_url(), i.arquivo, i.fimConfirmado);
  fflush(stderr);
}

int main(void) {
  int p[2];
  pthread_t th;
  char cmd[4096];
  size_t ncmd = 0;
  DiagEstado ult = DIAG_PARADO;
  int servindo = 0;
  const char *log = getenv("NUVIO_LOG");
  if (log) fdLog = open(log, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0600);
  if (pipe(p)) return 2;
  fdLeitura = p[0];
  pthread_create(&th, NULL, espelho, NULL);
  dup2(p[1], STDOUT_FILENO);
  close(p[1]);
  setvbuf(stdout, NULL, _IOLBF, 0);
  diaglog_tap_registrar();
  printf("[tv] linha ANTES da sessao (nao pode ir para o arquivo) apikey=SEGREDOANTES123\n");
  fflush(stdout);
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
  relatar("pronta");
  for (;;) {
    struct pollfd pf = { STDIN_FILENO, POLLIN, 0 };
    if (poll(&pf, 1, 20) > 0) {
      ssize_t r = read(STDIN_FILENO, cmd + ncmd, sizeof cmd - 1 - ncmd);
      if (r == 0) break;
      if (r > 0) ncmd += (size_t)r;
      cmd[ncmd] = 0;
      for (;;) {
        char *nl = strchr(cmd, '\n');
        if (!nl) break;
        *nl = 0;
        if (!strcmp(cmd, "iniciar")) { relatar(diaglog_iniciar(1) ? "iniciou" : "nao-iniciou"); }
        else if (!strcmp(cmd, "finalizar")) { diaglog_finalizar(); relatar("finalizou"); }
        else if (!strncmp(cmd, "linha ", 6)) { printf("%s\n", cmd + 6); fflush(stdout); }
        else if (!strncmp(cmd, "volume ", 7)) {
          long mb = atol(cmd + 7), i, n = mb * 1024 * 1024 / 128;
          for (i = 0; i < n; i++)
            printf("[ritmo] tela=60.00Hz ref=16.67ms(tela) | volume %08ld ......................................................\n", i);
          fflush(stdout);
          relatar("volume");
        }
        else if (!strcmp(cmd, "estado")) relatar("estado");
        else if (!strcmp(cmd, "sair")) { relatar("saindo"); return 0; }
        memmove(cmd, nl + 1, strlen(nl + 1) + 1);
        ncmd = strlen(cmd);
      }
    }
    diaglog_tick();
    {
      DiagInfo i;
      diaglog_info(&i);
      if (i.estado != ult) { ult = i.estado; relatar("mudou"); }
      if (logrede_ativo() != servindo) { servindo = logrede_ativo(); relatar(servindo ? "servidor-ligado" : "servidor-desligado"); }
    }
  }
  return 0;
}
