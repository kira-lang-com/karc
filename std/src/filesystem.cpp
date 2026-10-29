// The POSIX side of <filesystem>. The stat and dirent64 layouts are those
// of Linux on AArch64.
#include <cstring>
#include <cstdlib>
#include "platform.h"

extern "C" {
int fstatat(int directory, const char* path, void* buffer, int flags);
long getdents64(int descriptor, void* buffer, unsigned long size);
int mkdir(const char* path, int mode);
int rmdir(const char* path);
char* getcwd(char* buffer, unsigned long size);
int chdir(const char* path);
char* realpath(const char* path, char* resolved);
}

#define KARC_STAT_SIZE 256
#define KARC_STAT_MODE 16
#define KARC_STAT_SIZE_FIELD 48
#define KARC_STAT_MTIME 88
#define KARC_DIRENT_TYPE 18
#define KARC_DIRENT_NAME 19

// The type bits of st_mode: 0 missing, 1 regular, 2 directory, 3 symlink,
// 4 other; with the size and modification time in seconds.
int karc_fs_status(const char* path, int follow, long* size, long* modified) {
    char buffer[KARC_STAT_SIZE];
    int flags = follow ? 0 : 256;
    if (fstatat(KARC_AT_FDCWD, path, buffer, flags) != 0) { return 0; }
    unsigned int mode = 0;
    long fileSize = 0;
    long modifiedTime = 0;
    memcpy(&mode, buffer + KARC_STAT_MODE, sizeof(mode));
    memcpy(&fileSize, buffer + KARC_STAT_SIZE_FIELD, sizeof(fileSize));
    memcpy(&modifiedTime, buffer + KARC_STAT_MTIME, sizeof(modifiedTime));
    *size = fileSize;
    *modified = modifiedTime;
    unsigned int kind = mode & 61440;
    if (kind == 32768) { return 1; }
    if (kind == 16384) { return 2; }
    if (kind == 40960) { return 3; }
    return 4;
}

// A directory being read: its descriptor and a buffer of getdents64
// records, which the kernel writes into memory the program owns (a VM
// program cannot read libc's own dirent storage).
struct Directory {
    int descriptor;
    long filled;
    long at;
    char records[4096];
};

#define KARC_O_DIRECTORY 16384

void* karc_fs_open_directory(const char* path) {
    int descriptor = open(path, KARC_O_RDONLY | KARC_O_DIRECTORY, 0);
    if (descriptor < 0) { return 0; }
    struct Directory* directory = (struct Directory*)malloc(sizeof(struct Directory));
    if (directory == 0) {
        close(descriptor);
        return 0;
    }
    directory->descriptor = descriptor;
    directory->filled = 0;
    directory->at = 0;
    return directory;
}

int karc_fs_close_directory(void* opened) {
    struct Directory* directory = (struct Directory*)opened;
    int result = close(directory->descriptor);
    free(directory);
    return result;
}

// The next entry's name into `name` (at most `room` bytes), skipping . and
// ..; 0 at the end.
int karc_fs_next_entry(void* opened, char* name, int room) {
    if (opened == 0 || name == 0 || room <= 0) { return 0; }
    struct Directory* directory = (struct Directory*)opened;
    while (1) {
        if (directory->at >= directory->filled) {
            directory->filled = getdents64(directory->descriptor, directory->records, 4096);
            directory->at = 0;
            if (directory->filled <= 0) { return 0; }
        }
        char* record = directory->records + directory->at;
        unsigned short length = 0;
        memcpy(&length, record + 16, sizeof(length));
        if (length == 0 || directory->at + length > directory->filled) { return 0; }
        directory->at = directory->at + length;
        const char* text = record + KARC_DIRENT_NAME;
        if (strcmp(text, ".") == 0 || strcmp(text, "..") == 0) { continue; }
        strncpy(name, text, room - 1);
        name[room - 1] = 0;
        return 1;
    }
    return 0;
}

int karc_fs_make_directory(const char* path) { return mkdir(path, 493); }
int karc_fs_remove_directory(const char* path) { return rmdir(path); }
int karc_fs_remove_file(const char* path) { return unlinkat(KARC_AT_FDCWD, path, 0); }
int karc_fs_rename(const char* from, const char* to) { return renameat(KARC_AT_FDCWD, from, KARC_AT_FDCWD, to); }
int karc_fs_current(char* buffer, int room) { return getcwd(buffer, room) != 0; }
int karc_fs_change_directory(const char* path) { return chdir(path); }
int karc_fs_resolve(const char* path, char* buffer) { return realpath(path, buffer) != 0; }
