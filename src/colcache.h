// CACHE DAS FILEIRAS DAS COLLECTIONS (2.0.34).
//
// O log da TCL de 932 MB mostrou cada abertura de Collection baixando TODAS as
// fileiras de novo (Netflix duas vezes na mesma sessao) e fileiras baixadas de
// novo DENTRO da mesma visita, quando a Vitrine devolvia a memoria das
// distantes. Este modulo guarda o resultado de cada fonte:
//
//  - em MEMORIA, comprimido (RLE de zeros, o mesmo do cache de catalogo: um
//    CatItem de ~17,8 KB vira ~0,7 KB), com teto de bytes e descarte do mais
//    antigo. A fileira que devolveu a memoria volta daqui na hora, sem rede;
//  - em DISCO, um arquivo por fonte na pasta de cache do Android (podada pela
//    regra de sempre), para a proxima abertura do app.
//
// Quem le decide o frescor: `idade` volta em segundos. A Vitrine mostra o que
// tem e, passado COLCACHE_FRESCO_S, pede de novo em segundo plano; so troca a
// fileira se o conteudo mudou (stale-while-revalidate).
//
// A chave inclui a conta, o perfil, o idioma dos metadados, o filtro de vistos,
// o limite por fileira, a base do addon e a definicao da fonte: trocar
// qualquer um deles e outra entrada. Todas as funcoes sao seguras entre fios.
#ifndef NV_COLCACHE_H
#define NV_COLCACHE_H
#include <stdint.h>
#include "catalogo.h"
#include "colecoes.h"

#define COLCACHE_FRESCO_S (15 * 60)   // depois disto: mostra e revalida

typedef struct { int n, extraN, pagina, mais; } ColCacheMeta;

// Chave da fonte para o perfil/sessao atual.
uint64_t colcache_chave(const ColSource *fonte, const char *base, int limite);
// 1 = achou (memoria ou disco). `itens` recebe n + extraN itens (cabe em `max`).
int colcache_ler(uint64_t chave, CatItem *itens, int max, ColCacheMeta *meta, long *idade);
// Grava n + extraN itens. Pode ser chamada do fio do pedido (escreve o disco).
void colcache_gravar(uint64_t chave, const CatItem *itens, const ColCacheMeta *meta);
// Esquece tudo (troca de conta/logout). O disco e apagado so com `disco`.
void colcache_limpar(int disco);
// Estatisticas para o log de desempenho.
void colcache_stats(long *bytesMem, int *entradas, long *acertos, long *faltas);
// Teto da memoria (bytes). Padrao: 2 MB; 1 MB em TVs < 1,2 GB.
void colcache_teto(long bytes);

// Exposto para teste: compressao e o formato do arquivo.
size_t colcache_rle(const unsigned char *src, size_t n, unsigned char *dst);
int colcache_unrle(const unsigned char *enc, size_t encN, unsigned char *dst, size_t n);
#endif
