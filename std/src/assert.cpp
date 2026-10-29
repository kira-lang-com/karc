// <cassert>
#include <cassert>
#include <cstdio>
#include "platform.h"

void karc_assert(int holds, const char* condition, const char* file, int line) {
    if (holds) { return; }
    eprint_string(condition);
    eprint_string("\n");
    _exit(134);
}
