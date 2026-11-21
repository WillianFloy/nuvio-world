// Taloes de GL para tests/gif_modo.c (sem gl_compat.h, que renomeia as funcoes).
// O gif.c do Android sobe quadros para a GPU; aqui nada e desenhado: taloes.
void gfx_vidro_sujar(void) {}
void glBindTexture(unsigned a, unsigned b) { (void)a; (void)b; }
void glGenTextures(int n, unsigned *t) { for (int i = 0; i < n; i++) t[i] = 1; }
void glTexImage2D(unsigned a, int b, int c, int d, int e, int f, unsigned g, unsigned h, const void *p) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)p; }
void glTexSubImage2D(unsigned a, int b, int c, int d, int e, int f, unsigned g, unsigned h, const void *p) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)p; }
void glTexParameteri(unsigned a, unsigned b, int c) { (void)a; (void)b; (void)c; }
int perftv_enabled;
int perftv_uploads;
