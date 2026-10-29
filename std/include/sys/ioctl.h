#ifndef KARC_SYS_IOCTL_H
#define KARC_SYS_IOCTL_H
#include <sys/types.h>
#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int ioctl(int, unsigned long, ...);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
