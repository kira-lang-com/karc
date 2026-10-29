// A record that embeds a fixed buffer, filled and summed through the array field.
struct Buf { int len; int data[4]; };

int main() {
    struct Buf b;
    b.len = 3;
    b.data[0] = 10;
    b.data[1] = 20;
    b.data[2] = 30;
    int total = 0;
    int i = 0;
    for (i = 0; i < b.len; i++) { total = total + b.data[i]; }
    return total;
}
