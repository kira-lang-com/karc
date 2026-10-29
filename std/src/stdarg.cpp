// <cstdarg>: each packed argument is one 64-bit word.
#include <cstdarg>

char* karc_va_next(char** list) {
    char* slot = *list;
    *list = slot + 8;
    return slot;
}
