#include "androidcache.h"
#if defined(NV_ANDROID) || defined(NV_CACHE_TEST)
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct { char path[1200]; uint64_t bytes; time_t used; } Entry;
const char *nv_android_cache_dir(void) {
  const char *p = getenv("NUVIO_CACHE");
  return p ? p : "";
}
int nv_android_cache_name(const char *n) {
  /* Only regenerated file formats belong here; unknown names stay durable. */
  static const char *files[]={"epg-*.xml","epg-*.xml.gz","epg-*.ts","epg-x-*-p[0-9]*.bin",
    "ltv-indice.txt","ltv-indice-p[0-9]*.txt",
    "ltv-xt.bin","ltv-xt-p[0-9]*.bin","ltv-st.bin","ltv-st-p[0-9]*.bin",
    "ltv-[max][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f].bin",
    "ltv-[max][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]-p[0-9]*.bin",
    "m3u-cache-p[0-9]*.txt","plugins-c-*.txt","mkvass-*.ass",
    "noticias-*.txt","noticias2-*.txt","extras-*.txt","meta-*.txt",
    "ficha-*.json","serieaud-*.txt","catalogo-*.bin","colrow-*.bin",
    "anime-fribb.idx","anime-fribb.retry","explorar-mapa.txt","explorar-viz.txt",
    "explorar-climas.txt","frases-*.txt","recomendacoes-feed.json","recomendacoes-amigo.json"};
  if (!n || strstr(n,"..") || n[0]=='/') return 0;
  if (!strcmp(n,"cache") || !strcmp(n,"atualizacao") || !strcmp(n,"trailer") || !strcmp(n,"extras") ||
      !strcmp(n,"fontes") || !strcmp(n,"legendas") || !strcmp(n,"tendencia") ||
      !strcmp(n,"negcache.txt") || !strcmp(n,"corviva.txt") ||
      !strcmp(n,"loc-texto.txt") || !strcmp(n,"queda.txt") ||
      !strcmp(n,"queda-nativa.txt")) return 1;
  const char *subdirs[]={"atualizacao/","trailer/","extras/","fontes/","legendas/","tendencia/"};
  for (unsigned i=0;i<sizeof subdirs/sizeof *subdirs;i++)
    if (!strncmp(n,subdirs[i],strlen(subdirs[i]))) return 1;
  for (unsigned i=0;i<sizeof files/sizeof *files;i++)
    if (!fnmatch(files[i],n,FNM_PATHNAME)) return 1;
  return 0;
}
/* Walk only the dedicated cache tree, never symlinks. Temporary cache
 * writes can be evicted too: a failed rename is a recoverable cache miss. */
static int scan(const char *dir, Entry **list, size_t *n, uint64_t *bytes, int clear, int depth) {
  struct stat st;
  if (depth>24 || lstat(dir,&st) || !S_ISDIR(st.st_mode)) return 0;
  DIR *d=opendir(dir); struct dirent *e; int ok=1;
  if (!d) return 0;
  while ((e=readdir(d))) {
    char p[1200];
    if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
    if (snprintf(p,sizeof p,"%s/%s",dir,e->d_name)>=(int)sizeof p || lstat(p,&st)) { ok=0; continue; }
    if (S_ISDIR(st.st_mode)) {
      /* Update staging must survive every automatic trim until the installer
       * consumes it. Size reporting and explicit clear still include it. */
      if (list && depth==0 && !strcmp(e->d_name,"atualizacao")) continue;
      if (!scan(p,list,n,bytes,clear,depth+1)) ok=0;
      /* Retain empty cache directories: live writers reuse these paths. */
    } else if (clear) {
      if (unlink(p)) ok=0;
    } else if (S_ISREG(st.st_mode)) {
      *bytes+=(uint64_t)st.st_size;
      if (list) {
        Entry *next=realloc(*list,(*n+1)*sizeof **list);
        if (!next) { ok=0; break; }
        *list=next;
        snprintf(next[*n].path,sizeof next[*n].path,"%s",p);
        next[*n].bytes=(uint64_t)st.st_size;
        next[*n].used=st.st_mtime; (*n)++;
      }
    }
  }
  closedir(d); return ok;
}
static int oldest(const void *a,const void *b) {
  const Entry *x=a,*y=b;
  if (x->used!=y->used) return x->used<y->used ? -1:1;
  return strcmp(x->path,y->path);
}
uint64_t nv_android_cache_size(void) {
  uint64_t bytes=0; size_t n=0;
  pthread_mutex_lock(&lock);
  scan(nv_android_cache_dir(),NULL,&n,&bytes,0,0);
  pthread_mutex_unlock(&lock); return bytes;
}
uint64_t nv_android_cache_size_all(void) {
  const char *root=getenv("NUVIO_CACHE_ROOT");
  uint64_t bytes=0; size_t n=0;
  if (!root || !*root) return nv_android_cache_size();
  pthread_mutex_lock(&lock);
  scan(root,NULL,&n,&bytes,0,0);
  pthread_mutex_unlock(&lock); return bytes;
}
/* DIAGNOSTICO 2.0.35 (so medicao, nao muda o que e apagado): quanto custa cada
 * poda, o que ela apaga (imagens x fileiras das Collections x o resto) e quanto
 * cada classe ocupava antes. Lido pelo relatorio de 3 s (nv_android_cache_diag). */
typedef struct { unsigned n; uint64_t bytes; } Classe;
static struct {
  unsigned chamadas, comPoda, varridosMax; double msTotal, msMax;
  Classe apagou[3];      /* 0 imagens, 1 colrow-*.bin, 2 outros */
  uint64_t ocupava[3];   /* bytes por classe antes da ultima poda com apagamento */
  uint64_t totalAntes, cap; unsigned versao, vista;
} diag;
static int classeDe(const char *p) {
  if (strstr(p,"/images/")) return 0;
  if (strstr(p,"/colrow-")) return 1;
  return 2;
}
static double agoraMs(void) {
  struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
  return t.tv_sec*1000.0+t.tv_nsec/1e6;
}
void nv_android_cache_trim(uint64_t cap) {
  Entry *list=NULL; size_t n=0; uint64_t bytes=0;
  double t0=agoraMs();
  pthread_mutex_lock(&lock);
  scan(nv_android_cache_dir(),&list,&n,&bytes,0,0);
  if (n>1) qsort(list,n,sizeof *list,oldest);
  uint64_t antes=bytes, oc[3]={0,0,0}; int apagou=0;
  if (bytes>cap) for (size_t i=0;i<n;i++) oc[classeDe(list[i].path)]+=list[i].bytes;
  for (size_t i=0;i<n && bytes>cap;i++)
    if (!unlink(list[i].path)) {
      int c=classeDe(list[i].path);
      bytes-=list[i].bytes; apagou=1;
      diag.apagou[c].n++; diag.apagou[c].bytes+=list[i].bytes;
    }
  double ms=agoraMs()-t0;
  diag.chamadas++; diag.msTotal+=ms; if (ms>diag.msMax) diag.msMax=ms;
  if (n>diag.varridosMax) diag.varridosMax=(unsigned)n;
  if (apagou) {
    diag.comPoda++; diag.totalAntes=antes; diag.cap=cap;
    for (int c=0;c<3;c++) diag.ocupava[c]=oc[c];
    diag.versao++;
  }
  free(list); pthread_mutex_unlock(&lock);
}
/* Uma linha [diag] se houve poda com apagamento desde a ultima leitura; 0 se nada novo. */
int nv_android_cache_diag(char *buf,size_t tam) {
  int novo;
  pthread_mutex_lock(&lock);
  novo=diag.versao!=diag.vista;
  if (novo) {
    diag.vista=diag.versao;
    snprintf(buf,tam,"[diag] androidcache: %u podas em %u chamadas, %.1f ms total (max %.1f), "
      "ate %u arquivos varridos; apagado img=%u/%.1fMB col=%u/%.1fMB outros=%u/%.1fMB; "
      "antes da ultima poda %.1fMB de teto %.1fMB (img %.1f col %.1f outros %.1f)",
      diag.comPoda,diag.chamadas,diag.msTotal,diag.msMax,diag.varridosMax,
      diag.apagou[0].n,diag.apagou[0].bytes/1048576.0,
      diag.apagou[1].n,diag.apagou[1].bytes/1048576.0,
      diag.apagou[2].n,diag.apagou[2].bytes/1048576.0,
      diag.totalAntes/1048576.0,diag.cap/1048576.0,
      diag.ocupava[0]/1048576.0,diag.ocupava[1]/1048576.0,diag.ocupava[2]/1048576.0);
  }
  pthread_mutex_unlock(&lock);
  return novo;
}
int nv_android_cache_clear(void) {
  size_t n=0; uint64_t b=0;
  pthread_mutex_lock(&lock);
  int ok=scan(nv_android_cache_dir(),NULL,&n,&b,1,0);
  pthread_mutex_unlock(&lock); return ok;
}
void nv_android_cache_touch(const char *p) { utime(p,NULL); }
void nv_android_cache_migrate(const char *data) {
  /* Discard only known regenerated legacy files, including old subtrees. */
  DIR *d=opendir(data); struct dirent *e;
  if (!d) return;
  while ((e=readdir(d))) if (nv_android_cache_name(e->d_name)) {
    char p[1200]; struct stat st; size_t n=0; uint64_t b=0;
    snprintf(p,sizeof p,"%s/%s",data,e->d_name);
    if (lstat(p,&st)) continue;
    if (S_ISDIR(st.st_mode)) { scan(p,NULL,&n,&b,1,0); rmdir(p); }
    else unlink(p);
  }
  closedir(d);
}
#ifdef NV_ANDROID
/* Native cache writers include direct stdio (EPG/trailers) and atomic
 * rename (images/catalogues). Enforce the same budget at both boundaries. */
int __real_fclose(FILE *f);
int __real_rename(const char *a,const char *b);
static int cachePath(const char *p) {
  const char *root=nv_android_cache_dir(); size_t n=strlen(root);
  return n && !strncmp(p,root,n) && p[n]=='/';
}
int __wrap_fclose(FILE *f) {
  char fd[64],p[1200];
  snprintf(fd,sizeof fd,"/proc/self/fd/%d",fileno(f));
  ssize_t n=readlink(fd,p,sizeof p-1); if(n>=0) p[n]=0;
  int cached=n>=0 && cachePath(p);
  int writing=(fcntl(fileno(f),F_GETFL)&O_ACCMODE)!=O_RDONLY;
  int rc=__real_fclose(f);
  if (cached && !rc) {
    nv_android_cache_touch(p);
    if (writing) nv_android_cache_trim(NV_ANDROID_CACHE_BYTES);
  }
  return rc;
}
int __wrap_rename(const char *a,const char *b) {
  int rc=__real_rename(a,b);
  if (!rc && cachePath(b)) nv_android_cache_trim(NV_ANDROID_CACHE_BYTES);
  return rc;
}
#endif
#else
const char *nv_android_cache_dir(void) { return ""; }
int nv_android_cache_name(const char *n) { (void)n; return 0; }
uint64_t nv_android_cache_size(void) { return 0; }
uint64_t nv_android_cache_size_all(void) { return 0; }
void nv_android_cache_trim(uint64_t cap) { (void)cap; }
int nv_android_cache_clear(void) { return 0; }
void nv_android_cache_touch(const char *p) { (void)p; }
void nv_android_cache_migrate(const char *p) { (void)p; }
#endif
