int fibonacci(int n) {
    int a = 0;
    int b = 1;
    int i = 0;
    while (i < n) {
        int next = a + b;
        a = b;
        b = next;
        i = i + 1;
    }
    return a;
}

int main() {
    return fibonacci(10);
}
