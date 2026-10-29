// <cwctype>: wide-character classification. ASCII code points are
// classified as <cctype> does; others count as letters.
#ifndef KARC_STD_CWCTYPE
#define KARC_STD_CWCTYPE

#include <cctype>

#ifndef WEOF
#define WEOF (-1)
typedef int wint_t;
#endif
typedef int wctype_t;

inline int iswalpha(wint_t c) { return c >= 128 || isalpha(c); }
inline int iswdigit(wint_t c) { return c < 128 && isdigit(c); }
inline int iswalnum(wint_t c) { return iswalpha(c) || iswdigit(c); }
inline int iswspace(wint_t c) { return c < 128 && isspace(c); }
inline int iswupper(wint_t c) { return c < 128 && isupper(c); }
inline int iswlower(wint_t c) { return c < 128 && islower(c); }
inline int iswpunct(wint_t c) { return c < 128 && ispunct(c); }
inline int iswxdigit(wint_t c) { return c < 128 && isxdigit(c); }
inline int iswcntrl(wint_t c) { return c < 128 && iscntrl(c); }
inline int iswprint(wint_t c) { return c >= 128 || isprint(c); }
inline int iswgraph(wint_t c) { return c >= 128 || isgraph(c); }
inline int iswblank(wint_t c) { return c == ' ' || c == '\t'; }
inline wint_t towupper(wint_t c) { return c < 128 ? toupper(c) : c; }
inline wint_t towlower(wint_t c) { return c < 128 ? tolower(c) : c; }

#endif
