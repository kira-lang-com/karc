// A recursive-descent arithmetic evaluator: tokenizes and evaluates an integer
// expression with +, -, *, /, parentheses and precedence. Uses mutual
// recursion (via forward declarations), a global cursor and buffer.
char src[128];
int pos;

int parseExpr();
int parseTerm();
int parseFactor();

void skipSpace() { while (src[pos] == ' ') { pos = pos + 1; } }

int parseFactor() {
    skipSpace();
    if (src[pos] == '(') {
        pos = pos + 1;
        int v = parseExpr();
        skipSpace();
        pos = pos + 1;
        return v;
    }
    int v = 0;
    while (src[pos] >= '0' && src[pos] <= '9') {
        v = v * 10 + (src[pos] - '0');
        pos = pos + 1;
    }
    return v;
}

int parseTerm() {
    int v = parseFactor();
    skipSpace();
    while (src[pos] == '*' || src[pos] == '/') {
        char op = src[pos];
        pos = pos + 1;
        int r = parseFactor();
        if (op == '*') { v = v * r; } else { v = v / r; }
        skipSpace();
    }
    return v;
}

int parseExpr() {
    int v = parseTerm();
    skipSpace();
    while (src[pos] == '+' || src[pos] == '-') {
        char op = src[pos];
        pos = pos + 1;
        int r = parseTerm();
        if (op == '+') { v = v + r; } else { v = v - r; }
        skipSpace();
    }
    return v;
}

int eval(char* s) {
    int i = 0;
    while (s[i] != 0) { src[i] = s[i]; i = i + 1; }
    src[i] = 0;
    pos = 0;
    return parseExpr();
}

int main() {
    return eval("2 + 3 * (4 - 1) + 10 / 2");
}
