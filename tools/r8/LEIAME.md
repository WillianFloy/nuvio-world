# tools/r8 · as pranchas aprovadas (secoes 8 e 10) como especificacao

O codigo que gerou as pranchas aprovadas pelo dono (rodada 24/11) e a fonte da
verdade do redesenho: cada elemento e uma funcao do tempo (posicao, tamanho,
opacidade, escala, raio). O app tem de bater com ele.

| Arquivo | Para que serve |
|---|---|
| `base.py`, `menus.py`, `adapt.py`, `panels.py` | o gerador original (cores, icones, curvas, cada peca) |
| `pranchas.py` | refaz as 17 pranchas; `tests/r8_espec.sh` confere byte a byte com `aprovadas.sha256` |
| `exportar.py` | a especificacao numerica: cada elemento a 60 quadros/s, um `.json` por peca |
| `quadros.py` | desenha a peca no instante t (Chrome sem tela, Montserrat do app) |
| `comparar.py` | prancha x app lado a lado (imagens) ou um em cima do outro (`--video`, mp4) |
| `fase1.py`, `build_f1.py` | as 14 pranchas da secao 10 (Fase 1, aprovadas em 25/11) |
| `icones.py` | os icones das pranchas como `deploy/app/art/icones/r8_*.png` (128 px, traco 2) |

MUDANCAS PEDIDAS PELO DONO DEPOIS DA APROVACAO (os hashes em
`aprovadas.sha256` foram refeitos so por elas):
- 26/11, depois do teste na TCL: a Folha (`MC`, e a adaptacao) virou GAVETA,
  encostada na borda direita, que entra e sai deslizando pela direita, sempre
  a direita; a arte nao voa mais do cartaz (mora no topo e anda junto); a Home
  escurece ate a beirada (degrade de 280 px, 90 %). O foco das linhas dos
  menus (Folha e Desdobrar) ficou mais leve: raio 10, veu 10 %, contorno de
  1,5 px a 50 %. Mudam R8-MC-1/2, R8-MA-1/2 e R8-M-comparativo.

Pecas: `MC` Folha (padrao), `MA` Desdobrar, `S1` Estante (padrao), `S2` Coluna,
`C2` Central, `HO`/`HR` segurar CH+ sem/com relogio, `BU` Busca antiga (so
referencia); da secao 10: `BF`/`BM` Busca flutuante (a aprovada), `FHR`/`FHO`
segurar CH+ ate a Coluna, `FA` abas da Estante, `F2` S2 com abas, `FC` Central
com Editar atalhos.

No app (Fase 2): `src/r8tempo.c` (curvas e tempos; `tests/r8_tempo.sh` confere
quadro a quadro com esta especificacao), `src/r8estilo.c` (material, foco,
raios, Grafite/Preto puro), estilos `TXT_R8_*` (corpo e peso; 800 = Montserrat
ExtraBold) e `gfx_snap_desenhar_xf`/`_faixa` (a Home congelada recuada e a
faixa do Desdobrar); `tests/r8_fundacao_shot.sh` desenha e confere.

Fluxo:

    bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4
    python3 tools/r8/comparar.py BU /tmp/r8-app /tmp/r8-cmp 0.8 0.95 1.1 1.4

Requer `python3` com Pillow e Playwright, Google Chrome e ffmpeg (so para `--video`).
