// Win32 implementation of KArc's filesystem boundary. Windows structures are
// read as bytes so their layout does not depend on the frontend's C++ ABI.
extern "C" {
int GetFileAttributesExA(const char* path, int level, void* data);
void* CreateFileA(const char* path, unsigned int access, unsigned int sharing,
                  void* security, unsigned int creation, unsigned int flags, void* templateFile);
int GetFileInformationByHandle(void* file, void* information);
int CloseHandle(void* handle);
void* FindFirstFileA(const char* pattern, void* data);
int FindNextFileA(void* search, void* data);
int FindClose(void* search);
int CreateDirectoryA(const char* path, void* security);
int RemoveDirectoryA(const char* path);
int DeleteFileA(const char* path);
int MoveFileExA(const char* from, const char* to, unsigned int flags);
unsigned int GetCurrentDirectoryA(unsigned int room, char* buffer);
int SetCurrentDirectoryA(const char* path);
unsigned int GetFinalPathNameByHandleA(void* file, char* buffer, unsigned int room, unsigned int flags);
}

unsigned int karc_fs_word(const char* data, int offset) {
    unsigned int result = 0;
    memcpy(&result, data + offset, 4);
    return result;
}

long karc_fs_wide(const char* data, int lowOffset, int highOffset) {
    unsigned long low = karc_fs_word(data, lowOffset);
    unsigned long high = karc_fs_word(data, highOffset);
    return (long)((high << 32) | low);
}

void* karc_fs_handle(const char* path) {
    return CreateFileA(path, 0, 7, 0, 3, 33554432, 0);
}

int karc_fs_status(const char* path, int follow, long* size, long* modified) {
    char data[64];
    int sizeOffset = 28;
    if (follow) {
        void* file = karc_fs_handle(path);
        if (file == (void*)-1) { return 0; }
        int ok = GetFileInformationByHandle(file, data);
        CloseHandle(file);
        if (!ok) { return 0; }
        sizeOffset = 32;
    } else if (!GetFileAttributesExA(path, 0, data)) {
        return 0;
    }
    unsigned int attributes = karc_fs_word(data, 0);
    *size = karc_fs_wide(data, sizeOffset + 4, sizeOffset);
    long windowsTime = karc_fs_wide(data, 20, 24);
    *modified = windowsTime / 10000000 - 11644473600;
    if (!follow && (attributes & 1024) != 0) { return 3; }
    if ((attributes & 16) != 0) { return 2; }
    return 1;
}

struct Directory {
    void* search;
    int first;
    char data[320];
};

void* karc_fs_open_directory(const char* path) {
    int length = strlen(path);
    char* pattern = (char*)malloc(length + 3);
    if (pattern == 0) { return 0; }
    memcpy(pattern, path, length);
    if (length > 0 && path[length - 1] != '/' && path[length - 1] != '\\') {
        pattern[length++] = '\\';
    }
    pattern[length++] = '*';
    pattern[length] = 0;
    struct Directory* directory = (struct Directory*)malloc(sizeof(struct Directory));
    if (directory == 0) { free(pattern); return 0; }
    directory->search = FindFirstFileA(pattern, directory->data);
    free(pattern);
    if (directory->search == (void*)-1) { free(directory); return 0; }
    directory->first = 1;
    return directory;
}

int karc_fs_close_directory(void* opened) {
    if (opened == 0) { return -1; }
    struct Directory* directory = (struct Directory*)opened;
    int ok = FindClose(directory->search);
    free(directory);
    return ok ? 0 : -1;
}

int karc_fs_next_entry(void* opened, char* name, int room) {
    if (opened == 0 || name == 0 || room <= 0) { return 0; }
    struct Directory* directory = (struct Directory*)opened;
    while (1) {
        if (directory->first) { directory->first = 0; }
        else if (!FindNextFileA(directory->search, directory->data)) { return 0; }
        const char* text = directory->data + 44;
        if (strcmp(text, ".") == 0 || strcmp(text, "..") == 0) { continue; }
        strncpy(name, text, room - 1);
        name[room - 1] = 0;
        return 1;
    }
}

int karc_fs_make_directory(const char* path) { return CreateDirectoryA(path, 0) ? 0 : -1; }
int karc_fs_remove_directory(const char* path) { return RemoveDirectoryA(path) ? 0 : -1; }
int karc_fs_remove_file(const char* path) { return DeleteFileA(path) ? 0 : -1; }
int karc_fs_rename(const char* from, const char* to) { return MoveFileExA(from, to, 1) ? 0 : -1; }
int karc_fs_current(char* buffer, int room) {
    unsigned int length = GetCurrentDirectoryA(room, buffer);
    return length > 0 && length < room;
}
int karc_fs_change_directory(const char* path) { return SetCurrentDirectoryA(path) ? 0 : -1; }
int karc_fs_resolve(const char* path, char* buffer) {
    void* file = karc_fs_handle(path);
    if (file == (void*)-1) { return 0; }
    unsigned int length = GetFinalPathNameByHandleA(file, buffer, 4096, 0);
    CloseHandle(file);
    if (length == 0 || length >= 4096) { return 0; }
    // DOS paths are returned with the extended-length prefix.
    if (length >= 4 && buffer[0] == '\\' && buffer[1] == '\\' && buffer[2] == '?' && buffer[3] == '\\') {
        if (length >= 8 && buffer[4] == 'U' && buffer[5] == 'N' && buffer[6] == 'C' && buffer[7] == '\\') {
            memmove(buffer + 2, buffer + 8, length - 7);
        } else {
            memmove(buffer, buffer + 4, length - 3);
        }
    }
    return 1;
}
