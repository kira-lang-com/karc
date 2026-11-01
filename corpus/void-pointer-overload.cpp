int select(const char* value) { return 1; }
int select(const void* value) { return 2; }

int main() {
    char* text = 0;
    void* opaque = text;
    return select(text) * 10 + select(opaque);
}
