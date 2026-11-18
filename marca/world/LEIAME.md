# Identidade Nuvio World — arquivos originais e onde cada um é usado

Originais recebidos do dono em 18/11/2026 (pasta `original/`, nunca editados):

| Arquivo original | Uso | Gerado em |
|---|---|---|
| `nuvio-symbol-light.svg` (= `branco.svg`) | símbolo branco, fundos escuros | `deploy/app/art/marcas/logo-world-simbolo.png` (502x448): login, "Preparando seu catálogo", miniatura do seletor "Logo do app" |
| `nuvio-logo-vertical-light.svg` | símbolo + "World" empilhados, branco | `deploy/app/art/marcas/logo-world-marca.png` (826x908): abertura nativa (fallback do vídeo) e prévia nos Ajustes |
| `nuvio-logo-horizontal-light.svg` | símbolo + "Nuvio World" lado a lado, branco | `deploy/app/art/marcas/logo-world-horizontal.png` (1208x346): telas de Novidades |
| `nuvio-symbol-dark.svg`, `nuvio-logo-horizontal-dark.svg`, `nuvio-logo-vertical-dark.svg` | versões escuras para fundo claro | guardadas; o app não tem tela de fundo claro hoje |
| `nuvio-app-icon-square.png` (1008x1040, fundo verde) | ícone do launcher | `android/app/src/main/res/mipmap-nodpi/ic_launcher.png` (192x192): recorte central de 1008x1008 (16 px de fundo verde em cima e embaixo, o símbolo intacto) e redução |
| `nuvio-app-icon-horizontal.png` (1920x1080, fundo verde) | banner da Android TV | `android/app/src/main/res/drawable-nodpi/banner.png` (320x180): só redução, mesma proporção 16:9 |
| `abertura-original.mp4` | vídeo de abertura | `android/app/src/main/res/raw/abertura_world.mp4`: mesmo vídeo sem áudio, H.264 High 1080p 24 fps, com quadros-chave em 7,875 s e 8,0 s para os saltos do `SplashVideo.kt` |

Os ícones verdes servem só para o launcher. As telas internas usam apenas os logos transparentes.

## Textos dos logos

Os textos "Nuvio", "World" e "Nuvio World" dos SVGs estão como `<text>` em Arial, não convertidos em curvas.
Arial não existe no ambiente de build, então `rasterizar.py` desenha esses textos em **Liberation Sans**, que tem as métricas da Arial (mesmas larguras e posições).
O desenho das letras é muito próximo, mas não idêntico ao da Arial.
Para ficar fiel ao original, envie os SVGs com o texto convertido em curvas e gere de novo:

    python3 marca/world/rasterizar.py original/nuvio-logo-vertical-light.svg /tmp/v.png 2

Depois recorte as bordas transparentes e reduza para os tamanhos da tabela.
