// Ver logrede.h para o porque e as regras.
#include "logrede.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

// --- limpeza de uma linha (vale em todo alvo, inclusive no teste) -----------

// URL cortada no host: "https://host/caminho?x" -> "https://host/…". Como o
// registro.c (mascarar): so troca quando sobra 4 bytes ou mais, nunca cresce.
static void cortarUrls(char *s) {
  char *p = s;
  while ((p = strstr(p, "://")) != NULL) {
    char *h = p + 3, *at = NULL, *fim, *q;
    for (q = h; *q && *q != '/' && *q != '?' && *q != ' ' && *q != '"' && *q != '\''; q++)
      if (*q == '@') at = q;
    // userinfo (usuario:senha@) fora, como rede_url_publica faz.
    if (at) { memmove(h, at + 1, strlen(at + 1) + 1); q -= at + 1 - h; }
    if (*q != '/' && *q != '?') { p = q; continue; }
    fim = q;
    while (*fim && *fim != ' ' && *fim != '"' && *fim != '\'' && *fim != '>' && *fim != ')') fim++;
    if (fim - q < 4) { p = fim; continue; }
    memmove(q + 4, fim, strlen(fim) + 1);
    memcpy(q, "/\xe2\x80\xa6", 4);
    p = q + 4;
  }
}

// Esquema sem "//" (magnet:, etc.): se o valor tem '=' (parametros), tudo do
// ':' ao fim do token vira "…". Em magnet: caem xt (hash), dn (titulo) e
// tr (rastreador) — os tres sensiveis.
static void cortarEsquemas(char *s) {
  for (char *p = s; *p; p++) {
    char *a = p, *fim;
    if (!isalpha((unsigned char)*p)) continue;
    if (p > s && (isalnum((unsigned char)p[-1]) || p[-1] == '+' || p[-1] == '-' ||
                  p[-1] == '.' || p[-1] == ':')) continue;
    while (isalnum((unsigned char)*p) || *p == '+' || *p == '-' || *p == '.') p++;
    if (*p != ':' || p - a < 2 || p - a > 16 || p[1] == '/' || p[1] == ':') continue;
    fim = p + 1;
    while (*fim && *fim != ' ' && *fim != '"' && *fim != '\'' && *fim != '>' && *fim != ')') fim++;
    if (!memchr(p + 1, '=', (size_t)(fim - p - 1)) || fim - p < 5) continue;
    memmove(p + 4, fim, strlen(fim) + 1);
    memcpy(p, ":\xe2\x80\xa6", 4);
    p += 4;
  }
}

// "apikey=XYZ", "token: XYZ", "Authorization: Bearer XYZ" -> valor "…".
static void cortarSegredos(char *s) {
  static const char *const CH[] = { "apikey", "api_key", "api-key", "token", "password", "passwd",
                                    "senha", "secret", "authorization", "bearer", "auth", "key",
                                    "pass", "cookie", "sessionid", "signature", "sig" };
  for (size_t k = 0; k < sizeof CH / sizeof *CH; k++) {
    size_t n = strlen(CH[k]);
    char *p = s;
    while (*p) {
      char *q, *v, *fim;
      if (strncasecmp(p, CH[k], n)) { p++; continue; }
      // palavra inteira a esquerda ("monkey=" nao conta como "key=")
      if (p > s && (isalnum((unsigned char)p[-1]) || p[-1] == '_')) { p += n; continue; }
      q = p + n;
      if (*q == '"' || *q == '\'') q++;
      if (!strcasecmp(CH[k], "bearer")) { if (*q != ' ') { p = q; continue; } }
      else if (*q != '=' && *q != ':') { p = q; continue; }
      else q++;
      while (*q == ' ' || *q == '"' || *q == '\'') q++;
      if (!strncasecmp(q, "bearer ", 7)) q += 7;
      v = q; fim = v;
      while (*fim && *fim != ' ' && *fim != '&' && *fim != '"' && *fim != '\'' && *fim != ',' &&
             *fim != ';' && *fim != '}' && *fim != ')') fim++;
      if (fim - v >= 3) {
        memmove(v + 3, fim, strlen(fim) + 1);
        memcpy(v, "\xe2\x80\xa6", 3);
        fim = v + 3;
      }
      p = fim;
    }
  }
}

// Sequencia longa de letras E numeros (chave de debrid, hash, token): fica o
// comeco, o resto vira "…".
static void cortarLongas(char *s) {
  char *p = s;
  while (*p) {
    char *a = p;
    // Sequencia longa do alfabeto de token/chave (letras, numeros e os
    // extras de base64): mesmo sem digito pode ser uma chave (hex de
    // rastreador, token so de letras), entao basta o comprimento.
    while (isalnum((unsigned char)*p) || *p == '_' || *p == '+' || *p == '/' ||
           *p == '=' || *p == '-') p++;
    if (p - a >= 32) {
      memmove(a + 7, p, strlen(p) + 1);
      memcpy(a + 4, "\xe2\x80\xa6", 3);
      p = a + 7;
    }
    if (*p) p++;
  }
}

void logrede_limpar(char *linha) {
  if (!linha) return;
  cortarUrls(linha);
  cortarEsquemas(linha);
  cortarSegredos(linha);
  cortarLongas(linha);
}

#ifdef __EMSCRIPTEN__
void logrede_servidor(int ligado) { (void)ligado; }
const char *logrede_base(void) { return ""; }
int logrede_ativo(void) { return 0; }
#else

#include "diaglog.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define LR_CAB_MAX 4096

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_t fio;
static int vivo, ouvir = -1, acorda[2] = { -1, -1 }, porta;
static char base[96];
static time_t proximaTentativa, ipQuando;

static int privado(in_addr_t a) {
  unsigned h = ntohl(a);
  // 10/8, 172.16/12, 192.168/16, 127/8, e tambem 100.64/10 (CGNAT de mesh e
  // de operadora) e 169.254/16 (link-local): redes de casa que nao sao RFC 1918.
  return (h >> 24) == 10 || (h >> 20) == 0xAC1 || (h >> 16) == 0xC0A8 || (h >> 24) == 127 ||
         (h >> 22) == (0x64400000u >> 22) || (h >> 16) == 0xA9FE;
}

// O IP da TV na rede de casa (a mesma conta de celular.c).
static int ipLan(char *dst, size_t n) {
  struct sockaddr_in alvo, eu;
  socklen_t tam = sizeof eu;
  struct ifaddrs *lista = NULL, *i;
  int s = socket(AF_INET, SOCK_DGRAM, 0), ok = 0;
  in_addr_t achou = 0;
  const char *fixo = getenv("NUVIO_LOGREDE_IP");   // testes: o endereco anunciado
  if (fixo && *fixo) { snprintf(dst, n, "%s", fixo); if (s >= 0) close(s); return 1; }
  if (s >= 0) {
    memset(&alvo, 0, sizeof alvo);
    alvo.sin_family = AF_INET; alvo.sin_port = htons(9);
    inet_pton(AF_INET, "192.0.2.1", &alvo.sin_addr);
    if (!connect(s, (struct sockaddr *)&alvo, sizeof alvo) &&
        !getsockname(s, (struct sockaddr *)&eu, &tam) && privado(eu.sin_addr.s_addr) &&
        (ntohl(eu.sin_addr.s_addr) >> 24) != 127)
      achou = eu.sin_addr.s_addr;
    close(s);
  }
  if (!achou && !getifaddrs(&lista)) {
    for (i = lista; i; i = i->ifa_next) {
      in_addr_t a;
      if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET) continue;
      if (!(i->ifa_flags & IFF_UP) || (i->ifa_flags & IFF_LOOPBACK)) continue;
      a = ((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr;
      if (privado(a) && (ntohl(a) >> 24) != 127) { achou = a; break; }
    }
    freeifaddrs(lista);
  }
  if (achou) { struct in_addr a; a.s_addr = achou; ok = inet_ntop(AF_INET, &a, dst, (socklen_t)n) != NULL; }
  return ok;
}

static int enviarTudo(int c, const char *b, size_t n) {
  while (n) {
    ssize_t w = send(c, b, n, MSG_NOSIGNAL);
    if (w <= 0) { if (w < 0 && errno == EINTR) continue; return -1; }
    b += w; n -= (size_t)w;
  }
  return 0;
}

static const char *motivo(int codigo) {
  switch (codigo) {
    case 200: return "OK";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 409: return "Conflict";
    case 410: return "Gone";
    default:  return "Bad Request";
  }
}
static void responder(int c, int codigo, const char *tipo, const char *extra, const char *corpo, size_t n) {
  char cab[700];
  int k = snprintf(cab, sizeof cab,
                   "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nCache-Control: no-store\r\n"
                   "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nConnection: close\r\n%s\r\n",
                   codigo, motivo(codigo), tipo, (unsigned long)n, extra ? extra : "");
  if (enviarTudo(c, cab, (size_t)k) == 0 && n) enviarTudo(c, corpo, n);
}

// A PAGINA DA SESSAO: uma so, responsiva (celular e computador), sem nada de
// fora (nem fonte, nem script): a TV e a rede de casa bastam. Ela le
// estado.json a cada segundo e desenha o estado; o relogio anda sozinho entre
// as leituras. O botao baixa log.txt (Content-Disposition: attachment).
static const char PAGINA[] =
"<!doctype html><html><head><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1,viewport-fit=cover'>"
"<meta name=robots content=noindex><title>Nuvio logs</title><style>"
":root{color-scheme:dark;--bg:#0b0c0f;--card:#16181d;--fg:#f3f2ef;--dim:#9a9ca3;--line:#262931;--rec:#ef4f43;--ok:#4cc38a;--warn:#e8b84a}"
"*{box-sizing:border-box}html,body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}"
"main{max-width:560px;margin:0 auto;padding:max(24px,env(safe-area-inset-top)) 20px 32px}"
".card{background:var(--card);border:1px solid var(--line);border-radius:20px;padding:24px}"
".top{display:flex;align-items:center;gap:10px;font-size:13px;letter-spacing:.08em;text-transform:uppercase;color:var(--dim)}"
"#dot{width:12px;height:12px;border-radius:50%;background:var(--dim);flex:none}"
"#dot.rec{background:var(--rec);animation:p 1.4s ease-in-out infinite}#dot.ok{background:var(--ok)}#dot.warn{background:var(--warn)}"
"@keyframes p{50%{opacity:.35}}"
"h1{font-size:22px;margin:14px 0 4px;font-weight:650}"
"#big{font:600 clamp(56px,18vw,88px)/1.05 ui-monospace,'SF Mono',Menlo,Consolas,monospace;letter-spacing:-.02em;margin:10px 0 6px;font-variant-numeric:tabular-nums}"
"#lbl{color:var(--dim);font-size:14px;margin-bottom:14px}"
"#sub{color:#c9cad0;margin:0 0 20px}"
".btn{display:block;width:100%;text-align:center;padding:16px 18px;border-radius:14px;font-weight:650;font-size:17px;text-decoration:none;"
"background:var(--fg);color:#0b0c0f;border:0}"
".btn.off{background:#2a2d35;color:#7d8088;pointer-events:none}"
"#meta{color:var(--dim);font-size:13px;margin:14px 0 0;word-break:break-word}"
"footer{color:#6c6f76;font-size:12px;text-align:center;margin-top:18px}"
"@media (min-width:720px){main{padding-top:64px}.card{padding:36px}h1{font-size:26px}}"
"</style></head><body><main><div class=card>"
"<div class=top><span id=dot></span><span id=st>&hellip;</span></div>"
"<h1 id=tt>Nuvio</h1><div id=big>--:--</div><div id=lbl></div><p id=sub></p>"
"<a id=dl class='btn off' href='log.txt' download>Baixar log.txt</a><p id=meta></p>"
"</div><footer id=ft></footer></main><script>"
"var T={pt:{tt:'Logs de diagn\\u00f3stico',g:'Gravando',gl:'tempo de grava\\u00e7\\u00e3o',"
"gs:'Os logs ainda est\\u00e3o sendo coletados na TV. Use o app normalmente e escolha \\u201cFinalizar logs\\u201d na TV quando terminar.',"
"sv:'Salvando',svs:'A TV est\\u00e1 salvando o arquivo\\u2026',p:'Pronto para baixar',pl:'o endere\\u00e7o fecha em',"
"ps:'Toque em \\u201cBaixar log.txt\\u201d antes de a contagem chegar a zero.',e:'Sess\\u00e3o encerrada',"
"es:'Este endere\\u00e7o n\\u00e3o est\\u00e1 mais ativo. Para gravar de novo, inicie uma nova sess\\u00e3o na TV.',"
"x:'Sem resposta da TV',xs:'Tentando de novo\\u2026 Confira se este aparelho est\\u00e1 na mesma rede da TV.',"
"dl:'Baixar log.txt',dlw:'Dispon\\u00edvel depois de finalizar na TV',ln:'linhas',fim:'O download j\\u00e1 iniciado continua at\\u00e9 o fim.'},"
"en:{tt:'Diagnostic logs',g:'Recording',gl:'recording time',"
"gs:'Logs are still being collected on the TV. Use the app normally and choose \\u201cFinish logs\\u201d on the TV when you are done.',"
"sv:'Saving',svs:'The TV is saving the file\\u2026',p:'Ready to download',pl:'this address closes in',"
"ps:'Tap \\u201cDownload log.txt\\u201d before the countdown reaches zero.',e:'Session ended',"
"es:'This address is no longer active. To record again, start a new session on the TV.',"
"x:'The TV is not answering',xs:'Retrying\\u2026 Check that this device is on the same network as the TV.',"
"dl:'Download log.txt',dlw:'Available after finishing on the TV',ln:'lines',fim:'A download that already started keeps going until it ends.'}};"
"var L=T.pt,s=null,em=0,falhas=0,baixou=0,fimLocal=0;"
"function $(i){return document.getElementById(i)}"
"function mmss(ms){ms=Math.max(0,ms);var t=Math.floor(ms/1000),m=Math.floor(t/60),x=t%60;return (m<10?'0':'')+m+':'+(x<10?'0':'')+x}"
"function cd(ms){var t=Math.max(0,Math.ceil(ms/1000)),m=Math.floor(t/60),x=t%60;return m+':'+(x<10?'0':'')+x}"
"function kb(b){return b<1048576?Math.max(1,Math.round(b/1024))+' KB':(b/1048576).toFixed(1)+' MB'}"
"function pinta(){var d=$('dot'),dl=$('dl'),st=s?s.estado:'',agora=Date.now()-em;"
"$('tt').textContent=L.tt;dl.textContent=L.dl;d.className='';"
"if(st==='pronto'){var r=s.restante-agora;if(r<=0){fimLocal=1;st='encerrada'}}"
"if(!s&&falhas){st='x'}"
"if(st==='gravando'){d.className='rec';$('st').textContent=L.g;$('big').textContent=mmss(s.decorrido+agora);$('lbl').textContent=L.gl;"
"$('sub').textContent=falhas>2?L.xs:L.gs;dl.className='btn off';$('meta').textContent=L.dlw+(s.linhas?' \\u00b7 '+s.linhas+' '+L.ln:'')}"
"else if(st==='salvando'){d.className='warn';$('st').textContent=L.sv;$('big').textContent=mmss(s.decorrido);$('lbl').textContent=L.gl;$('sub').textContent=L.svs;dl.className='btn off';$('meta').textContent=''}"
"else if(st==='pronto'){d.className='ok';$('st').textContent=L.p;$('big').textContent=cd(s.restante-agora);$('lbl').textContent=L.pl;$('sub').textContent=L.ps;"
"dl.className='btn';$('meta').textContent=s.nome+' \\u00b7 '+kb(s.bytes)+' \\u00b7 '+s.linhas+' '+L.ln+' \\u00b7 '+mmss(s.decorrido)}"
"else if(st==='x'){d.className='warn';$('st').textContent=L.x;$('big').textContent='--:--';$('lbl').textContent='';$('sub').textContent=L.xs;dl.className='btn off';$('meta').textContent=''}"
"else if(st==='encerrada'){$('st').textContent=L.e;$('big').textContent='0:00';$('lbl').textContent='';$('sub').textContent=L.es+(baixou?' '+L.fim:'');dl.className='btn off';$('meta').textContent=''}"
"$('ft').textContent='Nuvio World'+(s&&s.aparelho?' \\u00b7 '+s.aparelho:'')}"
"function le(){if(fimLocal){pinta();return}fetch('estado.json',{cache:'no-store'}).then(function(r){return r.json()})"
".then(function(j){s=j;em=Date.now();falhas=0;L=j.pt?T.pt:T.en;document.documentElement.lang=j.pt?'pt-BR':'en'})"
".catch(function(){falhas++;if(s&&s.estado==='pronto'&&s.restante-(Date.now()-em)<=0)fimLocal=1;else if(falhas>2)s=s&&s.estado==='gravando'?s:null})"
".then(function(){pinta();setTimeout(le,1000)})}"
"$('dl').onclick=function(){baixou=1};setInterval(pinta,250);le();"
"</script></body></html>";

static const char ENCERRADA[] =
"<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Nuvio logs</title><style>body{margin:0;background:#0b0c0f;color:#f3f2ef;font:16px/1.5 system-ui,sans-serif;"
"display:flex;min-height:100vh;align-items:center;justify-content:center;padding:24px;box-sizing:border-box;text-align:center}"
"b{display:block;font-size:22px;margin-bottom:8px}span{color:#9a9ca3}</style></head><body><div>"
"<b>Sess\xc3\xa3o encerrada &middot; Session ended</b><span>Este endere\xc3\xa7o n\xc3\xa3o est\xc3\xa1 mais ativo. Inicie uma nova sess\xc3\xa3o na TV."
"<br>This address is no longer active. Start a new session on the TV.</span></div></body></html>";

static void jsonTexto(char *d, size_t n, const char *s) {
  size_t k = 0;
  for (; s && *s && k + 7 < n; s++) {
    unsigned char ch = (unsigned char)*s;
    if (ch == '"' || ch == '\\') { d[k++] = '\\'; d[k++] = (char)ch; }
    else if (ch < 0x20) k += (size_t)snprintf(d + k, n - k, "\\u%04x", ch);
    else d[k++] = (char)ch;
  }
  d[k] = 0;
}

// A sessao vale se o codigo do caminho e o da sessao atual e, pronta, o prazo
// nao passou (o servidor fecha no quadro seguinte; ate la, nada novo entra).
static int sessaoValida(const DiagInfo *i, const char *cod, size_t n) {
  if (i->estado == DIAG_PARADO || !i->codigo[0] || strlen(i->codigo) != n || strncmp(i->codigo, cod, n)) return 0;
  if (i->estado == DIAG_PRONTO && i->restanteMs <= 0) return 0;
  return 1;
}

static void responderEstado(int c, const DiagInfo *i, int valida) {
  char j[900], ap[200], nome[200];
  int k;
  const char *est = !valida ? "encerrada" : i->estado == DIAG_GRAVANDO ? "gravando" : i->estado == DIAG_SALVANDO ? "salvando" : "pronto";
  jsonTexto(ap, sizeof ap, i->aparelho);
  jsonTexto(nome, sizeof nome, valida ? i->nome : "");
  k = snprintf(j, sizeof j,
               "{\"estado\":\"%s\",\"decorrido\":%ld,\"restante\":%ld,\"janela\":%ld,\"linhas\":%ld,\"bytes\":%ld,"
               "\"nome\":\"%s\",\"pt\":%d,\"aparelho\":\"%s\"}",
               est, valida ? i->decorridoMs : 0, valida ? i->restanteMs : 0, DIAG_JANELA_MS,
               valida ? i->linhas : 0, valida ? i->bytes : 0, nome, i->pt, ap);
  responder(c, valida ? 200 : 410, "application/json; charset=utf-8", NULL, j, (size_t)k);
}

// O arquivo inteiro, em pedacos: o download que comecou dentro do prazo vai ate
// o fim mesmo que o prazo acabe no meio (o fio desta conexao e dele).
static void responderArquivo(int c, const DiagInfo *i) {
  char cab[900], buf[65536];
  struct stat st;
  int k;
  FILE *f = fopen(i->arquivo, "rb");
  if (!f || fstat(fileno(f), &st) != 0) {
    static const char msg[] = "log file not available\n";
    if (f) fclose(f);
    responder(c, 410, "text/plain; charset=utf-8", NULL, msg, sizeof msg - 1);
    return;
  }
  k = snprintf(cab, sizeof cab,
               "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %lu\r\n"
               "Content-Disposition: attachment; filename=\"%s\"\r\nCache-Control: no-store\r\n"
               "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nConnection: close\r\n\r\n",
               (unsigned long)st.st_size, i->nome);
  printf("[logrede] download %s (%ld KB) iniciado\n", i->nome, (long)(st.st_size + 1023) / 1024);
  fflush(stdout);
  if (enviarTudo(c, cab, (size_t)k) == 0) {
    size_t n;
    long total = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
      if (enviarTudo(c, buf, n) != 0) break;
      total += (long)n;
    }
    printf("[logrede] download %s: %ld de %ld bytes enviados\n", i->nome, total, (long)st.st_size);
    fflush(stdout);
  }
  fclose(f);
}

static void atender(int c) {
  char cab[LR_CAB_MAX + 1], metodo[8], caminho[256];
  size_t k = 0;
  struct sockaddr_in par;
  socklen_t tp = sizeof par;
  struct timeval tv = { 2, 0 }, tvs = { 15, 0 };
  DiagInfo info;
  setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, &tvs, sizeof tvs);
#ifdef SO_NOSIGPIPE
  { int um = 1; setsockopt(c, SOL_SOCKET, SO_NOSIGPIPE, &um, sizeof um); }
#endif
  memset(&par, 0, sizeof par);
  if (getpeername(c, (struct sockaddr *)&par, &tp) != 0 || par.sin_family != AF_INET ||
      !privado(par.sin_addr.s_addr)) {
    // Com o motivo na tela: uma pagina em branco parecia "quebrado".
    char msg[200], ip[48] = "?";
    int n;
    if (par.sin_family == AF_INET) inet_ntop(AF_INET, &par.sin_addr, ip, sizeof ip);
    n = snprintf(msg, sizeof msg, "Nuvio: this page only answers devices on the home network (your address: %s).\n", ip);
    printf("[logrede] refused %s (not a home-network address)\n", ip);
    fflush(stdout);
    responder(c, 403, "text/plain; charset=utf-8", NULL, msg, (size_t)n);
    return;
  }
  while (k < LR_CAB_MAX) {
    ssize_t r = recv(c, cab + k, LR_CAB_MAX - k, 0);
    if (r <= 0) break;
    k += (size_t)r; cab[k] = 0;
    if (strstr(cab, "\r\n\r\n")) break;
  }
  cab[k] = 0;
  if (sscanf(cab, "%7s %255s", metodo, caminho) != 2 || (strcmp(metodo, "GET") && strcmp(metodo, "HEAD"))) {
    responder(c, 400, "text/plain", NULL, "", 0);
    return;
  }
  { char *q = strchr(caminho, '?'); if (q) *q = 0; }
  diaglog_info(&info);
  // Raiz (quem digitou so o IP): leva a sessao atual, se houver.
  if (!strcmp(caminho, "/")) {
    if (info.estado != DIAG_PARADO && info.codigo[0] && !(info.estado == DIAG_PRONTO && info.restanteMs <= 0)) {
      char ex[64];
      snprintf(ex, sizeof ex, "Location: /s/%s/\r\n", info.codigo);
      responder(c, 302, "text/plain", ex, "", 0);
    } else responder(c, 410, "text/html; charset=utf-8", NULL, ENCERRADA, sizeof ENCERRADA - 1);
    return;
  }
  if (!strncmp(caminho, "/s/", 3)) {
    const char *cod = caminho + 3, *barra = strchr(cod, '/');
    size_t n = barra ? (size_t)(barra - cod) : strlen(cod);
    const char *resto = barra ? barra : "";
    int valida = n > 0 && n < sizeof info.codigo && sessaoValida(&info, cod, n);
    if (!*resto) {   // /s/<codigo> sem a barra: os caminhos relativos da pagina precisam dela
      char ex[64];
      if (!valida) { responder(c, 410, "text/html; charset=utf-8", NULL, ENCERRADA, sizeof ENCERRADA - 1); return; }
      snprintf(ex, sizeof ex, "Location: /s/%s/\r\n", info.codigo);
      responder(c, 301, "text/plain", ex, "", 0);
      return;
    }
    if (!strcmp(resto, "/estado.json")) { responderEstado(c, &info, valida); return; }
    if (!valida) { responder(c, 410, "text/html; charset=utf-8", NULL, ENCERRADA, sizeof ENCERRADA - 1); return; }
    if (!strcmp(resto, "/")) { responder(c, 200, "text/html; charset=utf-8", NULL, PAGINA, sizeof PAGINA - 1); return; }
    if (!strcmp(resto, "/log.txt")) {
      if (info.estado != DIAG_PRONTO) {
        static const char ainda[] = "Os logs ainda estao sendo coletados. Finalize na TV para baixar.\n"
                                    "Logs are still being collected. Finish on the TV to download.\n";
        responder(c, 409, "text/plain; charset=utf-8", NULL, ainda, sizeof ainda - 1);
        return;
      }
      if (!strcmp(metodo, "HEAD")) { responder(c, 200, "text/plain; charset=utf-8", NULL, "", 0); return; }
      responderArquivo(c, &info);
      return;
    }
  }
  responder(c, 404, "text/plain", NULL, "", 0);
}

// UM FIO POR CONEXAO, no maximo LR_CONEXOES ao mesmo tempo. Com um fio so, a
// conexao que o Chrome abre adiantada e nao usa (preconnect) segurava o
// atendimento ate o tempo de leitura acabar, e as seguintes ficavam na fila:
// a pagina "carregava sem nunca abrir". Os fios sao soltos (detached): desligar
// o servidor fecha so a porta de escuta; quem ja estava baixando termina.
#define LR_CONEXOES 6
static pthread_mutex_t contTrava = PTHREAD_MUTEX_INITIALIZER;
static int emAtendimento;
static void *atenderFio(void *u) {
  int c = (int)(intptr_t)u;
  atender(c);
  shutdown(c, SHUT_WR);
  close(c);
  pthread_mutex_lock(&contTrava); emAtendimento--; pthread_mutex_unlock(&contTrava);
  return NULL;
}

static void *servir(void *u) {
  (void)u;
  for (;;) {
    struct pollfd p[2];
    int c, cheio;
    pthread_t t;
    pthread_attr_t at;
    p[0].fd = ouvir; p[0].events = POLLIN; p[0].revents = 0;
    p[1].fd = acorda[0]; p[1].events = POLLIN; p[1].revents = 0;
    if (poll(p, 2, 1000) < 0) { if (errno == EINTR) continue; break; }
    if (p[1].revents) break;
    if (!(p[0].revents & POLLIN)) continue;
    c = accept(ouvir, NULL, NULL);
    if (c < 0) continue;
    fcntl(c, F_SETFD, FD_CLOEXEC);
    pthread_mutex_lock(&contTrava);
    cheio = emAtendimento >= LR_CONEXOES;
    if (!cheio) emAtendimento++;
    pthread_mutex_unlock(&contTrava);
    if (cheio) { close(c); continue; }
    pthread_attr_init(&at);
    pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&t, &at, atenderFio, (void *)(intptr_t)c) != 0) {
      close(c);
      pthread_mutex_lock(&contTrava); emAtendimento--; pthread_mutex_unlock(&contTrava);
    }
    pthread_attr_destroy(&at);
  }
  return NULL;
}

static void atualizarBase(void) {
  char ip[64], b[96];
  if (ipLan(ip, sizeof ip)) snprintf(b, sizeof b, "http://%s:%d", ip, porta);
  else b[0] = 0;
  pthread_mutex_lock(&trava);
  snprintf(base, sizeof base, "%s", b);
  pthread_mutex_unlock(&trava);
  ipQuando = time(NULL);
}

// PORTA 8099 PRIMEIRO: e a que o dono ja abriu nesta TV. Ocupada: o sistema
// escolhe uma livre, como o "Digitar pelo celular" (celular.c) — o endereco
// certo vai no QR e na tela.
static int subir(void) {
  struct sockaddr_in a;
  socklen_t al = sizeof a;
  int s, um = 1;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) return 0;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);
  fcntl(s, F_SETFD, FD_CLOEXEC);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons(LOGREDE_PORTA);
  if (bind(s, (struct sockaddr *)&a, sizeof a) != 0) {
    a.sin_port = 0;
    if (bind(s, (struct sockaddr *)&a, sizeof a) != 0) { close(s); return 0; }
  }
  if (listen(s, 16) != 0 || getsockname(s, (struct sockaddr *)&a, &al) != 0) { close(s); return 0; }
  if (pipe(acorda) != 0) { close(s); acorda[0] = acorda[1] = -1; return 0; }
  ouvir = s; porta = ntohs(a.sin_port);
  if (pthread_create(&fio, NULL, servir, NULL) != 0) {
    close(ouvir); ouvir = -1; close(acorda[0]); close(acorda[1]); acorda[0] = acorda[1] = -1;
    return 0;
  }
  vivo = 1;
  atualizarBase();
  printf("[logrede] diagnostic page on the local network: %s\n", base[0] ? base : "(no local IP)");
  fflush(stdout);
  return 1;
}

static void descer(void) {
  if (!vivo) return;
  if (write(acorda[1], "x", 1) < 0) { /* o poll acorda pelo timeout */ }
  pthread_join(fio, NULL);
  close(ouvir); ouvir = -1;
  close(acorda[0]); close(acorda[1]); acorda[0] = acorda[1] = -1;
  vivo = 0;
  pthread_mutex_lock(&trava); base[0] = 0; pthread_mutex_unlock(&trava);
  puts("[logrede] diagnostic page stopped");
  fflush(stdout);
}

void logrede_servidor(int ligado) {
  time_t agora;
  if (!ligado) { descer(); proximaTentativa = 0; return; }
  agora = time(NULL);
  if (!vivo) {
    if (agora < proximaTentativa) return;
    if (!subir()) proximaTentativa = agora + 5;
    return;
  }
  // A TV pode trocar de IP (DHCP) ou so ganhar rede agora.
  if (agora - ipQuando >= (base[0] ? 30 : 3)) atualizarBase();
}

const char *logrede_base(void) {
  static char b[96];
  pthread_mutex_lock(&trava);
  snprintf(b, sizeof b, "%s", vivo ? base : "");
  pthread_mutex_unlock(&trava);
  return b;
}
int logrede_ativo(void) { return vivo; }

#endif
