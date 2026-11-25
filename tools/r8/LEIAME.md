# tools/r8 · as pranchas aprovadas da secao 8 como especificacao

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

Pecas: `MC` Folha (padrao), `MA` Desdobrar, `S1` Estante (padrao), `S2` Coluna,
`C2` Central, `HO`/`HR` segurar CH+ sem/com relogio, `BU` Busca (referencia: a
abertura do meio sai da Fase 1).

Fluxo:

    bash tests/r8_app_quadros_shot.sh BU /tmp/r8-app 0.8 0.95 1.1 1.4
    python3 tools/r8/comparar.py BU /tmp/r8-app /tmp/r8-cmp 0.8 0.95 1.1 1.4

Requer `python3` com Pillow e Playwright, Google Chrome e ffmpeg (so para `--video`).
