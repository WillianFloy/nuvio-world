#ifndef NV_ANDROID_CACHE_H
#define NV_ANDROID_CACHE_H
#include <stddef.h>
#include <stdint.h>
#define NV_ANDROID_CACHE_BYTES (80ULL << 20)
const char *nv_android_cache_dir(void);
int nv_android_cache_name(const char *name);
uint64_t nv_android_cache_size(void);
uint64_t nv_android_cache_size_all(void);
void nv_android_cache_trim(uint64_t cap);
int nv_android_cache_diag(char *buf, size_t tam);   /* 2.0.35: linha [diag] se houve poda nova */
int nv_android_cache_clear(void);
void nv_android_cache_touch(const char *path);
void nv_android_cache_migrate(const char *data);
#endif
