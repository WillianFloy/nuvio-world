// O ID DO IMDb QUE OS TRAILERS PRECISAM (World, 21/11/2026; dono: "os trailers
// nao estao iniciando de jeito algum, nem automatico nem manualmente").
//
// As fontes de trailer do app sao a Apple (busca pelo TITULO na loja dos EUA)
// e o IMDb (pelo id "tt..."). Os catalogos da conta passaram a entregar quase
// tudo com id do TMDB ("tmdb:1560520"; 100 de 101 focos no log da SEMP) e com
// o titulo em portugues: o IMDb recusava o id (trailerimdb_pedir so aceita
// "tt") e nunca respondia, e a Apple nao achava "Uma Familia da Pesada"
// (candidatos=0). Resultado: o destaque esperava a janela e desistia, e o
// botao do detalhe ficava esperando uma resposta que nao vinha.
//
// Aqui o id do TMDB vira o "tt" pela API do TMDB (/movie|tv/{id}/external_ids,
// a mesma chave que o app ja usa nas artes), uma vez por titulo, num fio
// proprio, com o resultado em memoria. Quem pede recebe o "tt" quando ja se
// sabe e, ate la, o proprio id (e a consulta comeca).
#ifndef NV_TRAILERTT_H
#define NV_TRAILERTT_H

// O id que as fontes de trailer devem usar para `id` ("tt..." passa direto).
// Para "tmdb:N" devolve o "tt" ja resolvido ou, enquanto nao, o proprio `id` e
// dispara a consulta (uma por titulo; falha fica lembrada por 10 min). O
// ponteiro devolvido vale ate a proxima chamada que resolva OUTRO titulo:
// copie se for guardar.
const char *trailertt_id(const char *id, int serie);

// O titulo EM INGLES do TMDB para "tmdb:N" (vem na mesma consulta do "tt"; World 22/11). 1 se ja se sabe e existe.
// A Apple busca na loja dos EUA: com o titulo em portugues ela nao acha nada. Nao dispara consulta (trailertt_id dispara).
#include <stddef.h>
int trailertt_titulo_en(const char *id, char *dst, size_t tam);

// 1 = ja se sabe o resultado para `id` (resolvido, falhou ou nao precisa).
int trailertt_respondeu(const char *id);

// Testes: troca o download (padrao rede_baixar) e zera a tabela.
void trailertt_testes(char *(*baixar)(const char *url, int segundos), const char *chave);
#endif
