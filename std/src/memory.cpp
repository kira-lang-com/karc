// The allocator of <cstdlib>. Blocks come from the KLF runtime's raw heap; a
// 16-byte header in front of each keeps its size for `realloc`.
#include <cstdlib>
#include <cstring>
#include <new>

static const size_t header = 16;

void* malloc(size_t size) {
    if (size + header < size) { return 0; }
    char* block = __builtin_klf_alloc(size + header, 16);
    if (!block) { return 0; }
    size_t* stored = (size_t*)block;
    *stored = size;
    return block + header;
}

void* calloc(size_t count, size_t size) {
    size_t total = count * size;
    if (size != 0 && total / size != count) { return 0; }
    void* block = malloc(total);
    if (block) { memset(block, 0, total); }
    return block;
}

void free(void* pointer) {
    if (!pointer) { return; }
    char* bytes = (char*)pointer;
    __builtin_klf_free(bytes - header);
}

void* realloc(void* pointer, size_t size) {
    if (!pointer) { return malloc(size); }
    if (size == 0) {
        free(pointer);
        return 0;
    }
    char* bytes = (char*)pointer;
    size_t* stored = (size_t*)(bytes - header);
    size_t old = *stored;
    void* moved = malloc(size);
    if (!moved) { return 0; }
    size_t keep = old;
    if (size < keep) { keep = size; }
    memcpy(moved, pointer, keep);
    free(pointer);
    return moved;
}

extern "C" char* karc_operator_new(long size) {
    char* block = (char*)malloc(size);
    if (block) { memset(block, 0, size); }
    return block;
}

extern "C" void karc_operator_delete(char* pointer) {
    free(pointer);
}
