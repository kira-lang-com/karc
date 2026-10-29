// <cstring>
#include <cstring>
#include <cstdlib>

size_t strlen(const char* s) {
    const char* end = s;
    while (*end) { end++; }
    return end - s;
}

size_t strnlen(const char* s, size_t limit) {
    size_t count = 0;
    while (count < limit && s[count]) { count++; }
    return count;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t count) {
    size_t i = 0;
    while (i < count) {
        unsigned char left = a[i];
        unsigned char right = b[i];
        if (left != right) { return left - right; }
        if (left == 0) { return 0; }
        i++;
    }
    return 0;
}

char* strcpy(char* destination, const char* source) {
    size_t i = 0;
    while (source[i]) { destination[i] = source[i]; i++; }
    destination[i] = 0;
    return destination;
}

char* strncpy(char* destination, const char* source, size_t count) {
    size_t i = 0;
    while (i < count && source[i]) { destination[i] = source[i]; i++; }
    while (i < count) { destination[i] = 0; i++; }
    return destination;
}

char* strcat(char* destination, const char* source) {
    strcpy(destination + strlen(destination), source);
    return destination;
}

char* strncat(char* destination, const char* source, size_t count) {
    char* end = destination + strlen(destination);
    size_t i = 0;
    while (i < count && source[i]) { end[i] = source[i]; i++; }
    end[i] = 0;
    return destination;
}

char* strchr(const char* s, int c) {
    char wanted = c;
    while (*s != wanted) {
        if (*s == 0) { return 0; }
        s++;
    }
    return (char*)s;
}

char* strrchr(const char* s, int c) {
    char wanted = c;
    const char* found = 0;
    while (1) {
        if (*s == wanted) { found = s; }
        if (*s == 0) { return (char*)found; }
        s++;
    }
    return 0;
}

char* strstr(const char* haystack, const char* needle) {
    size_t length = strlen(needle);
    if (length == 0) { return (char*)haystack; }
    while (*haystack) {
        if (strncmp(haystack, needle, length) == 0) { return (char*)haystack; }
        haystack++;
    }
    return 0;
}

size_t strspn(const char* s, const char* accept) {
    size_t count = 0;
    while (s[count] && strchr(accept, s[count])) { count++; }
    return count;
}

size_t strcspn(const char* s, const char* reject) {
    size_t count = 0;
    while (s[count] && !strchr(reject, s[count])) { count++; }
    return count;
}

char* strpbrk(const char* s, const char* accept) {
    while (*s) {
        if (strchr(accept, *s)) { return (char*)s; }
        s++;
    }
    return 0;
}

char* strdup(const char* s) {
    size_t size = strlen(s) + 1;
    char* copy = (char*)malloc(size);
    if (copy) { memcpy(copy, s, size); }
    return copy;
}

char* strndup(const char* s, size_t count) {
    size_t size = strnlen(s, count);
    char* copy = (char*)malloc(size + 1);
    if (copy) {
        memcpy(copy, s, size);
        copy[size] = 0;
    }
    return copy;
}

static char* tokenRest;

char* strtok(char* s, const char* delimiters) {
    if (s == 0) { s = tokenRest; }
    if (s == 0) { return 0; }
    s = s + strspn(s, delimiters);
    if (*s == 0) {
        tokenRest = 0;
        return 0;
    }
    char* end = s + strcspn(s, delimiters);
    if (*end == 0) {
        tokenRest = 0;
    } else {
        *end = 0;
        tokenRest = end + 1;
    }
    return s;
}

int strcoll(const char* a, const char* b) { return strcmp(a, b); }

size_t strxfrm(char* destination, const char* source, size_t count) {
    size_t length = strlen(source);
    if (count > 0) { strncpy(destination, source, count); }
    return length;
}

// The messages of the common Linux error numbers.
char* strerror(int number) {
    if (number == 0) { return (char*)"Success"; }
    if (number == 1) { return (char*)"Operation not permitted"; }
    if (number == 2) { return (char*)"No such file or directory"; }
    if (number == 4) { return (char*)"Interrupted system call"; }
    if (number == 5) { return (char*)"Input/output error"; }
    if (number == 9) { return (char*)"Bad file descriptor"; }
    if (number == 12) { return (char*)"Cannot allocate memory"; }
    if (number == 13) { return (char*)"Permission denied"; }
    if (number == 17) { return (char*)"File exists"; }
    if (number == 20) { return (char*)"Not a directory"; }
    if (number == 21) { return (char*)"Is a directory"; }
    if (number == 22) { return (char*)"Invalid argument"; }
    if (number == 28) { return (char*)"No space left on device"; }
    if (number == 33) { return (char*)"Numerical argument out of domain"; }
    if (number == 34) { return (char*)"Numerical result out of range"; }
    return (char*)"Unknown error";
}

void* memcpy(void* destination, const void* source, size_t count) {
    char* to = (char*)destination;
    const char* from = (const char*)source;
    size_t i = 0;
    while (i < count) { to[i] = from[i]; i++; }
    return destination;
}

void* memmove(void* destination, const void* source, size_t count) {
    char* to = (char*)destination;
    const char* from = (const char*)source;
    if (to < from) {
        size_t i = 0;
        while (i < count) { to[i] = from[i]; i++; }
    } else {
        size_t i = count;
        while (i > 0) { i--; to[i] = from[i]; }
    }
    return destination;
}

void* memset(void* destination, int value, size_t count) {
    char* to = (char*)destination;
    size_t i = 0;
    while (i < count) { to[i] = value; i++; }
    return destination;
}

int memcmp(const void* a, const void* b, size_t count) {
    const unsigned char* left = (const unsigned char*)a;
    const unsigned char* right = (const unsigned char*)b;
    size_t i = 0;
    while (i < count) {
        if (left[i] != right[i]) { return left[i] - right[i]; }
        i++;
    }
    return 0;
}

void* memchr(const void* s, int c, size_t count) {
    const unsigned char* bytes = (const unsigned char*)s;
    unsigned char wanted = c;
    size_t i = 0;
    while (i < count) {
        if (bytes[i] == wanted) { return (void*)(bytes + i); }
        i++;
    }
    return 0;
}
