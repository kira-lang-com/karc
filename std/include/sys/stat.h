#ifndef KARC_SYS_STAT_H
#define KARC_SYS_STAT_H
#include <sys/types.h>
#include <time.h>

struct stat {
    dev_t st_dev;
    ino_t st_ino;
#if defined(__KARC_TARGET_AARCH64__)
    mode_t st_mode;
    nlink_t st_nlink;
#else
    nlink_t st_nlink;
    mode_t st_mode;
#endif
    uid_t st_uid;
    gid_t st_gid;
#if defined(__KARC_TARGET_X86_64__)
    int reserved0;
#endif
    dev_t st_rdev;
#if defined(__KARC_TARGET_AARCH64__)
    dev_t reserved1;
#endif
    off_t st_size;
    blksize_t st_blksize;
#if defined(__KARC_TARGET_AARCH64__)
    int reserved2;
#endif
    blkcnt_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
#if defined(__KARC_TARGET_AARCH64__)
    int reserved3[2];
#else
    long reserved3[3];
#endif
};
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#define S_IFMT 0170000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFLNK 0120000
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100

#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int stat(const char*, struct stat*);
int fstat(int, struct stat*);
int lstat(const char*, struct stat*);
int mkdir(const char*, mode_t);
int chmod(const char*, mode_t);
int fchmod(int, mode_t);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
