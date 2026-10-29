#ifndef KARC_SYS_TYPES_H
#define KARC_SYS_TYPES_H

#if !defined(__KARC_TARGET_LINUX_GNU__)
#error The bundled POSIX types require a Linux GNU target
#endif
#if !defined(__KARC_TARGET_AARCH64__) && !defined(__KARC_TARGET_X86_64__)
#error The bundled POSIX types require a supported 64-bit target
#endif

typedef long ssize_t;
typedef long off_t;
typedef long off64_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned int mode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef int pid_t;
typedef long time_t;
typedef long suseconds_t;
typedef long blkcnt_t;
#if defined(__KARC_TARGET_AARCH64__)
typedef unsigned int nlink_t;
typedef int blksize_t;
#else
typedef unsigned long nlink_t;
typedef long blksize_t;
#endif

#endif
