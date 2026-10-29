#ifndef KARC_FCNTL_H
#define KARC_FCNTL_H
#include <sys/types.h>
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_EXCL 0200
#define O_NOCTTY 0400
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_DSYNC 010000
#define O_SYNC 04010000
#define O_CLOEXEC 02000000
#if defined(__KARC_TARGET_AARCH64__)
#define O_DIRECTORY 040000
#define O_NOFOLLOW 0100000
#else
#define O_DIRECTORY 0200000
#define O_NOFOLLOW 0400000
#endif
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETLK 5
#define F_SETLK 6
#define F_SETLKW 7
#define FD_CLOEXEC 1
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2
struct flock {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};
#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int open(const char*, int, ...);
int fcntl(int, int, ...);
int posix_fallocate(int, off_t, off_t);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
