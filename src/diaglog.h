// SESSOES DE DIAGNOSTICO (dono, 20/11/2026): substituem o "Registro na rede
// local" ligado/desligado. Fluxo: Iniciar logs -> Gravando (endereco + QR na
// TV) -> Finalizar logs -> o arquivo fica para baixar por EXATAMENTE 120 s no
// mesmo endereco -> o servidor fecha e o endereco morre.
//
// DE ONDE VEM O TEXTO: do mesmo stdout de sempre (nenhum printf novo no app,
// nenhuma medicao alterada). Cada sessao abre e fecha com uma linha-marca
//   [diag] ==== inicio da sessao <codigo> ====
//   [diag] ==== fim da sessao <codigo> ====
// e so o que fica ENTRE as duas marcas vai para o arquivo da sessao. No Android
// o fio que ja copia o stdout para o nuvio.log (android.c, espelho) entrega os
// bytes a diaglog_bytes: o arquivo da sessao nao passa pelo corte de 2 MB do
// nuvio.log e nao pega nada de antes da marca. Sem esse fio (outras
// plataformas), ao finalizar le-se o nuvio.log a partir do tamanho que ele
// tinha no inicio, pelo mesmo separador de marcas.
//
// O ARQUIVO ENTREGUE e o bruto da sessao limpo linha a linha por
// logrede_limpar (URL cortada no host, apikey/token/senha/Authorization em
// "…", sequencias longas cortadas). O bruto e apagado assim que o limpo fica
// pronto; a sessao seguinte apaga os arquivos da anterior.
//
// SERVIDOR: o de logrede.c, ligado so enquanto houver sessao (Gravando,
// Salvando ou Pronto); as rotas levam o codigo da sessao (/s/<codigo>/).
#ifndef NUVIO_DIAGLOG_H
#define NUVIO_DIAGLOG_H

#include <stddef.h>

#define DIAG_JANELA_MS 120000L   // download depois de finalizar
#define DIAG_SALVAR_PRAZO_MS 4000L   // espera pela marca de fim no fio do stdout
#define DIAG_MAX_BRUTO (64L * 1024 * 1024)

typedef enum { DIAG_PARADO = 0, DIAG_GRAVANDO, DIAG_SALVANDO, DIAG_PRONTO } DiagEstado;

typedef struct {
  DiagEstado estado;
  char codigo[16];      // "" parado
  long decorridoMs;     // gravando/salvando: desde o inicio; pronto: duracao gravada
  long restanteMs;      // pronto: ate o endereco fechar
  long bytes, linhas;   // gravando: brutos ate agora; pronto: do arquivo limpo
  char arquivo[640];    // pronto: o log limpo
  char nome[96];        // nome do download
  int  pt;              // pagina em portugues
  char aparelho[96];
  int  cortado;         // passou de DIAG_MAX_BRUTO
  int  fimConfirmado;   // a marca de fim chegou (0 = salvo por prazo)
} DiagInfo;

// Fio de desenho. pt = idioma da interface e portugues (pagina web).
// Devolve 1 se a sessao comecou. Uma sessao Pronta e substituida (o codigo
// antigo deixa de valer na hora; download ja em andamento continua).
int  diaglog_iniciar(int pt);
void diaglog_finalizar(void);
// Uma vez por quadro: prazos, fim da limpeza, fechamento aos 120 s, servidor.
void diaglog_tick(void);
// Qualquer fio.
void diaglog_info(DiagInfo *o);
// "http://192.168.1.20:8099/s/k3m9tq" com sessao e rede; "" sem.
const char *diaglog_url(void);
// Fio de desenho: 1 uma vez logo depois de iniciar (main.c imprime o cabecalho).
int  diaglog_cabecalho_pendente(void);
// android.c (fio do stdout): bytes do stdout na ordem em que sairam.
void diaglog_bytes(const char *b, size_t n);
// android.c: este processo tem o fio do stdout (sem ele, o fallback do arquivo).
void diaglog_tap_registrar(void);

#endif
