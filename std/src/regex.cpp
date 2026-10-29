// The regular expression engine behind <regex>: a recursive-descent
// compiler to a node tree, and a backtracking matcher that carries what is
// left to match as a chain of continuations on the stack.
#include <regex>
#include <cctype>

#define NODE_CHAR 1
#define NODE_ANY 2
#define NODE_CLASS 3
#define NODE_START 4
#define NODE_END 5
#define NODE_WORD_BOUNDARY 6
#define NODE_NOT_WORD_BOUNDARY 7
#define NODE_GROUP 8
#define NODE_ALTERNATION 9
#define NODE_REPEAT 10
#define NODE_BACKREFERENCE 11

struct Compiler {
    const char* pattern;
    size_t at;
    std::basic_regex_karc* program;
};

static int addNode(struct Compiler* c, int kind) {
    std::karc_regex_node node;
    node.kind = kind;
    node.value = 0;
    node.least = 1;
    node.most = 1;
    node.lazy = false;
    node.negate = false;
    c->program->nodes.push_back(node);
    return (int)c->program->nodes.size() - 1;
}

static void addRange(std::vector<int>& ranges, int low, int high) {
    ranges.push_back(low);
    ranges.push_back(high);
}

// \d \w \s and their negations as ranges; false for other letters.
static bool classEscape(char letter, std::vector<int>& ranges, bool& negate) {
    char lower = tolower(letter);
    if (lower != 'd' && lower != 'w' && lower != 's') { return false; }
    negate = letter != lower;
    if (lower == 'd') { addRange(ranges, '0', '9'); }
    if (lower == 'w') {
        addRange(ranges, 'a', 'z');
        addRange(ranges, 'A', 'Z');
        addRange(ranges, '0', '9');
        addRange(ranges, '_', '_');
    }
    if (lower == 's') {
        addRange(ranges, ' ', ' ');
        addRange(ranges, '\t', '\r');
    }
    return true;
}

static int escapedCharacter(char letter) {
    if (letter == 'n') { return '\n'; }
    if (letter == 't') { return '\t'; }
    if (letter == 'r') { return '\r'; }
    if (letter == 'f') { return '\f'; }
    if (letter == 'v') { return '\v'; }
    if (letter == '0') { return 0; }
    return letter;
}

static std::vector<int> parseAlternation(struct Compiler* c);

static int parseClass(struct Compiler* c) {
    int id = addNode(c, NODE_CLASS);
    std::vector<int> ranges;
    bool negate = false;
    if (c->pattern[c->at] == '^') {
        negate = true;
        c->at++;
    }
    bool first = true;
    while (c->pattern[c->at] != 0 && (c->pattern[c->at] != ']' || first)) {
        first = false;
        int low = (unsigned char)c->pattern[c->at];
        c->at++;
        if (low == '\\') {
            char letter = c->pattern[c->at];
            c->at++;
            std::vector<int> escaped;
            bool negated = false;
            if (classEscape(letter, escaped, negated)) {
                // A negated escape inside a class keeps the complement's
                // ASCII ranges.
                if (negated) {
                    int from = 0;
                    for (size_t i = 0; i < escaped.size(); i += 2) {
                        if (escaped[i] > from) { addRange(ranges, from, escaped[i] - 1); }
                        from = escaped[i + 1] + 1;
                    }
                    addRange(ranges, from, 255);
                } else {
                    for (size_t i = 0; i < escaped.size(); i++) { ranges.push_back(escaped[i]); }
                }
                continue;
            }
            low = escapedCharacter(letter);
        }
        int high = low;
        if (c->pattern[c->at] == '-' && c->pattern[c->at + 1] != ']' && c->pattern[c->at + 1] != 0) {
            c->at++;
            high = (unsigned char)c->pattern[c->at];
            c->at++;
            if (high == '\\') {
                high = escapedCharacter(c->pattern[c->at]);
                c->at++;
            }
        }
        addRange(ranges, low, high);
    }
    if (c->pattern[c->at] != ']') { throw std::regex_error("unterminated character class"); }
    c->at++;
    c->program->nodes[id].ranges = ranges;
    c->program->nodes[id].negate = negate;
    return id;
}

static int parseAtom(struct Compiler* c) {
    char next = c->pattern[c->at];
    c->at++;
    if (next == '.') { return addNode(c, NODE_ANY); }
    if (next == '^') { return addNode(c, NODE_START); }
    if (next == '$') { return addNode(c, NODE_END); }
    if (next == '[') { return parseClass(c); }
    if (next == '(') {
        int id = addNode(c, NODE_GROUP);
        int group = -1;
        if (c->pattern[c->at] == '?' && c->pattern[c->at + 1] == ':') {
            c->at += 2;
        } else {
            c->program->groups++;
            group = c->program->groups;
        }
        std::vector<int> body = parseAlternation(c);
        if (c->pattern[c->at] != ')') { throw std::regex_error("unmatched parenthesis"); }
        c->at++;
        c->program->nodes[id].value = group;
        c->program->nodes[id].sequence = body;
        return id;
    }
    if (next == '\\') {
        char letter = c->pattern[c->at];
        if (letter == 0) { throw std::regex_error("trailing backslash"); }
        c->at++;
        if (letter == 'b') { return addNode(c, NODE_WORD_BOUNDARY); }
        if (letter == 'B') { return addNode(c, NODE_NOT_WORD_BOUNDARY); }
        if (letter >= '1' && letter <= '9') {
            int id = addNode(c, NODE_BACKREFERENCE);
            c->program->nodes[id].value = letter - '0';
            return id;
        }
        std::vector<int> ranges;
        bool negate = false;
        if (classEscape(letter, ranges, negate)) {
            int id = addNode(c, NODE_CLASS);
            c->program->nodes[id].ranges = ranges;
            c->program->nodes[id].negate = negate;
            return id;
        }
        int id = addNode(c, NODE_CHAR);
        c->program->nodes[id].value = escapedCharacter(letter);
        return id;
    }
    if (next == '*' || next == '+' || next == '?' || next == ')') { throw std::regex_error("nothing to repeat"); }
    int id = addNode(c, NODE_CHAR);
    c->program->nodes[id].value = (unsigned char)next;
    return id;
}

static int parseNumber(struct Compiler* c) {
    int value = 0;
    while (c->pattern[c->at] >= '0' && c->pattern[c->at] <= '9') {
        value = value * 10 + (c->pattern[c->at] - '0');
        c->at++;
    }
    return value;
}

// An atom and its quantifier, if any.
static int parsePiece(struct Compiler* c) {
    int atom = parseAtom(c);
    int least = -1;
    int most = -1;
    char next = c->pattern[c->at];
    if (next == '*') { least = 0; most = -1; c->at++; }
    else if (next == '+') { least = 1; most = -1; c->at++; }
    else if (next == '?') { least = 0; most = 1; c->at++; }
    else if (next == '{' && c->pattern[c->at + 1] >= '0' && c->pattern[c->at + 1] <= '9') {
        c->at++;
        least = parseNumber(c);
        most = least;
        if (c->pattern[c->at] == ',') {
            c->at++;
            most = -1;
            if (c->pattern[c->at] != '}') { most = parseNumber(c); }
        }
        if (c->pattern[c->at] != '}') { throw std::regex_error("bad repetition"); }
        c->at++;
    }
    if (least < 0) { return atom; }
    int id = addNode(c, NODE_REPEAT);
    c->program->nodes[id].least = least;
    c->program->nodes[id].most = most;
    if (c->pattern[c->at] == '?') {
        c->program->nodes[id].lazy = true;
        c->at++;
    }
    std::vector<int> body;
    body.push_back(atom);
    c->program->nodes[id].sequence = body;
    return id;
}

static std::vector<int> parseSequence(struct Compiler* c) {
    std::vector<int> sequence;
    while (c->pattern[c->at] != 0 && c->pattern[c->at] != '|' && c->pattern[c->at] != ')') {
        sequence.push_back(parsePiece(c));
    }
    return sequence;
}

static std::vector<int> parseAlternation(struct Compiler* c) {
    std::vector<int> first = parseSequence(c);
    if (c->pattern[c->at] != '|') { return first; }
    int id = addNode(c, NODE_ALTERNATION);
    std::vector<std::vector<int>> alternatives;
    alternatives.push_back(first);
    while (c->pattern[c->at] == '|') {
        c->at++;
        alternatives.push_back(parseSequence(c));
    }
    c->program->nodes[id].alternatives = alternatives;
    std::vector<int> result;
    result.push_back(id);
    return result;
}

namespace std {

regex& regex::assign(const char* pattern, int flags) {
    program_.nodes.clear();
    program_.groups = 0;
    program_.icase = (flags & 1) != 0;
    struct Compiler c;
    c.pattern = pattern;
    c.at = 0;
    c.program = &program_;
    program_.top = parseAlternation(&c);
    if (pattern[c.at] != 0) { throw regex_error("unmatched parenthesis"); }
    return *this;
}

}

// What is left to match: the rest of a sequence, the end of a group, one
// more round of a repetition, or the end of the text.
struct Continuation {
    int kind;
    const std::vector<int>* sequence;
    size_t index;
    int node;
    int count;
    long start;
    Continuation* next;
};

#define REST_SEQUENCE 0
#define REST_GROUP_END 1
#define REST_REPEAT 2
#define REST_TEXT_END 3

struct Matcher {
    const std::basic_regex_karc* program;
    const std::string* text;
    std::vector<long>* bounds;
};

static bool run(struct Matcher* m, struct Continuation* rest, long at);

static bool sameCharacter(struct Matcher* m, int a, int b) {
    if (m->program->icase) { return tolower(a) == tolower(b); }
    return a == b;
}

static bool inClass(struct Matcher* m, const std::karc_regex_node& node, int c) {
    bool found = false;
    for (size_t i = 0; i + 1 < node.ranges.size(); i += 2) {
        if (c >= node.ranges[i] && c <= node.ranges[i + 1]) { found = true; }
        if (m->program->icase && tolower(c) >= node.ranges[i] && tolower(c) <= node.ranges[i + 1]) { found = true; }
        if (m->program->icase && toupper(c) >= node.ranges[i] && toupper(c) <= node.ranges[i + 1]) { found = true; }
    }
    return found != node.negate;
}

static bool isWord(struct Matcher* m, long at) {
    if (at < 0 || at >= (long)m->text->size()) { return false; }
    int c = (unsigned char)(*m->text)[at];
    return isalnum(c) || c == '_';
}

static bool runSequence(struct Matcher* m, const std::vector<int>* sequence, struct Continuation* next, long at) {
    struct Continuation rest;
    rest.kind = REST_SEQUENCE;
    rest.sequence = sequence;
    rest.index = 0;
    rest.next = next;
    return run(m, &rest, at);
}

static bool repeat(struct Matcher* m, int id, int count, long previous, struct Continuation* next, long at) {
    const std::karc_regex_node& node = m->program->nodes[id];
    // A round that matched nothing would repeat forever.
    if (count > node.least && previous == at) { return run(m, next, at); }
    bool more = node.most < 0 || count < node.most;
    struct Continuation again;
    again.kind = REST_REPEAT;
    again.node = id;
    again.count = count + 1;
    again.start = at;
    again.next = next;
    if (node.lazy) {
        if (count >= node.least && run(m, next, at)) { return true; }
        return more && runSequence(m, &node.sequence, &again, at);
    }
    if (more && runSequence(m, &node.sequence, &again, at)) { return true; }
    return count >= node.least && run(m, next, at);
}

static bool matchNode(struct Matcher* m, int id, struct Continuation* next, long at) {
    const std::karc_regex_node& node = m->program->nodes[id];
    long size = (long)m->text->size();
    int kind = node.kind;
    if (kind == NODE_CHAR) {
        return at < size && sameCharacter(m, (unsigned char)(*m->text)[at], node.value) && run(m, next, at + 1);
    }
    if (kind == NODE_ANY) { return at < size && (*m->text)[at] != '\n' && run(m, next, at + 1); }
    if (kind == NODE_CLASS) { return at < size && inClass(m, node, (unsigned char)(*m->text)[at]) && run(m, next, at + 1); }
    if (kind == NODE_START) { return at == 0 && run(m, next, at); }
    if (kind == NODE_END) { return at == size && run(m, next, at); }
    if (kind == NODE_WORD_BOUNDARY) { return isWord(m, at - 1) != isWord(m, at) && run(m, next, at); }
    if (kind == NODE_NOT_WORD_BOUNDARY) { return isWord(m, at - 1) == isWord(m, at) && run(m, next, at); }
    if (kind == NODE_GROUP) {
        if (node.value < 0) { return runSequence(m, &node.sequence, next, at); }
        std::vector<long>& bounds = *m->bounds;
        long savedStart = bounds[2 * node.value];
        long savedEnd = bounds[2 * node.value + 1];
        bounds[2 * node.value] = at;
        struct Continuation close;
        close.kind = REST_GROUP_END;
        close.node = node.value;
        close.next = next;
        if (runSequence(m, &node.sequence, &close, at)) { return true; }
        bounds[2 * node.value] = savedStart;
        bounds[2 * node.value + 1] = savedEnd;
        return false;
    }
    if (kind == NODE_ALTERNATION) {
        for (size_t i = 0; i < node.alternatives.size(); i++) {
            if (runSequence(m, &node.alternatives[i], next, at)) { return true; }
        }
        return false;
    }
    if (kind == NODE_REPEAT) { return repeat(m, id, 0, -1, next, at); }
    if (kind == NODE_BACKREFERENCE) {
        std::vector<long>& bounds = *m->bounds;
        long first = bounds[2 * node.value];
        long last = bounds[2 * node.value + 1];
        if (first < 0 || last < 0) { return run(m, next, at); }
        long length = last - first;
        if (at + length > size) { return false; }
        for (long i = 0; i < length; i++) {
            if (!sameCharacter(m, (unsigned char)(*m->text)[first + i], (unsigned char)(*m->text)[at + i])) { return false; }
        }
        return run(m, next, at + length);
    }
    return false;
}

static bool run(struct Matcher* m, struct Continuation* rest, long at) {
    if (rest == 0) { return true; }
    if (rest->kind == REST_SEQUENCE) {
        if (rest->index >= rest->sequence->size()) { return run(m, rest->next, at); }
        struct Continuation after;
        after.kind = REST_SEQUENCE;
        after.sequence = rest->sequence;
        after.index = rest->index + 1;
        after.next = rest->next;
        return matchNode(m, (*rest->sequence)[rest->index], &after, at);
    }
    if (rest->kind == REST_GROUP_END) {
        std::vector<long>& bounds = *m->bounds;
        long saved = bounds[2 * rest->node + 1];
        bounds[2 * rest->node + 1] = at;
        if (run(m, rest->next, at)) { return true; }
        bounds[2 * rest->node + 1] = saved;
        return false;
    }
    if (rest->kind == REST_REPEAT) { return repeat(m, rest->node, rest->count, rest->start, rest->next, at); }
    return at == (long)m->text->size() && run(m, rest->next, at);
}

namespace std {

bool karc_regex_run(const std::basic_regex_karc& program, const std::string& text, size_t start, bool whole, std::vector<long>& bounds) {
    struct Matcher m;
    m.program = &program;
    m.text = &text;
    m.bounds = &bounds;
    size_t last = whole ? start : text.size();
    for (size_t from = start; from <= last; from++) {
        bounds.clear();
        for (int i = 0; i <= program.groups; i++) {
            bounds.push_back(-1);
            bounds.push_back(-1);
        }
        bounds[0] = (long)from;
        struct Continuation textEnd;
        textEnd.kind = REST_TEXT_END;
        textEnd.next = 0;
        struct Continuation close;
        close.kind = REST_GROUP_END;
        close.node = 0;
        close.next = whole ? &textEnd : 0;
        if (runSequence(&m, &program.top, &close, (long)from)) { return true; }
    }
    return false;
}

}

namespace std {

string smatch::format(const char* pattern) const {
    std::string out;
    for (const char* f = pattern; *f != 0; f++) {
        if (*f != '$' || f[1] == 0) {
            out += *f;
            continue;
        }
        f++;
        if (*f == '$') { out += '$'; }
        else if (*f == '&') { out += str(0); }
        else if (*f == '`') { out += prefix(); }
        else if (*f == '\'') { out += suffix(); }
        else if (*f >= '0' && *f <= '9') {
            size_t n = (size_t)(*f - '0');
            if (f[1] >= '0' && f[1] <= '9' && n * 10 + (size_t)(f[1] - '0') < size()) {
                f++;
                n = n * 10 + (size_t)(*f - '0');
            }
            if (n < size()) { out += str(n); }
        } else {
            out += '$';
            out += *f;
        }
    }
    return out;
}

string regex_replace(const string& text, const regex& pattern, const string& format, int flags) {
    std::string out;
    size_t from = 0;
    size_t copied = 0;
    std::vector<long> bounds;
    while (from <= text.size() && karc_regex_run(pattern.karc_program(), text, from, false, bounds)) {
        std::smatch match;
        match.karc_set(text, bounds);
        out += text.substr(copied, (size_t)bounds[0] - copied);
        out += match.format(format.c_str());
        copied = (size_t)bounds[1];
        from = bounds[1] == bounds[0] ? (size_t)bounds[1] + 1 : (size_t)bounds[1];
        if (bounds[1] == bounds[0] && (size_t)bounds[0] < text.size()) {
            out += text[(size_t)bounds[0]];
            copied++;
        }
        if ((flags & 8) != 0) { break; }
    }
    if (copied < text.size()) { out += text.substr(copied); }
    return out;
}

}
