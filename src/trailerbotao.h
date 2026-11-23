#ifndef NUVIO_TRAILERBOTAO_H
#define NUVIO_TRAILERBOTAO_H
// O TRAILER DO YOUTUBE NO BOTAO "TRAILER" (World/Android, 23/11/2026). Qual dos trailers do TMDB vai ao botao e se ele
// passa na frente da Apple/do IMDb. Pedido do dono: "Dublado primeiro" e, sem audio no idioma, "pelo menos a legenda".
// Puro sobre extras_* e trailerapple_tem_legenda: testado em tests/trailerbotao.sh.

// O indice do trailer do TMDB: `k` pedido (>0); senao o dublado; senao o "Legendado" no idioma; senao o primeiro.
// -1 sem trailer do YouTube.
int trailerbotao_escolher(int k);
// O `youtubeIdiomaPreferido` de trailerfonte para o trailer `kk`: 1 passa na frente da Apple/IMDb, 0 fica atras,
// -1 espera (TMDB ou Apple ainda sem resposta). Dublado e Legendado no idioma passam; em outra lingua passa quando a
// Apple nao tem legenda no idioma (o YouTube liga a dele, traduzida se preciso). Audio escolhido em ingles: 0.
int trailerbotao_prioridade(int kk, const char *imdb, const char *apple, int appleRespondeu);
#endif
