// A tiny stack-based bytecode virtual machine. The program is a flat array of
// (opcode, operand) pairs; opcodes push, add, sub, mul, dup and halt.
enum Op { PUSH, ADD, SUB, MUL, DUP, HALT };

int run(int* prog, int steps) {
    int stack[64];
    int sp = 0;
    int pc = 0;
    while (pc < steps) {
        int op = prog[pc * 2];
        int arg = prog[pc * 2 + 1];
        switch (op) {
            case PUSH:
                stack[sp] = arg;
                sp = sp + 1;
                break;
            case ADD:
                sp = sp - 1;
                stack[sp - 1] = stack[sp - 1] + stack[sp];
                break;
            case SUB:
                sp = sp - 1;
                stack[sp - 1] = stack[sp - 1] - stack[sp];
                break;
            case MUL:
                sp = sp - 1;
                stack[sp - 1] = stack[sp - 1] * stack[sp];
                break;
            case DUP:
                stack[sp] = stack[sp - 1];
                sp = sp + 1;
                break;
            case HALT:
                pc = steps;
                break;
        }
        pc = pc + 1;
    }
    return stack[sp - 1];
}

int main() {
    int prog[32];
    prog[0] = PUSH; prog[1] = 2;
    prog[2] = PUSH; prog[3] = 3;
    prog[4] = ADD;  prog[5] = 0;
    prog[6] = PUSH; prog[7] = 4;
    prog[8] = MUL;  prog[9] = 0;
    prog[10] = PUSH; prog[11] = 5;
    prog[12] = SUB;  prog[13] = 0;
    prog[14] = HALT; prog[15] = 0;
    return run(prog, 8);
}
