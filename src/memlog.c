#include "memlog.h"
#include <stdio.h>
#include <string.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif

static void (*avisoCb)(void);   // memlog_quando_aviso (fio principal)

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
static int abaixo; // Compartilhado por FPS e player, ambos no fio principal.

static long lerKB(const char *path, const char *campo) {
  FILE *f = fopen(path, "r");
  char linha[256], chave[64], unidade[8], sobra;
  long kb = -1, valor;
  if (!f) return -1;
  while (fgets(linha, sizeof linha, f)) {
    if (sscanf(linha, "%63s %ld %7s %c", chave, &valor, unidade, &sobra) == 3 &&
        !strcmp(chave, campo) && !strcmp(unidade, "kB") && valor >= 0) {
      kb = valor;
      break;
    }
  }
  fclose(f);
  return kb;
}
#endif

// 2.0.36: o piso do aviso de memoria baixa, proporcional a RAM no Android. O
// log da TCL (932 MB) mostrou MemAvailable normal de 130-190 MB: o piso fixo de
// 150 MB disparava `aliviarMemoria` 5 vezes em ~2 min, e cada uma derrubava o
// teto de texturas de 48 MB para 11 MB por 30 s (ciclo de redecodificacao e
// carregamento visivel). A escala e a do webOS (tex_memoria_livre: 150 MB para
// 1965 MB de RAM), com piso minimo; de 1965 MB para cima fica em 150 MB. O
// onTrimMemory do Android segue como sinal de pressao REAL do sistema.
int memlog_piso_para_ram(long ramMb) {
  if (ramMb <= 0 || ramMb >= 1965) return MEMLOG_PISO_MB;
  long piso = MEMLOG_PISO_MB * ramMb / 1965;
  return (int)(piso < 48 ? 48 : piso);
}
// Para voltar a armar o aviso a memoria precisa passar do piso por esta folga
// (60 MB na escala de 1965 MB): MemAvailable oscila dezenas de MB em segundos.
int memlog_folga_para_ram(long ramMb) {
  if (ramMb <= 0 || ramMb >= 1965) return 0;
  long folga = 60 * ramMb / 1965;
  return (int)(folga < 16 ? 16 : folga);
}
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
static int pisoAviso(void) {
#ifdef __ANDROID__
  static int piso = -1;
  if (piso < 0) piso = memlog_piso_para_ram(lerKB("/proc/meminfo", "MemTotal:") / 1024);
  return piso;
#else
  return MEMLOG_PISO_MB;
#endif
}
static int folgaAviso(void) {
#ifdef __ANDROID__
  static int folga = -1;
  if (folga < 0) folga = memlog_folga_para_ram(lerKB("/proc/meminfo", "MemTotal:") / 1024);
  return folga;
#else
  return 0;
#endif
}
#endif

void memlog_amostra(char *sufixo, size_t tamanho) {
  if (!tamanho) return;
  sufixo[0] = 0;
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
  long rss = lerKB("/proc/self/status", "VmRSS:");
  long disponivel = lerKB("/proc/meminfo", "MemAvailable:");
  if (rss < 0 || disponivel < 0) return;
  snprintf(sufixo, tamanho, " | mem=%.1f/%.1fMB", rss / 1024.0, disponivel / 1024.0);
  int piso = pisoAviso();
  if (disponivel < piso * 1024L && !abaixo) {
    printf("[mem] aviso MemAvailable<%dMB mem=%.1f/%.1fMB\n", piso, rss / 1024.0, disponivel / 1024.0);
    fflush(stdout);
    if (avisoCb) avisoCb();   // texturas + mkvass + texto + libass + heap
    else memlog_devolver();
  }
  // Armado de novo so acima do piso + folga (no Android; fora dele a folga e 0).
  if (disponivel < piso * 1024L) abaixo = 1;
  else if (disponivel >= (piso + folgaAviso()) * 1024L) abaixo = 0;
#endif
}

int memlog_pouca_memoria(void) {
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
  long kb = lerKB("/proc/meminfo", "MemAvailable:");
  return kb >= 0 && kb < MEMLOG_PISO_MB * 1024;
#else
  return 0;
#endif
}

// Mesma regra do NvPlayer.poucaRamAparelho: ate 3 GiB de RAM total. So Android.
int memlog_android_fraco(void) {
#if defined(__ANDROID__)
  static int fraco = -1;
  if (fraco < 0) {
    long kb = lerKB("/proc/meminfo", "MemTotal:");
    fraco = kb > 0 && kb <= 3L * 1024 * 1024;
  }
  return fraco;
#else
  return 0;
#endif
}

long memlog_livre_mb(void) {
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
  long kb = lerKB("/proc/meminfo", "MemAvailable:");
  return kb >= 0 ? kb / 1024 : -1;
#else
  return -1;
#endif
}

void memlog_evento(const char *evento) {
  char sufixo[96];
  memlog_amostra(sufixo, sizeof sufixo);
  if (*sufixo) {
    printf("[mem] %s%s\n", evento, sufixo + 2);
    fflush(stdout);
  }
}

// #415 (LG G4, webOS 9): a sessao de video deixava o rss em 524 MB com o player
// fechado e o gpu-cache parado — heap livre que o glibc segura. Devolve ao
// sistema no aviso e ao fechar o player. Sem glibc (Android, WASM), nada.
void memlog_devolver(void) {
#ifdef __GLIBC__
  malloc_trim(0);
#endif
}

void memlog_quando_aviso(void (*cb)(void)) { avisoCb = cb; }

int memlog_pode_reabrir(long (*livre)(void), void (*aliviar)(void)) {
  long mb = livre();
  if (mb < 0 || mb >= MEMLOG_PISO_MB) return 1;
  aliviar();
  mb = livre();
  return mb < 0 || mb >= MEMLOG_PISO_MB;
}
