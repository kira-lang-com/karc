// The platform boundary of the std: the POSIX calls it is written over. They
// are C symbols, reached through `call.foreign` on every engine.
#ifndef KARC_STD_PLATFORM
#define KARC_STD_PLATFORM

#if defined(__KARC_TARGET_WINDOWS_MSVC__)
// The Microsoft C runtime exports the underscore spellings. Use them in
// bytecode as well as native binaries so both resolve the same functions.
#define write _write
#define read _read
#define open _open
#define close _close
#define isatty _isatty
#endif

extern "C" {
#if defined(__KARC_TARGET_WINDOWS_MSVC__)
int write(int descriptor, const void* data, unsigned int size);
int read(int descriptor, void* data, unsigned int size);
long _lseeki64(int descriptor, long offset, int whence);
int _setmode(int descriptor, int mode);
int DeleteFileA(const char* path);
int RemoveDirectoryA(const char* path);
int MoveFileExA(const char* from, const char* to, unsigned int flags);
#else
long write(int descriptor, const void* data, unsigned long size);
long read(int descriptor, void* data, unsigned long size);
long lseek(int descriptor, long offset, int whence);
int unlinkat(int directory, const char* path, int flags);
int renameat(int fromDirectory, const char* from, int toDirectory, const char* to);
#endif
void _exit(int status);
int open(const char* path, int flags, int mode);
int close(int descriptor);
int isatty(int descriptor);
}

// The current directory for the *at calls.
#define KARC_AT_FDCWD (-100)

#define KARC_O_RDONLY 0
#define KARC_O_WRONLY 1
#define KARC_O_RDWR 2
#if defined(__KARC_TARGET_WINDOWS_MSVC__)
#define lseek _lseeki64
#define KARC_O_CREAT 256
#define KARC_O_TRUNC 512
#define KARC_O_APPEND 8
#define KARC_O_BINARY 32768
#else
#define KARC_O_CREAT 64
#define KARC_O_TRUNC 512
#define KARC_O_APPEND 1024
#define KARC_O_BINARY 0
#endif

#endif
