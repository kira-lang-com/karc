// <cstdlib>: conversions, sorting, pseudo-random numbers and exit.
#include <cstdlib>
#include <cstring>
#include <cctype>
#include "platform.h"

int abs(int value) { if (value < 0) { return -value; } return value; }
long labs(long value) { if (value < 0) { return -value; } return value; }

static int digitValue(int c) {
    if (isdigit(c)) { return c - '0'; }
    if (islower(c)) { return c - 'a' + 10; }
    if (isupper(c)) { return c - 'A' + 10; }
    return 99;
}

unsigned long strtoul(const char* s, char** end, int base) {
    const char* p = s;
    while (isspace(*p)) { p++; }
    int negative = 0;
    if (*p == '+') { p++; }
    else if (*p == '-') { negative = 1; p++; }
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X') && isxdigit(p[2])) {
        p = p + 2;
        base = 16;
    }
    if (base == 0 && p[0] == '0') { base = 8; }
    if (base == 0) { base = 10; }
    unsigned long value = 0;
    const char* first = p;
    while (digitValue(*p) < base) {
        value = value * base + digitValue(*p);
        p++;
    }
    if (end) {
        if (p == first) { *end = (char*)s; } else { *end = (char*)p; }
    }
    if (negative) { return 0 - value; }
    return value;
}

long strtol(const char* s, char** end, int base) {
    return (long)strtoul(s, end, base);
}

double strtod(const char* s, char** end) {
    const char* p = s;
    while (isspace(*p)) { p++; }
    double sign = 1.0;
    if (*p == '+') { p++; }
    else if (*p == '-') { sign = -1.0; p++; }
    double value = 0.0;
    const char* first = p;
    while (isdigit(*p)) {
        value = value * 10.0 + (*p - '0');
        p++;
    }
    if (*p == '.') {
        p++;
        double scale = 0.1;
        while (isdigit(*p)) {
            value = value + (*p - '0') * scale;
            scale = scale / 10.0;
            p++;
        }
    }
    if (p == first || (p == first + 1 && *first == '.')) {
        if (end) { *end = (char*)s; }
        return 0.0;
    }
    if (*p == 'e' || *p == 'E') {
        const char* mark = p;
        p++;
        int negative = 0;
        if (*p == '+') { p++; }
        else if (*p == '-') { negative = 1; p++; }
        if (isdigit(*p)) {
            int exponent = 0;
            while (isdigit(*p)) {
                exponent = exponent * 10 + (*p - '0');
                p++;
            }
            while (exponent > 0) {
                if (negative) { value = value / 10.0; } else { value = value * 10.0; }
                exponent--;
            }
        } else {
            p = mark;
        }
    }
    if (end) { *end = (char*)p; }
    return sign * value;
}

double atof(const char* s) { return strtod(s, 0); }

long atol(const char* s) { return strtol(s, 0, 10); }
int atoi(const char* s) { return (int)strtol(s, 0, 10); }

static void swapBytes(char* a, char* b, size_t size) {
    size_t i = 0;
    while (i < size) {
        char saved = a[i];
        a[i] = b[i];
        b[i] = saved;
        i++;
    }
}

// Insertion sort: stable, allocation-free and enough for the subset's inputs.
void qsort(void* base, size_t count, size_t size, karc_compare_fn compare) {
    char* bytes = (char*)base;
    size_t i = 1;
    while (i < count) {
        size_t j = i;
        while (j > 0 && compare(bytes + (j - 1) * size, bytes + j * size) > 0) {
            swapBytes(bytes + (j - 1) * size, bytes + j * size, size);
            j--;
        }
        i++;
    }
}

void* bsearch(const void* key, const void* base, size_t count, size_t size, karc_compare_fn compare) {
    const char* bytes = (const char*)base;
    size_t low = 0;
    size_t high = count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        const char* element = bytes + middle * size;
        int order = compare(key, element);
        if (order == 0) { return (void*)element; }
        if (order < 0) { high = middle; } else { low = middle + 1; }
    }
    return 0;
}

static unsigned long randomState = 1;

int rand() {
    randomState = randomState * 1103515245 + 12345;
    return (int)((randomState / 65536) % 2147483648u);
}

void srand(unsigned int seed) { randomState = seed; }

extern "C" void karc_output_flush();
void exit(int status) {
    karc_output_flush();
    _exit(status);
}
void abort() { _exit(134); }
