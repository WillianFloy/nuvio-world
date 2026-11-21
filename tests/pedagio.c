// PEDAGIO DE IMAGENS (pedagio.h): quem passa, em que tamanho, e o disjuntor.
#include "../src/pedagio.h"
#include <stdio.h>
#include <string.h>
static int falhas;
#define OK(c, m) do { if (c) printf("ok    %s\n", m); else { printf("FALHA %s\n", m); falhas++; } } while (0)
int main(void) {
  char u[1200];
  const char *gh = "https://raw.githubusercontent.com/exemplo/colecoes/main/pasta/Serviços & Co.png";
  OK(pedagio_url(gh, 192, u, sizeof u, 1000), "arte do GitHub passa pelo pedagio");
  printf("      %s\n", u);
  OK(!strncmp(u, "https://wsrv.nl/?url=https://raw.githubusercontent.com/", 55), "URL original vai no parametro url");
  OK(strstr(u, "Servi%C3%A7os%20%26%20Co.png") != NULL, "acento, espaco e & codificados (nao quebram a URL)");
  OK(strstr(u, "&w=256&we&output=webp&q=85") != NULL, "teto 192 -> degrau 256, sem ampliar, WebP");
  OK(pedagio_url(gh, 320, u, sizeof u, 1000) && strstr(u, "&w=384&"), "teto 320 -> degrau 384");
  OK(pedagio_url(gh, 1280, u, sizeof u, 1000) && strstr(u, "&w=1280&"), "teto 1280 -> degrau 1280");
  OK(!pedagio_url(gh, 2560, u, sizeof u, 1000), "teto acima de 1920: direto");
  OK(!pedagio_url("https://image.tmdb.org/t/p/original/x.jpg", 300, u, sizeof u, 1000), "TMDB (tem tamanhos proprios): direto");
  OK(!pedagio_url("https://images.metahub.space/poster/medium/tt1/img", 300, u, sizeof u, 1000), "metahub: direto");
  OK(!pedagio_url("https://exemplo.org/anim.GIF?x=1", 300, u, sizeof u, 1000), "GIF: direto (animacao)");
  OK(!pedagio_url("http://192.168.1.10:8080/a.png", 300, u, sizeof u, 1000), "rede local: direto");
  OK(!pedagio_url("http://localhost/a.png", 300, u, sizeof u, 1000), "localhost: direto");
  OK(!pedagio_url("/data/app/arte.png", 300, u, sizeof u, 1000), "arquivo local: direto");
  OK(!pedagio_url("https://wsrv.nl/?url=x.png", 300, u, sizeof u, 1000), "ja e do pedagio: nao repete");
  OK(pedagio_eh("https://wsrv.nl/?url=a") && !pedagio_eh(gh), "pedagio_eh reconhece a URL do servico");
  // Disjuntor.
  pedagio_resultado(0, 2000); pedagio_resultado(0, 2001);
  OK(pedagio_url(gh, 192, u, sizeof u, 2002), "duas falhas: continua");
  pedagio_resultado(0, 2003);
  OK(!pedagio_url(gh, 192, u, sizeof u, 2004), "tres falhas seguidas: desligado (vai direto ao original)");
  OK(!pedagio_url(gh, 192, u, sizeof u, 2003 + 9u * 60000u), "ainda desligado aos 9 min");
  OK(pedagio_url(gh, 192, u, sizeof u, 2003 + 10u * 60000u + 1), "volta depois de 10 min");
  pedagio_resultado(0, 700000); pedagio_resultado(1, 700001); pedagio_resultado(0, 700002); pedagio_resultado(0, 700003);
  OK(pedagio_url(gh, 192, u, sizeof u, 700004), "um sucesso no meio zera a contagem");
  { PedagioStats s; pedagio_stats(&s, 1);
    OK(s.pedidos == 7 && s.ok == 1 && s.falhas == 6 && s.desligado == 1, "contadores do log [pedagio]"); }
  pedagio_ligado = 0;
  OK(!pedagio_url(gh, 192, u, sizeof u, 800000), "desligado (debug.nuvio.pedagio=0): direto");
  printf("%s\n", falhas ? "pedagio: FALHOU" : "pedagio: tudo ok");
  return falhas != 0;
}
