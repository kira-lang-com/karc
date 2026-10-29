// Classic libc string routines in idiomatic pointer style.
int strlen(char* s) { char* p = s; while (*p) { p = p + 1; } return p - s; }
void strcpy(char* d, char* s) { while (*s) { *d = *s; d = d + 1; s = s + 1; } *d = 0; }
int strcmp(char* a, char* b) { while (*a && *a == *b) { a = a + 1; b = b + 1; } return *a - *b; }
char* strchr(char* s, char c) { while (*s) { if (*s == c) { return s; } s = s + 1; } return 0; }

int main() {
    char buf[16];
    strcpy(buf, "hello");
    int L = strlen(buf);
    int eq = strcmp(buf, "hello");
    char* e = strchr(buf, 'l');
    int idx = e - buf;
    char* miss = strchr(buf, 'z');
    int notfound = miss == 0 ? 1 : 0;
    return L * 1000 + idx * 10 + notfound + eq;
}
