// <csetjmp>: setjmp and longjmp over the C library's. A jmp_buf is a class
// holding the C buffer, sized for every supported target; the macros pass
// its words. longjmp skips the destructors of the frames it leaves, as in
// C++.
#ifndef KARC_STD_CSETJMP
#define KARC_STD_CSETJMP

struct jmp_buf {
    long words[64];
};

extern "C" {
int _setjmp(long* env);
void longjmp(long* env, int value);
}
#define setjmp(env) _setjmp((env).words)
#define longjmp(env, value) longjmp((env).words, value)

#endif
