// SERVIDOR DAS SESSOES DE DIAGNOSTICO NA REDE DE CASA (diaglog.h). Nasceu em
// 16/10/2026 como "Registro na rede local" (o fim do nuvio.log ao vivo, ligado
// por um interruptor); desde 20/11 serve so a sessao de diagnostico em curso:
//   /s/<codigo>/            a pagina (celular e computador, sem app nem internet)
//   /s/<codigo>/estado.json o estado, o tempo e o prazo
//   /s/<codigo>/log.txt     o arquivo limpo, so depois de finalizar
//   /                       leva a sessao atual
// Regras:
//   - sobe e desce com a sessao (diaglog_tick); fio proprio, nunca o de desenho;
//   - so responde a enderecos da rede local (10/8, 172.16/12, 192.168/16, CGNAT,
//     link-local) e ao proprio aparelho; so GET/HEAD;
//   - codigo errado, de outra sessao ou vencido: 410 "sessao encerrada";
//   - o texto e limpo linha a linha por logrede_limpar: URL cortada no host,
//     valores de apikey/token/senha/Authorization trocados por "…", sequencias
//     longas de letras e numeros (chaves) cortadas;
//   - no .wgt nao existe (o navegador nao abre porta).
#ifndef NUVIO_LOGREDE_H
#define NUVIO_LOGREDE_H

#include <stddef.h>

#define LOGREDE_PORTA 8099   // ocupada: uma livre (logrede_url diz qual)

// Uma vez por quadro (diaglog_tick): liga ou desliga. Desligar fecha a porta
// de escuta; conexoes em andamento (um download) terminam no fio delas.
void logrede_servidor(int ligado);
// "http://192.168.1.20:8099" com o servidor de pe e um IP de rede local; "" sem.
const char *logrede_base(void);
// 1 com a porta aberta (mesmo sem IP de rede local ainda).
int logrede_ativo(void);
// Limpa uma linha de log no lugar (exposta para teste). Nunca cresce a linha.
void logrede_limpar(char *linha);

#endif
