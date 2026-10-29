// The exception in flight. A throw hands over a heap copy of the thrown value
// and the names it can be caught as, `;T;Base;...;`; the lowering then leaves
// every function up to a handler, testing `karc_exception_pending` after each
// call (see definitions/lowering/lower_exception.klfd).
#include "platform.h"

extern "C" void karc_output_flush();

static int pending;
static char* thrown;
static const char* names;
// Set when an exception_ptr took the exception: its handler does not free it.
static int kept;

extern "C" void karc_terminate() {
    karc_output_flush();
    write(2, "terminate called after throwing an exception\n", 45);
    _exit(134);
}

extern "C" int karc_exception_pending() { return pending; }

// The lowering declares these with byte pointers, so they take byte pointers.
extern "C" void karc_exception_throw(char* object, const char* chain) {
    thrown = object;
    names = chain;
    pending = 1;
}

// `throw;` outside a handler terminates, as C++ requires.
extern "C" void karc_exception_rethrow() {
    if (thrown == 0) { karc_terminate(); }
    pending = 1;
}

// Whether the exception can be caught as `name`: `;name;` is in its chain.
extern "C" int karc_exception_matches(const char* name) {
    if (names == 0) { return 0; }
    for (const char* at = names; *at != 0; at++) {
        if (*at != ';') { continue; }
        const char* candidate = at + 1;
        const char* wanted = name;
        while (*wanted != 0 && *candidate == *wanted) {
            candidate++;
            wanted++;
        }
        if (*wanted == 0 && *candidate == ';') { return 1; }
    }
    return 0;
}

// A handler takes the exception: it is no longer pending.
extern "C" char* karc_exception_catch() {
    pending = 0;
    return thrown;
}

// The handler finished and freed the exception.
extern "C" void karc_exception_forget() {
    thrown = 0;
    names = 0;
    kept = 0;
}

// std::current_exception: the exception being handled, which its handler
// then leaves alone; and std::rethrow_exception's way back in.
extern "C" char* karc_exception_keep() {
    kept = thrown != 0;
    return thrown;
}
extern "C" int karc_exception_kept() { return kept; }
extern "C" const char* karc_exception_names() { return names; }
extern "C" void karc_exception_restore(char* object, const char* chain) {
    thrown = object;
    names = chain;
    kept = 1;
}

// An exception left `main`.
extern "C" void karc_exception_uncaught() {
    karc_output_flush();
    const char* prefix = "terminate called after throwing an instance of '";
    long length = 0;
    while (prefix[length] != 0) { length++; }
    write(2, prefix, length);
    if (names != 0) {
        long end = 1;
        while (names[end] != 0 && names[end] != ';') { end++; }
        write(2, names + 1, end - 1);
    }
    write(2, "'\n", 2);
    _exit(134);
}

// While destructors run on the way to a handler, the exception is set aside
// so the code they call does not take it for its own.
extern "C" void karc_exception_suspend() { pending = 0; }
extern "C" void karc_exception_resume() { pending = 1; }

// For dynamic_cast (definitions/lowering/lower_rtti.klfd): whether `chain`,
// `;T;Base;...;`, names `name`; and a null test on a byte pointer.
extern "C" int karc_type_chain_has(const char* chain, const char* name) {
    if (chain == 0) { return 0; }
    for (const char* at = chain; *at != 0; at++) {
        if (*at != ';') { continue; }
        const char* candidate = at + 1;
        const char* wanted = name;
        while (*wanted != 0 && *candidate == *wanted) {
            candidate++;
            wanted++;
        }
        if (*wanted == 0 && *candidate == ';') { return 1; }
    }
    return 0;
}

extern "C" int karc_is_null(const char* pointer) { return pointer == 0; }
