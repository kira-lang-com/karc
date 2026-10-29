#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstdarg>

struct Pair { char tag; long value; };

class Text {
public:
    Text(const char* initial) : length(0) {
        data = new char[32];
        append(initial);
    }
    ~Text() {
        printf("drop %s\n", data);
        delete[] data;
    }
    void append(const char* more) {
        strcpy(data + length, more);
        length = length + strlen(more);
    }
    int size() const { return length; }
private:
    char* data;
    int length;
};

int sum(int count, ...) {
    va_list list;
    va_start(list, count);
    int total = 0;
    for (int i = 0; i < count; i++) { total += va_arg(list, int); }
    va_end(list);
    return total;
}

int byValue(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}

int main() {
    int failures = 0;
    char buffer[32];
    strcpy(buffer, "klf");
    strcat(buffer, " std");
    if (strlen(buffer) != 7 || strcmp(buffer, "klf std") != 0) { failures++; }
    if (strstr(buffer, "std") - buffer != 4) { failures++; }
    char* copy = strdup(buffer);
    copy[0] = toupper(copy[0]);
    if (copy[0] != 'K' || buffer[0] != 'k') { failures++; }
    free(copy);
    int* values = (int*)malloc(4 * sizeof(int));
    values[0] = 9; values[1] = 2; values[2] = 7; values[3] = 4;
    qsort(values, 4, sizeof(int), byValue);
    values = (int*)realloc(values, 8 * sizeof(int));
    if (values[0] != 2 || values[3] != 9) { failures++; }
    free(values);
    if (strtol("-0x2a", 0, 16) != -42 || atoi("17") != 17) { failures++; }
    Pair* pairs = new Pair[3];
    pairs[2].value = 40;
    if (sizeof(Pair) != 16 || pairs[1].value != 0) { failures++; }
    int* one = new int(2);
    if (sum(3, pairs[2].value, *one, 0) != 42) { failures++; }
    delete one;
    delete[] pairs;
    for (int i = 0; i < 3; i++) {
        Text scoped("loop");
        if (i == 1) { break; }
    }
    Text* word = new Text("heap");
    word->append("!");
    if (word->size() != 5) { failures++; }
    delete word;
    char line[32];
    snprintf(line, sizeof(line), "[%-4s|%04d]", "ab", 7);
    puts(buffer);
    printf("%s %d %x\n", line, -12, 255);
    return failures;
}
