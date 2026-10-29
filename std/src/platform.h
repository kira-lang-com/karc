// The platform boundary of the std: the POSIX calls it is written over. They
// are C symbols, reached through `call.foreign` on every engine.
#ifndef KARC_STD_PLATFORM
#define KARC_STD_PLATFORM

extern "C" {
long write(int descriptor, const void* data, unsigned long size);
long read(int descriptor, void* data, unsigned long size);
void _exit(int status);
int open(const char* path, int flags, int mode);
int close(int descriptor);
long lseek(int descriptor, long offset, int whence);
int isatty(int descriptor);
int unlinkat(int directory, const char* path, int flags);
int renameat(int fromDirectory, const char* from, int toDirectory, const char* to);
}

// The current directory for the *at calls.
#define KARC_AT_FDCWD (-100)

// Linux open(2) flags.
#define KARC_O_RDONLY 0
#define KARC_O_WRONLY 1
#define KARC_O_RDWR 2
#define KARC_O_CREAT 64
#define KARC_O_TRUNC 512
#define KARC_O_APPEND 1024

#endif
