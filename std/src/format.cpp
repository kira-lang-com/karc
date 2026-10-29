// The printf family. Output goes through a Sink: a caller buffer, a stream,
// or both not at all (a size query). Streams are buffered 128 bytes at a time.
#include <cstdio>
#include <cstring>
#include "platform.h"

extern "C" long karc_write(int descriptor, const char* data, long count);

struct Sink {
    char* buffer;
    size_t size;
    size_t length;
    int descriptor;
    char pending[128];
    int pendingCount;
};

static void flushSink(struct Sink* sink) {
    if (sink->pendingCount > 0) {
        karc_write(sink->descriptor, sink->pending, sink->pendingCount);
        sink->pendingCount = 0;
    }
}

static void emit(struct Sink* sink, int c) {
    if (sink->descriptor >= 0) {
        if (sink->pendingCount == 128) { flushSink(sink); }
        sink->pending[sink->pendingCount] = c;
        sink->pendingCount++;
    } else if (sink->length + 1 < sink->size) {
        sink->buffer[sink->length] = c;
    }
    sink->length++;
}

static void emitRepeated(struct Sink* sink, int c, int count) {
    while (count > 0) { emit(sink, c); count--; }
}

// One converted field: `digits` holds `count` characters, `sign` is 0 or a
// prefix character, padded to `width` on the left or, with `left`, the right.
static void emitField(struct Sink* sink, const char* digits, int count, int sign, int width, int left, int zero) {
    int total = count;
    if (sign) { total++; }
    if (!left && !zero) { emitRepeated(sink, ' ', width - total); }
    if (sign) { emit(sink, sign); }
    if (!left && zero) { emitRepeated(sink, '0', width - total); }
    int i = 0;
    while (i < count) { emit(sink, digits[i]); i++; }
    if (left) { emitRepeated(sink, ' ', width - total); }
}

static int toDigits(unsigned long value, unsigned long base, int upper, char* out) {
    char reversed[24];
    int count = 0;
    while (1) {
        unsigned long digit = value % base;
        if (digit < 10) { reversed[count] = '0' + digit; }
        else if (upper) { reversed[count] = 'A' + digit - 10; }
        else { reversed[count] = 'a' + digit - 10; }
        count++;
        value = value / base;
        if (value == 0) { break; }
    }
    int i = 0;
    while (i < count) { out[i] = reversed[count - 1 - i]; i++; }
    return count;
}

// Fixed-point digits of a non-negative finite value into `out`: the integer
// part, then `precision` fraction digits, rounded half up.
static int fixedDigits(double value, int precision, char* out) {
    double rounding = 0.5;
    for (int i = 0; i < precision; i++) { rounding = rounding / 10.0; }
    value = value + rounding;
    double whole = __builtin_trunc(value);
    double fraction = value - whole;
    char reversed[320];
    int count = 0;
    if (whole < 1.0) {
        reversed[0] = '0';
        count = 1;
    }
    while (whole >= 1.0 && count < 320) {
        double next = __builtin_trunc(whole / 10.0);
        int digit = (int)(whole - next * 10.0);
        reversed[count] = '0' + digit;
        count++;
        whole = next;
    }
    int length = 0;
    for (int i = count - 1; i >= 0; i--) {
        out[length] = reversed[i];
        length++;
    }
    if (precision > 0) {
        out[length] = '.';
        length++;
        for (int i = 0; i < precision; i++) {
            fraction = fraction * 10.0;
            int digit = (int)fraction;
            if (digit > 9) { digit = 9; }
            out[length] = '0' + digit;
            length++;
            fraction = fraction - digit;
        }
    }
    return length;
}

// The decimal exponent of a positive finite value: v = m * 10^e, 1 <= m < 10.
static int decimalExponent(double value) {
    int exponent = 0;
    while (value >= 10.0) { value = value / 10.0; exponent++; }
    while (value < 1.0) { value = value * 10.0; exponent--; }
    return exponent;
}

static int scientificDigits(double value, int precision, char* out) {
    int exponent = 0;
    if (value != 0.0) {
        exponent = decimalExponent(value);
        for (int i = 0; i < exponent; i++) { value = value / 10.0; }
        for (int i = exponent; i < 0; i++) { value = value * 10.0; }
        double rounding = 0.5;
        for (int i = 0; i < precision; i++) { rounding = rounding / 10.0; }
        if (value + rounding >= 10.0) {
            value = value / 10.0;
            exponent++;
        }
    }
    int length = fixedDigits(value, precision, out);
    out[length] = 'e';
    length++;
    if (exponent < 0) {
        out[length] = '-';
        exponent = 0 - exponent;
    } else {
        out[length] = '+';
    }
    length++;
    if (exponent < 10) {
        out[length] = '0';
        length++;
    }
    char digits[8];
    int count = 0;
    while (exponent > 0 || count == 0) {
        digits[count] = '0' + exponent % 10;
        exponent = exponent / 10;
        count++;
    }
    for (int i = count - 1; i >= 0; i--) {
        out[length] = digits[i];
        length++;
    }
    return length;
}

// Drops trailing fraction zeros (and a bare point) before any exponent.
static int stripZeros(char* out, int length) {
    int point = -1;
    int exponentAt = length;
    for (int i = 0; i < length; i++) {
        if (out[i] == '.') { point = i; }
        if (out[i] == 'e') { exponentAt = i; }
    }
    if (point < 0) { return length; }
    int end = exponentAt;
    while (end > point + 1 && out[end - 1] == '0') { end--; }
    if (end == point + 1) { end = point; }
    int written = end;
    for (int i = exponentAt; i < length; i++) {
        out[written] = out[i];
        written++;
    }
    return written;
}

static unsigned long doubleBits(double x) {
    unsigned long bits = 0;
    memcpy(&bits, &x, sizeof(bits));
    return bits;
}

static void emitDouble(struct Sink* sink, double value, char style, int precision, int width, int left, int zero, int plus) {
    char digits[360];
    int count = 0;
    int sign = 0;
    unsigned long bits = doubleBits(value);
    if ((bits & 9218868437227405312ul) == 9218868437227405312ul && (bits & 4503599627370495ul) != 0) {
        emitField(sink, "nan", 3, 0, width, left, 0);
        return;
    }
    if (value < 0.0) {
        sign = '-';
        value = 0.0 - value;
    } else if (plus) {
        sign = '+';
    }
    if ((bits & 9223372036854775807ul) == 9218868437227405312ul) {
        emitField(sink, "inf", 3, sign, width, left, 0);
        return;
    }
    if (precision < 0) { precision = 6; }
    if (style == 'f' || style == 'F') {
        count = fixedDigits(value, precision, digits);
    } else if (style == 'e' || style == 'E') {
        count = scientificDigits(value, precision, digits);
    } else {
        if (precision == 0) { precision = 1; }
        int exponent = 0;
        if (value != 0.0) { exponent = decimalExponent(value); }
        if (exponent < -4 || exponent >= precision) {
            count = stripZeros(digits, scientificDigits(value, precision - 1, digits));
        } else {
            count = stripZeros(digits, fixedDigits(value, precision - 1 - exponent, digits));
        }
    }
    emitField(sink, digits, count, sign, width, left, zero);
}

static int format(struct Sink* sink, const char* f, va_list arguments) {
    while (*f) {
        if (*f != '%') { emit(sink, *f); f++; continue; }
        f++;
        int left = 0;
        int zero = 0;
        int plus = 0;
        while (*f == '-' || *f == '0' || *f == '+' || *f == ' ' || *f == '#') {
            if (*f == '-') { left = 1; }
            if (*f == '0') { zero = 1; }
            if (*f == '+') { plus = 1; }
            f++;
        }
        int width = 0;
        if (*f == '*') { width = va_arg(arguments, int); f++; }
        while (*f >= '0' && *f <= '9') { width = width * 10 + (*f - '0'); f++; }
        int precision = -1;
        if (*f == '.') {
            f++;
            precision = 0;
            if (*f == '*') { precision = va_arg(arguments, int); f++; }
            while (*f >= '0' && *f <= '9') { precision = precision * 10 + (*f - '0'); f++; }
        }
        int wide = 0;
        while (*f == 'l' || *f == 'z' || *f == 'j' || *f == 't' || *f == 'h') {
            if (*f != 'h') { wide = 1; }
            f++;
        }
        if (left) { zero = 0; }
        char digits[24];
        char c = *f;
        if (c == 0) { break; }
        f++;
        if (c == 'd' || c == 'i') {
            long value = 0;
            if (wide) { value = va_arg(arguments, long); } else { value = va_arg(arguments, int); }
            int sign = 0;
            unsigned long magnitude = value;
            if (value < 0) { sign = '-'; magnitude = 0 - magnitude; }
            else if (plus) { sign = '+'; }
            emitField(sink, digits, toDigits(magnitude, 10, 0, digits), sign, width, left, zero);
        } else if (c == 'u' || c == 'x' || c == 'X' || c == 'o') {
            unsigned long value = 0;
            if (wide) { value = va_arg(arguments, unsigned long); } else { value = va_arg(arguments, unsigned int); }
            unsigned long base = 10;
            if (c == 'x' || c == 'X') { base = 16; }
            if (c == 'o') { base = 8; }
            emitField(sink, digits, toDigits(value, base, c == 'X', digits), 0, width, left, zero);
        } else if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G') {
            emitDouble(sink, va_arg(arguments, double), c, precision, width, left, zero, plus);
        } else if (c == 'p') {
            unsigned long value = va_arg(arguments, unsigned long);
            emitField(sink, digits, toDigits(value, 16, 0, digits), 0, width - 2, left, 0);
        } else if (c == 'c') {
            digits[0] = va_arg(arguments, int);
            emitField(sink, digits, 1, 0, width, left, 0);
        } else if (c == 's') {
            const char* text = va_arg(arguments, const char*);
            if (!text) { text = "(null)"; }
            int count = (int)strlen(text);
            if (precision >= 0 && precision < count) { count = precision; }
            emitField(sink, text, count, 0, width, left, 0);
        } else {
            emit(sink, c);
        }
    }
    return (int)sink->length;
}

static struct Sink streamSink(int descriptor) {
    struct Sink sink;
    sink.buffer = 0;
    sink.size = 0;
    sink.length = 0;
    sink.descriptor = descriptor;
    sink.pendingCount = 0;
    return sink;
}

int vfprintf(FILE* stream, const char* f, va_list arguments) {
    struct Sink sink = streamSink(stream->descriptor);
    int written = format(&sink, f, arguments);
    flushSink(&sink);
    return written;
}

int vprintf(const char* f, va_list arguments) {
    return vfprintf(stdout, f, arguments);
}

int vsnprintf(char* buffer, size_t size, const char* f, va_list arguments) {
    struct Sink sink = streamSink(-1);
    sink.buffer = buffer;
    sink.size = size;
    int written = format(&sink, f, arguments);
    if (size > 0) {
        if (sink.length < size) { buffer[sink.length] = 0; } else { buffer[size - 1] = 0; }
    }
    return written;
}

int vsprintf(char* buffer, const char* f, va_list arguments) {
    return vsnprintf(buffer, 4294967295u, f, arguments);
}

int printf(const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int written = vfprintf(stdout, f, arguments);
    va_end(arguments);
    return written;
}

int fprintf(FILE* stream, const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int written = vfprintf(stream, f, arguments);
    va_end(arguments);
    return written;
}

int snprintf(char* buffer, size_t size, const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int written = vsnprintf(buffer, size, f, arguments);
    va_end(arguments);
    return written;
}

int sprintf(char* buffer, const char* f, ...) {
    va_list arguments;
    va_start(arguments, f);
    int written = vsnprintf(buffer, 4294967295u, f, arguments);
    va_end(arguments);
    return written;
}
