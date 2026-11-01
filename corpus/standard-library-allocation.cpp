#include <cstdlib>

int main() {
    char* data = (char*)malloc(16);
    if (!data) { return 1; }
    data[0] = 42;
    int value = data[0];
    free(data);
    return value;
}
