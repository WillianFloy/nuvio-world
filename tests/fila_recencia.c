// NAVEGACAO RAPIDA (19/11): regras da fila do tex_cache, sem GL.
//   1. No mesmo nivel vence o pedido do quadro mais recente (o que a pessoa
//      deixou para tras vai para o fim; os da tela ficam na ordem de chegada).
//   2. Niveis continuam mandando: logo > tela cheia > cartaz visivel > pre-carga.
//   3. Pre-carga: o pedido MAIS NOVO passa na frente (troca de direcao).
//   4. Pre-carga vence: pedida ha >= NV_TEX_LA_VENCE_MS e ainda na fila e obsoleta.
//   5. Promocao de previa desfeita (pedido obsoleto) NAO congela o borrado.
#include "../src/sdlcompat.h"
#include "../src/tex_cache.c"
#include <stdio.h>
static int falhas;
#define OK(c, m) do { if (c) printf("PASS: %s\n", m); else { printf("FAIL: %s\n", m); falhas++; } } while (0)
static int f[MAX_FILA], ini, fim;
static void limpar(void) { memset(itens, 0, sizeof itens); ini = fim = 0; }
static void por(int idx, int urg, int la, unsigned long quadro, Uint32 pedido) {
  itens[idx].estado = PENDENTE; itens[idx].urgente = urg; itens[idx].lookahead = la;
  itens[idx].ultimoQuadro = quadro; itens[idx].ultimoPedido = pedido;
  f[fim] = idx; fim = (fim + 1) % MAX_FILA;
}
int main(void) {
  // 1. Trilha de navegacao: 1..4 pedidos ha 10-40 quadros (passou), 5..7 na tela agora.
  limpar(); quadroAtual = 100;
  por(1, 0, 0, 60, 1); por(2, 0, 0, 70, 2); por(3, 0, 0, 80, 3); por(4, 0, 0, 90, 4);
  por(5, 0, 0, 100, 5); por(6, 0, 0, 100, 6); por(7, 0, 0, 100, 7);
  int a = tirarFila(f, &ini, fim), b = tirarFila(f, &ini, fim), c = tirarFila(f, &ini, fim), d = tirarFila(f, &ini, fim);
  OK(a == 5 && b == 6 && c == 7, "os cartazes da tela agora saem primeiro, na ordem de chegada");
  OK(d == 4, "depois, o rastro mais recente (o que a pessoa acabou de deixar)");
  // 2. Niveis.
  limpar(); quadroAtual = 50;
  por(1, 0, 1, 0, 900); por(2, 0, 0, 50, 10); por(3, 1, 0, 40, 11); por(4, 2, 0, 30, 12);
  a = tirarFila(f, &ini, fim); b = tirarFila(f, &ini, fim); c = tirarFila(f, &ini, fim); d = tirarFila(f, &ini, fim);
  OK(a == 4 && b == 3 && c == 2 && d == 1, "logo > tela cheia > cartaz visivel > pre-carga, mesmo pedidos antes");
  // 3. Pre-cargas: a mais nova primeiro.
  limpar(); quadroAtual = 10;
  por(1, 0, 1, 0, 100); por(2, 0, 1, 0, 200); por(3, 0, 1, 0, 300);
  OK(tirarFila(f, &ini, fim) == 3, "pre-carga: a direcao nova (pedido mais novo) passa na frente");
  // 4. Vencimento da pre-carga.
  limpar();
  itens[1].lookahead = 1; itens[1].ultimoQuadro = 0; itens[1].ultimoPedido = SDL_GetTicks();
  OK(!pedidoObsoleto(&itens[1]), "pre-carga recem-pedida nao e obsoleta");
  itens[1].ultimoPedido = SDL_GetTicks() - NV_TEX_LA_VENCE_MS - 10;
  OK(pedidoObsoleto(&itens[1]), "pre-carga parada na fila ha 1,5 s sai (lugar por onde a pessoa passou)");
  itens[1].sup = (SDL_Surface *)1;
  OK(!pedidoObsoleto(&itens[1]), "ja decodificada nunca e jogada fora");
  itens[1].sup = NULL;
  // 5. Previa: promocao desfeita volta ao teto da previa.
  limpar();
  itens[2].tex = 7; itens[2].previa = 1; itens[2].tetoUsado = NV_TEX_PREVIA_LARG; itens[2].limite = 288;
  itens[2].estado = PENDENTE;
  desistir(2);
  OK(itens[2].estado == PRONTO && itens[2].tetoUsado == NV_TEX_PREVIA_LARG,
     "promocao de previa desfeita: o proximo pedido do cartao tenta de novo (nao fica borrado)");
  itens[3].tex = 8; itens[3].previa = 0; itens[3].tetoUsado = 128; itens[3].limite = 288; itens[3].estado = PENDENTE;
  desistir(3);
  OK(itens[3].tetoUsado == 288, "fora da previa a regra antiga continua (nao decodifica em laco)");
  if (falhas) return 1;
  puts("fila_recencia: ok");
  return 0;
}
