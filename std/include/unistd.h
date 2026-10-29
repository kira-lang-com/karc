#ifndef KARC_UNISTD_H
#define KARC_UNISTD_H
#include <sys/types.h>
#include <stddef.h>
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define _SC_PAGESIZE 30
#define _SC_PAGE_SIZE _SC_PAGESIZE
#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int close(int);
int access(const char*, int);
char* getcwd(char*, size_t);
int ftruncate(int, off_t);
ssize_t read(int, void*, size_t);
ssize_t write(int, const void*, size_t);
ssize_t pread(int, void*, size_t, off_t);
ssize_t pwrite(int, const void*, size_t, off_t);
off_t lseek(int, off_t, int);
int unlink(const char*);
int rmdir(const char*);
int fsync(int);
int fdatasync(int);
pid_t getpid(void);
uid_t geteuid(void);
int fchown(int, uid_t, gid_t);
ssize_t readlink(const char*, char*, size_t);
unsigned int sleep(unsigned int);
int usleep(unsigned int);
long sysconf(int);
int getpagesize(void);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
