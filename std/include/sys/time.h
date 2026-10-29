#ifndef KARC_SYS_TIME_H
#define KARC_SYS_TIME_H
#include <sys/types.h>
struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#if defined(__cplusplus) || defined(__KARC__)
extern "C" {
#endif
int gettimeofday(struct timeval*, void*);
#if defined(__cplusplus) || defined(__KARC__)
}
#endif
#endif
