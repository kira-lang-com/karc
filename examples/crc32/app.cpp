// CRC-32 (IEEE 802.3) of a byte string, computed bitwise. Exercises unsigned
// 32-bit arithmetic, shifts, xor and masking.
unsigned int crc32(char* data, int len) {
    unsigned int crc = 0xFFFFFFFF;
    int i = 0;
    while (i < len) {
        unsigned int byte = (unsigned int)(unsigned char)data[i];
        crc = crc ^ byte;
        int j = 0;
        while (j < 8) {
            unsigned int mask = 0 - (crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
            j = j + 1;
        }
        i = i + 1;
    }
    return crc ^ 0xFFFFFFFF;
}
int main() {
    return crc32("123456789", 9);
}
