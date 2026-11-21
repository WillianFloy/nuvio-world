// "PEDAGIO" DE IMAGENS (World, 21/11/2026; ideia do dono): a arte que vem de
// hosts que nao servem tamanhos (repositorios do GitHub, CDNs de addons) passa
// por um servico de redimensionamento ANTES de chegar a TV, e chega no tamanho
// do desenho. Nos logs da TCL essas artes vinham com 10 a 60 vezes os pixels
// exibidos (2560x1440 para um cartao de 320x180) e custavam 300-600 ms de
// decode cada; as miniaturas locais (miniatura.h) resolvem da SEGUNDA vez em
// diante, o pedagio resolve a PRIMEIRA.
//
// Servico: wsrv.nl (codigo aberto, weserv/images, com cache mundial na
// Cloudflare; da para hospedar o nosso com o mesmo formato de URL — basta
// trocar PEDAGIO_BASE). Recebe a URL original e devolve a imagem reduzida, em
// WebP (mantem transparencia de logos). So imagens PUBLICAS passam por ele:
// as mesmas URLs que a TV ja baixaria.
//
// Regras:
//   - so http(s) e hosts SEM escada de tamanhos propria (TMDB e metahub ja
//     tem: artetamanho.h); nunca GIF (animacao), nunca rede local;
//   - largura em degraus fixos (256 ... 1920), o menor que cobre o teto do
//     pedido — degrau fixo e o que deixa o cache do servico acertar;
//   - `we`: nunca amplia uma arte menor que o degrau;
//   - DISJUNTOR: 3 falhas seguidas do servico desligam o pedagio por 10 min
//     (a arte vai direto ao original, como antes). Quem chama tambem tenta o
//     original na hora, quando o pedagio falha.
#ifndef NV_PEDAGIO_H
#define NV_PEDAGIO_H
#include <stddef.h>

#define PEDAGIO_BASE "https://wsrv.nl/"

// Ligado (padrao). Android: setprop debug.nuvio.pedagio 0 desliga.
extern int pedagio_ligado;

// 1 e `saida` = URL do pedagio para `url` no teto `limite` (px); 0 quando a
// URL deve ir direto (host com tamanhos, GIF, local, disjuntor aberto...).
int pedagio_url(const char *url, int limite, char *saida, size_t tam, unsigned agoraMs);

// A URL e do pedagio? (para o log e para o recuo ao original)
int pedagio_eh(const char *url);

// Resultado de uma ida ao pedagio (disjuntor e contadores).
void pedagio_resultado(int ok, unsigned agoraMs);

typedef struct { int pedidos, ok, falhas, desligado; } PedagioStats;
void pedagio_stats(PedagioStats *o, int zerar);
#endif
