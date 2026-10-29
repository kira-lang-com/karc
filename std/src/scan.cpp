// The scanf family. A Source reads characters from a string or a stream,
// with one character of lookahead pushed back.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

struct Source {
    const char* text;
    FILE* stream;
    int consumed;
};

static int next(struct Source* source) {
    int c = EOF;
    if (source->stream != 0) {
        c = fgetc(source->stream);
    } else if (*source->text != 0) {
        c = (unsigned char)*source->text;
        source->text++;
    }
    if (c != EOF) { source->consumed++; }
    return c;
}

static void back(struct Source* source, int c) {
    if (c == EOF) { return; }
    source->consumed--;
    if (source->stream != 0) {
        ungetc(c, source->stream);
    } else {
        source->text--;
    }
}

static int skipSpace(struct Source* source) {
    int c = next(source);
    while (c != EOF && isspace(c)) { c = next(source); }
    back(source, c);
    return c;
}

// Reads up to `width` characters of a number into `out`: an optional sign,
// then digits of `base` (0 detects a 0x or 0 prefix); floating-point
// numbers also take a point and an exponent.
static int scanNumber(struct Source* source, char* out, int width, int base, int floating) {
    int count = 0;
    int c = next(source);
    if ((c == '-' || c == '+') && count < width) {
        out[count] = c;
        count++;
        c = next(source);
    }
    int seenExponent = 0;
    while (c != EOF && count < width && count < 62) {
        int ok = 0;
        if (floating) {
            ok = isdigit(c) || c == '.' || ((c == 'e' || c == 'E') && !seenExponent) ||
                ((c == '-' || c == '+') && count > 0 && (out[count - 1] == 'e' || out[count - 1] == 'E'));
            if (c == 'e' || c == 'E') { seenExponent = 1; }
        } else if (base == 16 || base == 0) {
            ok = isxdigit(c) || ((c == 'x' || c == 'X') && count > 0 && out[count - 1] == '0');
        } else if (base == 8) {
            ok = c >= '0' && c <= '7';
        } else {
            ok = isdigit(c);
        }
        if (!ok) { break; }
        out[count] = c;
        count++;
        c = next(source);
    }
    back(source, c);
    out[count] = 0;
    return count > 0 && !(count == 1 && (out[0] == '-' || out[0] == '+'));
}

static int scan(struct Source* source, const char* f, va_list arguments) {
    int assigned = 0;
    while (*f) {
        if (isspace((unsigned char)*f)) {
            skipSpace(source);
            f++;
            continue;
        }
        if (*f != '%') {
            int c = next(source);
            if (c != (unsigned char)*f) {
                back(source, c);
                return assigned;
            }
            f++;
            continue;
        }
        f++;
        if (*f == '%') {
            skipSpace(source);
            int c = next(source);
            if (c != '%') { return assigned; }
            f++;
            continue;
        }
        int suppress = 0;
        if (*f == '*') { suppress = 1; f++; }
        int width = 0;
        while (*f >= '0' && *f <= '9') { width = width * 10 + (*f - '0'); f++; }
        if (width == 0) { width = 1 << 30; }
        int size = 0;
        while (*f == 'l' || *f == 'h' || *f == 'z' || *f == 'j' || *f == 't' || *f == 'L') {
            if (*f == 'l' || *f == 'z' || *f == 'j' || *f == 't' || *f == 'L') { size = 1; }
            if (*f == 'h') { size = -1; }
            f++;
        }
        char conversion = *f;
        f++;
        if (conversion == 'n') {
            if (!suppress) { *va_arg(arguments, int*) = source->consumed; }
            continue;
        }
        if (conversion == 'c') {
            if (width == 1 << 30) { width = 1; }
            char* out = suppress ? 0 : va_arg(arguments, char*);
            for (int i = 0; i < width; i++) {
                int c = next(source);
                if (c == EOF) { return assigned == 0 ? EOF : assigned; }
                if (out) { out[i] = c; }
            }
            if (!suppress) { assigned++; }
            continue;
        }
        if (conversion == '[') {
            int negate = 0;
            if (*f == '^') { negate = 1; f++; }
            const char* setStart = f;
            if (*f == ']') { f++; }
            while (*f && *f != ']') { f++; }
            const char* setEnd = f;
            if (*f == ']') { f++; }
            char* out = suppress ? 0 : va_arg(arguments, char*);
            int count = 0;
            int c = next(source);
            while (c != EOF && count < width) {
                int member = 0;
                for (const char* s = setStart; s < setEnd; s++) {
                    if (s + 2 < setEnd && s[1] == '-') {
                        if (c >= (unsigned char)s[0] && c <= (unsigned char)s[2]) { member = 1; }
                        s += 2;
                    } else if ((unsigned char)*s == c) {
                        member = 1;
                    }
                }
                if (member == negate) { break; }
                if (out) { out[count] = c; }
                count++;
                c = next(source);
            }
            back(source, c);
            if (count == 0) { return assigned; }
            if (out) {
                out[count] = 0;
                assigned++;
            }
            continue;
        }
        if (skipSpace(source) == EOF) { return assigned == 0 ? EOF : assigned; }
        if (conversion == 's') {
            char* out = suppress ? 0 : va_arg(arguments, char*);
            int count = 0;
            int c = next(source);
            while (c != EOF && !isspace(c) && count < width) {
                if (out) { out[count] = c; }
                count++;
                c = next(source);
            }
            back(source, c);
            if (out) {
                out[count] = 0;
                assigned++;
            }
            continue;
        }
        char digits[64];
        if (conversion == 'f' || conversion == 'e' || conversion == 'g' || conversion == 'E' || conversion == 'G' || conversion == 'a') {
            if (!scanNumber(source, digits, width, 10, 1)) { return assigned; }
            if (!suppress) {
                double value = strtod(digits, 0);
                if (size == 1) { *va_arg(arguments, double*) = value; } else { *va_arg(arguments, float*) = value; }
                assigned++;
            }
            continue;
        }
        int base = 10;
        if (conversion == 'x' || conversion == 'X' || conversion == 'p') { base = 16; }
        if (conversion == 'o') { base = 8; }
        if (conversion == 'i') { base = 0; }
        if (!scanNumber(source, digits, width, base, 0)) { return assigned; }
        if (suppress) { continue; }
        long value = 0;
        if (conversion == 'u' || conversion == 'x' || conversion == 'X' || conversion == 'o' || conversion == 'p') {
            value = (long)strtoul(digits, 0, base);
        } else {
            value = strtol(digits, 0, base);
        }
        if (conversion == 'p' || size == 1) {
            *va_arg(arguments, long*) = value;
        } else if (size == -1) {
            *va_arg(arguments, short*) = value;
        } else {
            *va_arg(arguments, int*) = value;
        }
        assigned++;
    }
    return assigned;
}

int vsscanf(const char* text, const char* f, va_list arguments) {
    struct Source source;
    source.text = text;
    source.stream = 0;
    source.consumed = 0;
    if (*text == 0) { return EOF; }
    return scan(&source, f, arguments);
}

int vfscanf(FILE* stream, const char* f, va_list arguments) {
    struct Source source;
    source.text = "";
    source.stream = stream;
    source.consumed = 0;
    return scan(&source, f, arguments);
}

int vscanf(const char* f, va_list arguments) { return vfscanf(stdin, f, arguments); }

int sscanf(const char* text, const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int result = vsscanf(text, f, arguments);
    va_end(arguments);
    return result;
}

int fscanf(FILE* stream, const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int result = vfscanf(stream, f, arguments);
    va_end(arguments);
    return result;
}

int scanf(const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int result = vfscanf(stdin, f, arguments);
    va_end(arguments);
    return result;
}
