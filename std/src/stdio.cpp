// <cstdio>: output on descriptor 1, errors on 2, input on 0.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include "platform.h"

// Standard output is buffered: fully into a file or pipe, by line on a
// terminal. The buffer empties when full, before standard input is read,
// on fflush, endl and exit, and when main returns (the lowering calls
// karc_output_flush there), so printf and cout share one ordering.
static char output[65536];
static long outputUsed;
static int outputMode = -1;

extern "C" void karc_output_flush() {
    long done = 0;
    while (done < outputUsed) {
        long put = write(1, output + done, outputUsed - done);
        if (put <= 0) { break; }
        done = done + put;
    }
    outputUsed = 0;
}

// Writes to `descriptor`, through the buffer for standard output.
extern "C" long karc_write(int descriptor, const char* data, long count) {
    if (descriptor != 1) { return write(descriptor, data, count); }
    if (outputMode < 0) { outputMode = isatty(1) ? 1 : 0; }
    if (outputUsed + count > (long)sizeof(output)) { karc_output_flush(); }
    if (count > (long)sizeof(output)) { return write(1, data, count); }
    memcpy(output + outputUsed, data, count);
    outputUsed = outputUsed + count;
    if (outputMode == 1 && memchr(data, '\n', count) != 0) { karc_output_flush(); }
    return count;
}

// Standard input is about to be read: what was written shows first.
extern "C" void karc_before_input(int descriptor) {
    if (descriptor == 0) { karc_output_flush(); }
}

int putchar(int c) {
    char byte[1];
    byte[0] = c;
    if (karc_write(1, byte, 1) != 1) { return EOF; }
    return (unsigned char)byte[0];
}

int print_string(const char* s) {
    size_t length = strlen(s);
    karc_write(1, s, length);
    return (int)length;
}

int eprint_string(const char* s) {
    size_t length = strlen(s);
    write(2, s, length);
    return (int)length;
}

int puts(const char* s) {
    int written = print_string(s);
    putchar('\n');
    return written + 1;
}

int getchar() {
    unsigned char byte[1];
    karc_before_input(0);
    if (read(0, byte, 1) != 1) { return EOF; }
    return byte[0];
}

static struct FILE streams[3];

FILE* karc_stream(int descriptor) {
    if (streams[descriptor].descriptor != descriptor) {
        streams[descriptor].descriptor = descriptor;
        streams[descriptor].pushed = -1;
    }
    return &streams[descriptor];
}

int fputc(int c, FILE* stream) {
    char byte[1];
    byte[0] = c;
    if (karc_write(stream->descriptor, byte, 1) != 1) {
        stream->error = 1;
        return EOF;
    }
    return (unsigned char)byte[0];
}

int putc(int c, FILE* stream) { return fputc(c, stream); }

int fputs(const char* s, FILE* stream) {
    size_t length = strlen(s);
    if (karc_write(stream->descriptor, s, length) != (long)length) {
        stream->error = 1;
        return EOF;
    }
    return (int)length;
}

int fgetc(FILE* stream) {
    if (stream->pushed >= 0) {
        int c = stream->pushed;
        stream->pushed = -1;
        return c;
    }
    unsigned char byte[1];
    karc_before_input(stream->descriptor);
    long got = read(stream->descriptor, byte, 1);
    if (got != 1) {
        if (got == 0) { stream->eof = 1; } else { stream->error = 1; }
        return EOF;
    }
    return byte[0];
}

int getc(FILE* stream) { return fgetc(stream); }

int ungetc(int c, FILE* stream) {
    if (c == EOF || stream->pushed >= 0) { return EOF; }
    stream->pushed = (unsigned char)c;
    stream->eof = 0;
    return (unsigned char)c;
}

char* fgets(char* buffer, int size, FILE* stream) {
    int count = 0;
    while (count + 1 < size) {
        int c = fgetc(stream);
        if (c == EOF) { break; }
        buffer[count] = c;
        count++;
        if (c == '\n') { break; }
    }
    if (count == 0) { return 0; }
    buffer[count] = 0;
    return buffer;
}

int fflush(FILE* stream) {
    if (stream == 0 || stream->descriptor == 1) { karc_output_flush(); }
    return 0;
}

// fopen's mode: "r", "w", "a", each with an optional "+" and "b".
static int openFlags(const char* mode) {
    int plus = 0;
    for (const char* m = mode; *m; m++) {
        if (*m == '+') { plus = 1; }
    }
    if (mode[0] == 'r') { return plus ? KARC_O_RDWR : KARC_O_RDONLY; }
    int access = plus ? KARC_O_RDWR : KARC_O_WRONLY;
    if (mode[0] == 'a') { return access | KARC_O_CREAT | KARC_O_APPEND; }
    return access | KARC_O_CREAT | KARC_O_TRUNC;
}

FILE* fopen(const char* path, const char* mode) {
    int descriptor = open(path, openFlags(mode), 420);
    if (descriptor < 0) { return 0; }
    FILE* stream = (FILE*)malloc(sizeof(FILE));
    if (stream == 0) {
        close(descriptor);
        return 0;
    }
    stream->descriptor = descriptor;
    stream->eof = 0;
    stream->error = 0;
    stream->pushed = -1;
    return stream;
}

FILE* freopen(const char* path, const char* mode, FILE* stream) {
    if (stream->descriptor > 2) { close(stream->descriptor); }
    int descriptor = open(path, openFlags(mode), 420);
    if (descriptor < 0) { return 0; }
    stream->descriptor = descriptor;
    stream->eof = 0;
    stream->error = 0;
    stream->pushed = -1;
    return stream;
}

int fclose(FILE* stream) {
    if (stream == 0) { return EOF; }
    int result = 0;
    if (stream->descriptor > 2) {
        result = close(stream->descriptor);
        free(stream);
    }
    return result == 0 ? 0 : EOF;
}

size_t fread(void* buffer, size_t size, size_t count, FILE* stream) {
    if (size != 0 && count > (size_t)-1 / size) {
        stream->error = 1;
        return 0;
    }
    size_t wanted = size * count;
    if (wanted == 0) { return 0; }
    char* out = (char*)buffer;
    size_t done = 0;
    if (stream->pushed >= 0) {
        out[0] = stream->pushed;
        stream->pushed = -1;
        done = 1;
    }
    while (done < wanted) {
        karc_before_input(stream->descriptor);
        long got = read(stream->descriptor, out + done, wanted - done);
        if (got <= 0) {
            if (got == 0) { stream->eof = 1; } else { stream->error = 1; }
            break;
        }
        done = done + got;
    }
    return done / size;
}

size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream) {
    if (size != 0 && count > (size_t)-1 / size) {
        stream->error = 1;
        return 0;
    }
    size_t wanted = size * count;
    if (wanted == 0) { return 0; }
    const char* in = (const char*)buffer;
    size_t done = 0;
    while (done < wanted) {
        long put = karc_write(stream->descriptor, in + done, wanted - done);
        if (put <= 0) {
            stream->error = 1;
            break;
        }
        done = done + put;
    }
    return done / size;
}

int fseek(FILE* stream, long offset, int whence) {
    stream->pushed = -1;
    stream->eof = 0;
    return lseek(stream->descriptor, offset, whence) < 0 ? -1 : 0;
}

long ftell(FILE* stream) {
    long at = lseek(stream->descriptor, 0, SEEK_CUR);
    if (at >= 0 && stream->pushed >= 0) { at--; }
    return at;
}

void rewind(FILE* stream) {
    fseek(stream, 0, SEEK_SET);
    stream->error = 0;
}
int fgetpos(FILE* stream, fpos_t* position) {
    *position = ftell(stream);
    return *position < 0 ? -1 : 0;
}
int fsetpos(FILE* stream, const fpos_t* position) { return fseek(stream, *position, SEEK_SET); }
int feof(FILE* stream) { return stream->eof; }
int ferror(FILE* stream) { return stream->error; }
void clearerr(FILE* stream) {
    stream->eof = 0;
    stream->error = 0;
}
int fileno(FILE* stream) { return stream->descriptor; }
int setvbuf(FILE* stream, char* buffer, int mode, size_t size) { return 0; }
void setbuf(FILE* stream, char* buffer) {}
int remove(const char* path) { return unlinkat(KARC_AT_FDCWD, path, 0) == 0 ? 0 : -1; }
int rename(const char* from, const char* to) { return renameat(KARC_AT_FDCWD, from, KARC_AT_FDCWD, to) == 0 ? 0 : -1; }

void perror(const char* message) {
    if (message != 0 && message[0] != 0) {
        write(2, message, strlen(message));
        write(2, ": ", 2);
    }
    const char* text = strerror(errno);
    write(2, text, strlen(text));
    write(2, "\n", 1);
}

static int printDigits(unsigned long value, unsigned long base) {
    char digits[24];
    int count = 0;
    while (1) {
        unsigned long digit = value % base;
        if (digit < 10) { digits[count] = '0' + digit; } else { digits[count] = 'a' + digit - 10; }
        count++;
        value = value / base;
        if (value == 0) { break; }
    }
    int i = count;
    while (i > 0) {
        i--;
        putchar(digits[i]);
    }
    return count;
}

int print_unsigned(unsigned long value) { return printDigits(value, 10); }
int print_hex(unsigned long value) { return printDigits(value, 16); }

int print_int(long value) {
    if (value < 0) {
        putchar('-');
        return 1 + printDigits(0 - (unsigned long)value, 10);
    }
    return printDigits(value, 10);
}
