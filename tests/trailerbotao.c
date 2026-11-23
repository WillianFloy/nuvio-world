// World (23/11): qual trailer do TMDB vai ao botao e se passa na frente da Apple/IMDb (trailerbotao.c real).
// Pedido do dono (log s2p3n7): "pelo menos a legenda se o audio nao for possivel".
//   1. dublado existe: e o escolhido e passa na frente;
//   2. sem dublado, "Legendado" em pt: escolhido e passa na frente (legenda na imagem);
//   3. so ingles e a Apple SEM legenda pt: o YouTube passa (liga a legenda dele);
//   4. so ingles e a Apple COM legenda pt: fica a Apple;
//   5. so ingles e a Apple ainda sem resposta: espera;
//   6. audio do trailer escolhido em ingles: nada passa na frente;
//   7. "Legendado" em espanhol com alvo pt nao conta como legenda;
//   8. TMDB ainda carregando: espera; sem trailer nenhum: -1 de escolha.
#include "../src/trailerbotao.h"
#include <stdio.h>
#include <string.h>
static int falhas;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
typedef struct { const char *yt, *lg; int pref, leg; } T;
static T tr[6]; static int n, carregando, appleLeg; static const char *alvo = "pt";
int extras_n_trailers(void) { return n; }
const char *extras_trailer_yt(int i) { return i >= 0 && i < n ? tr[i].yt : ""; }
const char *extras_trailer_lingua(int i) { return i >= 0 && i < n ? tr[i].lg : ""; }
int extras_trailer_legendado(int i) { return i >= 0 && i < n && tr[i].leg; }
int extras_trailer_idioma_preferido(int i) { if (!n && carregando) return -1; return i >= 0 && i < n && tr[i].pref; }
void extras_trailer_audio_iso(char *d) { strcpy(d, alvo); }
int trailerapple_tem_legenda(const char *imdb, const char *iso) { (void)imdb; return appleLeg && !strcmp(iso, "pt"); }
static void lista(int nn, T a, T b, T c) { n = nn; tr[0] = a; tr[1] = b; tr[2] = c; }
int main(void) {
  T en = { "engENG00001", "en", 0, 0 }, dub = { "dubDUB00001", "pt", 1, 0 }, leg = { "legLEG00001", "pt", 0, 1 },
    legEs = { "legESP00001", "es", 0, 1 };
  const char *A = "file:///apple";
  int k;
  lista(3, dub, leg, en);
  k = trailerbotao_escolher(0);
  ok(k == 0 && trailerbotao_prioridade(k, "tt", A, 1) == 1, "1. dublado: escolhido e na frente");
  lista(2, en, leg, en);
  k = trailerbotao_escolher(0);
  ok(k == 1 && trailerbotao_prioridade(k, "tt", A, 1) == 1, "2. sem dublado: o Legendado pt, na frente");
  ok(trailerbotao_prioridade(k, "tt", NULL, 0) == 1, "2. o Legendado nao espera a Apple responder");
  lista(1, en, en, en); appleLeg = 0;
  k = trailerbotao_escolher(0);
  ok(k == 0 && trailerbotao_prioridade(k, "tt", A, 1) == 1, "3. so ingles, Apple sem legenda: YouTube na frente");
  ok(trailerbotao_prioridade(k, "tt", NULL, 1) == 1, "3. so ingles, sem Apple (IMDb sem legenda): YouTube na frente");
  appleLeg = 1;
  ok(trailerbotao_prioridade(k, "tt", A, 1) == 0, "4. so ingles, Apple com legenda pt: fica a Apple");
  appleLeg = 0;
  ok(trailerbotao_prioridade(k, "tt", NULL, 0) == -1, "5. Apple ainda sem resposta: espera");
  alvo = "en";
  ok(trailerbotao_prioridade(k, "tt", A, 1) == 0, "6. audio escolhido em ingles: nada na frente");
  alvo = "pt";
  lista(2, en, legEs, en);
  k = trailerbotao_escolher(0);
  ok(k == 0, "7. Legendado em espanhol nao e escolhido com alvo pt");
  appleLeg = 1;
  ok(trailerbotao_prioridade(1, "tt", A, 1) == 0, "7. e nao passa na frente da Apple com legenda");
  appleLeg = 0;
  n = 0; carregando = 1;
  ok(trailerbotao_escolher(0) == -1 && trailerbotao_prioridade(-1, "tt", A, 1) == -1, "8. TMDB carregando: espera");
  carregando = 0;
  ok(trailerbotao_prioridade(-1, "tt", A, 1) == 0, "8. sem trailer nenhum: nada");
  printf("%s\n", falhas ? "trailerbotao: FALHOU" : "trailerbotao: ok");
  return falhas != 0;
}
