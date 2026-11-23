// World (23/11): o master reduzido da Apple (montarReduzido, trailerapple.c real) leva TODOS os audios do grupo da
// variante e as legendas do grupo SUBTITLES dela, para o trailer trocar de faixa. Antes: um audio, nenhuma legenda
// (log bg824y: "faixas: 1 audio, 0 legenda"). Sem rede: o master e um arquivo escrito pelo teste.
//   1. os 2 audios do grupo da variante entram; o de outro grupo nao;
//   2. as 3 legendas do grupo entram; a de URI relativa nao (o reduzido e file://);
//   3. a variante escolhida continua a mesma (1080p avc1 no Android) e cita o grupo de legendas;
//   4. master sem legendas: o STREAM-INF deixa de citar SUBTITLES.
#include "../src/trailerapple.c"
#include <stdlib.h>
static int falhas;
static void ok(int c, const char *m) { printf("%s: %s\n", c ? "PASS" : "FAIL", m); falhas += !c; }
char *rede_baixar_com(const char *url, int s, const char *const *cab) { (void)url; (void)s; (void)cab; return NULL; }
char *rede_baixar(const char *url, int s) { (void)url; (void)s; return NULL; }
const char *desc_chave_tmdb_reserva(void) { return "K"; }
static char pasta[256];
char *dados_caminho(char *dst, unsigned tam, const char *nome) {
  snprintf(dst, tam, "%s/%s", pasta, nome); mkdir(dst, 0700); return dst; }
void dados_marcar_sujo(int leve) { (void)leve; }
int ajustes_trailer_qualidade(void) { return 0; }
static char *lerTudo(const char *c) {
  FILE *f = fopen(c, "rb"); long n; char *b;
  if (!f) return NULL;
  fseek(f, 0, SEEK_END); n = ftell(f); rewind(f);
  b = calloc(1, (size_t)n + 1); if (fread(b, 1, (size_t)n, f) != (size_t)n) {} fclose(f); return b; }
static int conta(const char *s, const char *agulha) { int n = 0; for (; s && (s = strstr(s, agulha)); s++) n++; return n; }
static void escreverMaster(const char *id, const char *txt) {
  char c[600]; FILE *f; caminhoMaster(id, c, sizeof c, ".m3u8"); f = fopen(c, "w"); fputs(txt, f); fclose(f); }
#define A(g, l, u) "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"" g "\",LANGUAGE=\"" l "\",NAME=\"" l "\",DEFAULT=YES,URI=\"" u "\"\n"
#define L(l, u) "#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID=\"subs\",LANGUAGE=\"" l "\",NAME=\"" l "\",AUTOSELECT=YES,URI=\"" u "\"\n"
int main(void) {
  char toca[700], cr[600], *r;
  snprintf(pasta, sizeof pasta, "%s/nuvio-apple-faixas-XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  if (!mkdtemp(pasta)) { perror("mkdtemp"); return 1; }
  escreverMaster("tt1", "#EXTM3U\n"
    A("aac", "en", "https://a/en.m3u8") A("aac", "pt-BR", "https://a/pt.m3u8") A("ac3", "en", "https://a/en51.m3u8")
    L("en", "https://s/en.m3u8") L("pt-BR", "https://s/pt.m3u8") L("es-419", "https://s/es.m3u8") L("fr", "relativa/fr.m3u8")
    "#EXT-X-STREAM-INF:BANDWIDTH=9000000,RESOLUTION=3840x1606,CODECS=\"hvc1.2.4.L150,mp4a.40.2\",AUDIO=\"aac\",SUBTITLES=\"subs\"\nhttps://v/4k.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=5000000,RESOLUTION=1920x804,CODECS=\"avc1.640028,mp4a.40.2\",AUDIO=\"aac\",SUBTITLES=\"subs\"\nhttps://v/1080.m3u8\n"
    "#EXT-X-STREAM-INF:BANDWIDTH=2000000,RESOLUTION=1280x536,CODECS=\"avc1.64001f,mp4a.40.2\",AUDIO=\"aac\",SUBTITLES=\"subs\"\nhttps://v/720.m3u8\n");
  ok(montarReduzido("tt1", 0, toca, sizeof toca), "monta o reduzido");
  caminhoMaster("tt1", cr, sizeof cr, "-play.m3u8");
  r = lerTudo(cr);
  printf("%s", r ? r : "(nada)\n");
  ok(r && conta(r, "TYPE=AUDIO") == 2 && strstr(r, "a/en.m3u8") && strstr(r, "a/pt.m3u8") && !strstr(r, "en51"),
     "1. os 2 audios do grupo da variante (en e pt-BR), nao o de outro grupo");
  ok(r && conta(r, "DEFAULT=YES") == 1 && conta(r, "DEFAULT=NO") == 1, "1. um so DEFAULT=YES no grupo de audio");
  ok(r && conta(r, "TYPE=SUBTITLES") == 3 && strstr(r, "s/pt.m3u8") && !strstr(r, "relativa"),
     "2. as 3 legendas com URI absoluta; a relativa fica de fora");
  ok(r && conta(r, "#EXT-X-STREAM-INF") == 1 && strstr(r, "v/1080.m3u8") && strstr(r, "SUBTITLES=\"subs\""),
     "3. uma variante (1080p avc1) citando o grupo de legendas");
  free(r);
  escreverMaster("tt2", "#EXTM3U\n" A("aac", "en", "https://a/en.m3u8")
    "#EXT-X-STREAM-INF:BANDWIDTH=5000000,RESOLUTION=1920x804,CODECS=\"avc1.640028,mp4a.40.2\",AUDIO=\"aac\",SUBTITLES=\"subs\",CLOSED-CAPTIONS=NONE\nhttps://v/1080.m3u8\n");
  ok(montarReduzido("tt2", 0, toca, sizeof toca), "monta o reduzido sem legendas");
  caminhoMaster("tt2", cr, sizeof cr, "-play.m3u8");
  r = lerTudo(cr);
  printf("%s", r ? r : "(nada)\n");
  ok(r && !strstr(r, "SUBTITLES") && strstr(r, "AUDIO=\"aac\",CLOSED-CAPTIONS=NONE") && conta(r, "TYPE=AUDIO") == 1,
     "4. sem legendas: o STREAM-INF nao cita SUBTITLES (o resto da linha fica)");
  free(r);
  printf("%s\n", falhas ? "trailerapple_faixas: FALHOU" : "trailerapple_faixas: ok");
  return falhas != 0;
}
