// <cfenv>: the floating-point environment. KArc rounds to nearest and
// raises no flags, so these report that and change nothing.
#ifndef KARC_STD_CFENV
#define KARC_STD_CFENV

#define FE_TONEAREST 0
#define FE_DOWNWARD 4194304
#define FE_UPWARD 8388608
#define FE_TOWARDZERO 12582912
#define FE_INVALID 1
#define FE_DIVBYZERO 2
#define FE_OVERFLOW 4
#define FE_UNDERFLOW 8
#define FE_INEXACT 16
#define FE_ALL_EXCEPT 31

typedef unsigned int fexcept_t;
struct fenv_t { unsigned int unused; };

inline int fegetround() { return FE_TONEAREST; }
inline int fesetround(int mode) { return mode == FE_TONEAREST ? 0 : 1; }
inline int feclearexcept(int flags) { return 0; }
inline int fetestexcept(int flags) { return 0; }
inline int feraiseexcept(int flags) { return 0; }
inline int fegetenv(fenv_t* environment) { return 0; }
inline int fesetenv(const fenv_t* environment) { return 0; }

#endif
