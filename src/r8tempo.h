// TEMPO DAS PECAS R8 (plano 25/11, Fase 2): as curvas e os tempos EXATOS das
// pranchas aprovadas (canvas, secoes 8 e 10; o codigo delas esta em tools/r8).
//
// Nada aqui toca em GL nem em SDL: o relogio entra como argumento (segundos,
// double), para os testes compararem quadro a quadro com a especificacao
// exportada pelas pranchas (tests/r8_tempo.sh).
//
// As pecas (menus C e A, Estante, Coluna, Central, segurar CH+ e Busca) usam
// SO isto. Nao usam as molas antigas (mola.h/anim.h): a prancha e a regra.
#ifndef NV_R8TEMPO_H
#define NV_R8TEMPO_H

// ---------------------------------------------------------------- curvas
// Iguais as de tools/r8/base.py, que geraram as pranchas.
float r8_seg(float t, float a, float b);    // (t - a) / (b - a), preso em [0, 1]
float r8_lerp(float a, float b, float k);
float r8_dec(float k);                       // entrada: 1 - (1 - k)^3,2
float r8_acc(float k);                       // saida:   k^2,4
float r8_mola(float k);                      // foco: chega com ~10 % de repique e assenta (a curva da prancha)

// ---------------------------------------------------------------- tempos (s)
#define R8_PAINEL_ABRIR   0.50f   // Estante, Coluna editorial e Central
#define R8_PAINEL_FECHAR  0.28f
#define R8_MENU_ABRIR     0.44f   // menus C (Folha) e A (Desdobrar)
#define R8_MENU_FECHAR    0.26f
#define R8_FOLHA_ABRIR    0.44f   // folha de opcoes que nasce da pilula (10.2)
#define R8_FOLHA_FECHAR   0.26f
#define R8_BUSCA_ABRIR    0.65f   // progresso LINEAR; cada fase aplica a sua curva
#define R8_BUSCA_FECHAR   0.30f
#define R8_MOLA_DUR       0.27f   // o foco desliza
#define R8_TROCA_DUR      0.16f   // troca cruzada de conteudo
#define R8_TROCA_DX       12.0f   //   e o deslocamento dela (px)
#define R8_REDUZIDO_DUR   0.12f   // Reduzir animacoes: so opacidade
// O conteudo comeca a sair 20 ms ANTES da superficie (pranchas: o conteudo
// apaga de C0 - 0,02 a C0 + 0,09; a superficie fecha a partir de C0).
#define R8_CONTEUDO_ANTES 0.02f
#define R8_CONTEUDO_SAIR  0.11f
#define R8_MENU_CONTEUDO_SAIR 0.10f   // nos menus C e A (menus.py: C0 - 0,02 a C0 + 0,08)
// Segurar CH+ (decisao 4): 200 ms sem desenho + 450 ms enchendo.
#define R8_SEGURAR_MORTO  0.20f
#define R8_SEGURAR_ENCHER 0.45f
#define R8_SEGURAR_ESVAZIA 0.22f   // soltar antes: esvazia e cancela

// ---------------------------------------------------------------- abertura
// Abrir/fechar de uma superficie. Interromper no meio continua do valor em que
// estava (nunca salta): abrir de v vai a 1 pela curva de entrada; fechar de v
// vai a 0 pela de saida. Com v = 0 ou 1 e exatamente a curva da prancha.
typedef struct {
  int    estado;      // 0 fechada, 1 abrindo/aberta, 2 fechando
  double t0;          // quando o movimento atual comecou (a superficie)
  double tConteudo;   // quando o conteudo comecou a sair (fechando)
  double tAbriu;      // relogio da cascata de entrada do conteudo
  float  v0;          // valor no inicio do movimento atual
  float  dAbrir, dFechar;
  float  dConteudoSair;   // R8_CONTEUDO_SAIR (paineis) ou R8_MENU_CONTEUDO_SAIR
} R8Abertura;

void  r8ab_iniciar(R8Abertura *a, float dAbrir, float dFechar);
void  r8ab_abrir(R8Abertura *a, double agora);
// Fechar: o conteudo sai a partir de `agora`; a superficie, R8_CONTEUDO_ANTES
// depois (como na prancha).
void  r8ab_fechar(R8Abertura *a, double agora);
float r8ab_valor(const R8Abertura *a, double agora);    // com a curva
float r8ab_linear(const R8Abertura *a, double agora);   // sem curva (a Busca)
int   r8ab_fechada(const R8Abertura *a, double agora);  // fechou e ja assentou
int   r8ab_assentada(const R8Abertura *a, double agora);// aberta e parada em 1
// Opacidade do item `i` da cascata de conteudo: entra de `inicio + i*passo`
// por `dur` depois da abertura; sai junto (R8_CONTEUDO_SAIR) ao fechar.
float r8ab_cascata(const R8Abertura *a, double agora, float i, float inicio, float passo, float dur);

// ---------------------------------------------------------------- mola (foco)
// Um valor (ou um retangulo) que vai ao alvo pela mola em 270 ms. Mudar de
// alvo no meio parte do valor atual.
typedef struct { float de[5], para[5]; double t0; int n; } R8Mola;
void  r8mola_iniciar(R8Mola *m, const float *v, int n);
void  r8mola_ir(R8Mola *m, const float *alvo, double agora);
void  r8mola_valor(const R8Mola *m, double agora, float *out);
int   r8mola_parada(const R8Mola *m, double agora);

// ---------------------------------------------------------------- troca
// Troca cruzada (aba, pagina, agrupamento): o conteudo que sai some em 55 % de
// 160 ms indo 12 px para a esquerda; o que entra aparece de 25 % a 100 % vindo
// de 12 px a direita.
typedef struct { int atual, anterior; double t0; } R8Troca;
void  r8troca_iniciar(R8Troca *x, int atual);
void  r8troca_ir(R8Troca *x, int novo, double agora);
// Peso (opacidade) e deslocamento em x do conteudo `k`.
float r8troca_peso(const R8Troca *x, double agora, int k, float *dx);

// ---------------------------------------------------------------- segurar CH+
typedef struct {
  int    estado;      // 0 parado, 1 segurando, 2 esvaziando (soltou antes), 3 completou
  double t0;          // apertou
  double tSolto;      // soltou (esvaziando) ou completou
  float  pSolto;      // progresso no instante em que soltou
} R8Segurar;
void  r8seg_iniciar(R8Segurar *s);
void  r8seg_apertar(R8Segurar *s, double agora);
// Soltou. Devolve 1 se foi um toque curto (antes de completar): a acao do
// toque (Salvos) vale; 0 se ja tinha completado (a Central ja abriu).
int   r8seg_soltar(R8Segurar *s, double agora);
// Avanca o relogio; devolve 1 UMA vez, no quadro em que completa (abrir a Central).
int   r8seg_passo(R8Segurar *s, double agora);
float r8seg_progresso(const R8Segurar *s, double agora);   // 0..1 do fio
float r8seg_visivel(const R8Segurar *s, double agora);     // opacidade do fio

#endif
