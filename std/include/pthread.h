#ifndef KARC_PTHREAD_H
#define KARC_PTHREAD_H

#if !defined(__KARC_TARGET_LINUX_GNU__)
#error The bundled pthread declarations require a Linux GNU target
#endif

#if defined(__KARC_TARGET_AARCH64__)
#define KARC_PTHREAD_MUTEX_BYTES 48
#define KARC_PTHREAD_MUTEXATTR_BYTES 8
#elif defined(__KARC_TARGET_X86_64__)
#define KARC_PTHREAD_MUTEX_BYTES 40
#define KARC_PTHREAD_MUTEXATTR_BYTES 4
#else
#error The bundled pthread declarations require a supported 64-bit target
#endif

typedef unsigned long pthread_t;
typedef union {
    char bytes[KARC_PTHREAD_MUTEX_BYTES];
    long alignment;
} pthread_mutex_t;
typedef union {
    char bytes[KARC_PTHREAD_MUTEXATTR_BYTES];
    int alignment;
} pthread_mutexattr_t;

#define PTHREAD_MUTEX_INITIALIZER {{0}}
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL

#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_trylock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);
int pthread_mutexattr_init(pthread_mutexattr_t*);
int pthread_mutexattr_destroy(pthread_mutexattr_t*);
int pthread_mutexattr_settype(pthread_mutexattr_t*, int);
pthread_t pthread_self(void);
int pthread_equal(pthread_t, pthread_t);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif

#endif
