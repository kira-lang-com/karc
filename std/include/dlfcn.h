#ifndef KARC_DLFCN_H
#define KARC_DLFCN_H
#include <sys/types.h>
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 0x100
#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
void* dlopen(const char*, int);
void* dlsym(void*, const char*);
int dlclose(void*);
char* dlerror(void);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
