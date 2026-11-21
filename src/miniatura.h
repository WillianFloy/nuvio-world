// MINIATURAS LOCAIS: a arte guardada no disco JA NO TAMANHO DE EXIBICAO.
//
// POR QUE (logs da TCL, 20/11/2026): a arte das Collections vem de
// repositorios no GitHub com 10 a 60 vezes mais pixels do que a tela mostra —
// 1000x1500 para um cartaz de 192x288, 2560x1440 e 3840x2160 para um cartao de
// 320x180 ou um fundo de 1280x720. Cada uma custava 300 a 600 ms de decode
// nesta CPU, com dois fios de decode: uma fileira de pastas levava 1-2 s para
// preencher (o "esqueleto" visivel), e o cache de disco guardava o ORIGINAL,
// entao cada abertura do app pagava tudo de novo. Decodificar 2560x1440 ainda
// pede ~15 MB de RAM de uma vez, numa TV com ~60 MB livres.
//
// O servidor da Netflix entrega a imagem no tamanho exato; os nossos (GitHub,
// TMDB, addons) nao. Este modulo faz o equivalente na TV: depois do PRIMEIRO
// decode caro, grava a superficie reduzida (JPEG 4:4:4 q90 quando opaca, cru
// RGBA quando tem transparencia). Da segunda vez em diante o decode le uma
// imagem do tamanho da tela. O original continua no cache, para quem pedir um
// tamanho maior (destaque, promocao).
//
// Formato do arquivo: carga util (JPEG, que comeca em FF D8, ou pixels crus)
// seguida de um RODAPE de 32 bytes que termina com "NVMI". O JPEG ignora o
// que vem depois do fim da imagem, entao a carga e lida pelo decodificador de
// sempre. Rodape invalido, tamanho que nao bate ou decode que falha: o
// arquivo e APAGADO (cache que corrompe nao pode derrubar nada, so voltar ao
// caminho lento).
//
// Nome: <hash32 da url em 8 hex><teto em 4 hex>.nvm, na pasta do cache de
// arte — entra no mesmo indice e na mesma poda do cache de disco.
#ifndef NV_MINIATURA_H
#define NV_MINIATURA_H
#include <stddef.h>
#include <SDL2/SDL.h>

typedef struct {
  int srcW, srcH;            // tamanho do ARQUIVO original (fonteW da promocao)
  int limite;                // teto do pedido que gerou a miniatura
  int limTam;                // variante de tamanho usada no download (0 = nenhuma)
  unsigned char magica[4];   // primeiros bytes do original
} MiniInfo;

// Nome do arquivo da miniatura de `url` para o teto `limite`.
void mini_nome(const char *dir, const char *url, int limite, char *dst, size_t n);

// Le a miniatura. NULL quando nao existe (*corrompida = 0) ou quando estava
// invalida e foi apagada (*corrompida = 1). Devolve ABGR8888.
SDL_Surface *mini_ler(const char *caminho, int limite, MiniInfo *info, int *corrompida);

// Vale gravar? Arte de origem conhecida (JPEG, PNG, WebP parado; nunca GIF
// nem WebP animado) que foi reduzida a ao menos metade dos pixels, ou que
// levou >= 60 ms para decodificar; saida entre 8x8 e 1920x1088.
// `cab` sao os primeiros bytes do original (ate 32; nCab o que veio).
int mini_vale_gravar(int srcW, int srcH, int outW, int outH, unsigned decodeMs,
                     const unsigned char *cab, int nCab);

// Grava (temporario + rename). Devolve os bytes gravados, 0 em falha.
long mini_gravar(const char *caminho, SDL_Surface *s, const MiniInfo *info);

// Contadores da sessao (log [mini]).
typedef struct { int lidas, gravadas, corrompidas, falhas; double msLer, msGravar; long bytesGravados; } MiniStats;
void mini_stats(MiniStats *o, int zerar);
#endif
