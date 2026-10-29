// <cuchar>: UTF-16 and UTF-32 conversions to and from UTF-8.
#ifndef KARC_STD_CUCHAR
#define KARC_STD_CUCHAR

#include <cwchar>

inline size_t c32rtomb(char* out, char32_t c, mbstate_t* state) { return karc_utf8_encode((unsigned int)c, out); }
inline size_t mbrtoc32(char32_t* out, const char* in, size_t count, mbstate_t* state) {
    if (in == 0 || count == 0) { return 0; }
    size_t used = 0;
    unsigned int code = karc_utf8_decode(in, &used);
    if (out != 0) { *out = (char32_t)code; }
    return code == 0 ? 0 : used;
}
inline size_t c16rtomb(char* out, char16_t c, mbstate_t* state) { return karc_utf8_encode((unsigned int)c, out); }
inline size_t mbrtoc16(char16_t* out, const char* in, size_t count, mbstate_t* state) {
    if (in == 0 || count == 0) { return 0; }
    size_t used = 0;
    unsigned int code = karc_utf8_decode(in, &used);
    if (out != 0) { *out = (char16_t)code; }
    return code == 0 ? 0 : used;
}

#endif
