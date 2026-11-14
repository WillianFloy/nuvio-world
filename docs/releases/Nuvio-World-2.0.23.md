# Nuvio World 2.0.23

## Correção das Collections

- Restaurada a leitura da lista completa `sources`, incluindo fontes `tmdb` e `trakt`.
- `catalogSources` permanece como projeção dos catálogos de addons; não substitui nem apaga as fontes diretas da coleção.
- Pastas compostas por fontes TMDB/Trakt voltam a ser consideradas válidas e permanecem visíveis na Home, mesmo sem um addon associado.
- O carregamento de "Ver tudo" e da Vitrine volta a encaminhar fontes TMDB/Trakt para os resolvedores nativos, incluindo paginação.
- As abas identificam corretamente fontes de séries TMDB/Trakt.

## Validação

- Suíte Nuvio World Collections Regression Tests aprovada.
- Testes de parser, reconciliação da Home, identidade de addons, modos de visualização e abas aprovados.
