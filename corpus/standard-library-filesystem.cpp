#include <cstdio>

int karc_fs_status(const char* path, int follow, long* size, long* modified);
void* karc_fs_open_directory(const char* path);
int karc_fs_close_directory(void* directory);
int karc_fs_next_entry(void* directory, char* name, int room);
int karc_fs_make_directory(const char* path);
int karc_fs_remove_directory(const char* path);
int karc_fs_remove_file(const char* path);
int karc_fs_rename(const char* from, const char* to);

int main() {
    if (karc_fs_make_directory("karc-fs-probe") != 0) { return 1; }
    FILE* file = fopen("karc-fs-probe/a", "wb");
    if (!file) { return 2; }
    if (fwrite("hello\n", 1, 6, file) != 6) { return 3; }
    fclose(file);
    long size = 0;
    long modified = 0;
    if (karc_fs_status("karc-fs-probe/a", 1, &size, &modified) != 1 || size != 6 || modified <= 0) { return 4; }
    void* directory = karc_fs_open_directory("karc-fs-probe");
    if (!directory) { return 5; }
    char name[64];
    if (!karc_fs_next_entry(directory, name, 64) || name[0] != 'a') { return 6; }
    if (karc_fs_close_directory(directory) != 0) { return 7; }
    if (karc_fs_rename("karc-fs-probe/a", "karc-fs-probe/b") != 0) { return 8; }
    if (karc_fs_remove_file("karc-fs-probe/b") != 0) { return 9; }
    if (karc_fs_remove_directory("karc-fs-probe") != 0) { return 10; }
    return 42;
}
