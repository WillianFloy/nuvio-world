// MATERIAL, FOCO, RAIOS E CORES DAS PECAS R8 (Fase 2; prancha R8-1 Fundacao).
//
// Um lugar so para o que as pranchas fixaram, para as pecas (menus C e A,
// Estante, Coluna, Central, segurar CH+ e Busca) nao inventarem valor proprio.
// A tipografia sao os estilos TXT_R8_* de text.h (corpo e peso das pranchas;
// 800 = Montserrat ExtraBold).
//
// TEMA (Ajustes > Aparencia > Tom da interface, gfx_tema_preto):
//  - Grafite: a superficie e a Tinta rgb(14,14,16) a 97 %.
//  - Preto puro: a superficie e PRETO #000 (decisao 8, 25/11: "no preto puro tem
//    que ser preto"). O fio de 1,5 px e a sombra continuam: sao eles que separam
//    a peca do fundo.
#ifndef NV_R8ESTILO_H
#define NV_R8ESTILO_H

#include "gfx.h"

// ---------------------------------------------------------------- cores (0..1)
#define R8_COR(r, g, b) (r) / 255.0f, (g) / 255.0f, (b) / 255.0f
#define R8_TINTA   R8_COR(14, 14, 16)     // superficie (Grafite)
#define R8_INK     R8_COR(11, 11, 12)     // texto sobre o osso
#define R8_OSSO    R8_COR(244, 242, 238)  // texto principal, foco, botao primario
#define R8_MUTED   R8_COR(180, 177, 171)  // texto de apoio
#define R8_DIM     R8_COR(140, 138, 133)  // legenda, rotulo de grupo
#define R8_ACENTO  R8_COR(255, 77, 109)   // so progresso e "ao vivo"; nunca foco

// ---------------------------------------------------------------- raios (px)
#define R8_RAIO_FOLHA   22.0f   // folha do menu C, folha da S2
#define R8_RAIO_PAINEL  16.0f   // folha de opcoes, linha, cartao de pagina
#define R8_RAIO_CARTAZ  12.0f
#define R8_RAIO_PILULA  26.0f   // pilula de 52 px (metade da altura)
#define R8_RAIO_COLUNA  26.0f   // Coluna da Central

// ---------------------------------------------------------------- material
#define R8_SUPERFICIE_A   0.97f
#define R8_FIO_PX         1.5f
#define R8_FIO_A          0.10f
// Sombra longa: 0 30 px 80 px, preto 60 % (box-shadow da prancha).
#define R8_SOMBRA_DY      30.0f
#define R8_SOMBRA_BLUR    80.0f
#define R8_SOMBRA_A       0.60f
// Foco numa acao: veu osso 13 % + contorno de 2 px. Num conteudo: contorno de
// 3 px. Escala do conteudo em foco 1,06; pressionado 0,96.
#define R8_FOCO_VEU_A     0.13f
#define R8_FOCO_ACAO_PX   2.0f
#define R8_FOCO_CONT_PX   3.0f
#define R8_FOCO_ESCALA    1.06f
#define R8_APERTO_ESCALA  0.96f

// A cor da superficie no tema atual (Grafite: Tinta; Preto puro: 0, 0, 0).
void r8_superficie_cor(float *r, float *g, float *b, float *a);
// A superficie inteira: sombra longa, miolo e fio. `a` multiplica tudo (a
// opacidade da peca entrando). Opaca (a >= 0,98 no miolo), a sombra sai
// vazada: nada e pintado embaixo dela.
void r8_material(GfxRect r, float raioPx, float a);
// O mesmo, com a sombra deslocada (dx, dy): a Folha do menu C joga a sombra
// para o lado do conteudo (-30, 0 na direita; +30, 0 na esquerda).
void r8_material_sombra(GfxRect r, float raioPx, float a, float dx, float dy);
// So a sombra (para quem pinta o miolo por conta propria, ex.: a arte da folha).
void r8_sombra(GfxRect r, float raioPx, float a);
void r8_sombra_desl(GfxRect r, float raioPx, float a, float dx, float dy);
// Foco numa acao (linha, botao, pilula) e num conteudo (anel por dentro de `r`).
void r8_foco_acao(GfxRect r, float raioPx, float a);
void r8_foco_conteudo(GfxRect r, float raioPx, float a);
// FOCO NUMA LINHA DE MENU (Folha e Desdobrar, dono 26/11: o de acao ficava
// redondo e pesado demais): raio 10, veu osso 10 %, contorno de 1,5 px a 50 %.
#define R8_RAIO_LINHA     10.0f
#define R8_FOCO_LINHA_VEU 0.10f
#define R8_FOCO_LINHA_PX  1.5f
#define R8_FOCO_LINHA_A   0.50f
void r8_foco_linha(GfxRect r, float a);
// Degrade horizontal preto de `x0` (transparente) a `x1` (`forca`), da altura
// inteira da tela: a Home escurecendo ate a borda da gaveta.
void r8_escuro_lateral(float x0, float x1, float forca);
// Veu preto de fundo (a Home recuada escurecendo): `forca` 0..1.
void r8_veu(float forca);

#endif
