// p68-ar - creates, lists and extracts GNU-format archives.
//   p68-ar rcs lib.a a.o b.o ...   create/replace members, write symbol index
//   p68-ar t lib.a                 list members
//   p68-ar x lib.a [member ...]    extract members into the current directory
#include "../common/ar.h"
#include <cstdio>
#include <cstdlib>

using namespace p68;

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: p68-ar {r|q|t|x|d}[csuv] archive [members...]\n");
        return 2;
    }
    std::string ops = argv[1];
    if (!ops.empty() && ops[0] == '-') ops = ops.substr(1);
    std::string arName = argv[2];
    char op = 0;
    bool verbose = false;
    for (char c : ops) {
        if (c == 'r' || c == 'q' || c == 't' || c == 'x' || c == 'd') op = c;
        else if (c == 'v') verbose = true;
        else if (c == 'c' || c == 's' || c == 'u' || c == 'D' || c == 'U' || c == 'S' || c == 'o') {}
        else { fprintf(stderr, "p68-ar: unknown operation '%c'\n", c); return 2; }
    }
    if (!op && ops.find('s') != std::string::npos) op = 'r'; // "ar s lib.a": just rewrite the index
    if (!op) { fprintf(stderr, "p68-ar: no operation given\n"); return 2; }

    std::vector<ArInput> members;
    Bytes old;
    bool exists = readFile(arName, old);
    if (exists) {
        std::vector<ArMember> ms;
        std::vector<ArSymbol> idx;
        bool hasIdx;
        std::string err;
        if (!parseArchive(old, ms, idx, hasIdx, err)) { fprintf(stderr, "p68-ar: %s: %s\n", arName.c_str(), err.c_str()); return 1; }
        for (const ArMember& m : ms) members.push_back(ArInput{m.name, Bytes(old.begin() + (long)m.off, old.begin() + (long)(m.off + m.size))});
    } else if (op == 't' || op == 'x' || op == 'd') {
        fprintf(stderr, "p68-ar: cannot open '%s'\n", arName.c_str());
        return 1;
    }
    auto wanted = [&](const std::string& n) {
        if (argc <= 3) return true;
        for (int i = 3; i < argc; i++) if (baseName(argv[i]) == n) return true;
        return false;
    };
    if (op == 't') {
        for (const ArInput& m : members) if (wanted(m.name)) printf("%s\n", m.name.c_str());
        return 0;
    }
    if (op == 'x') {
        int rc = 0;
        for (const ArInput& m : members) {
            if (!wanted(m.name)) continue;
            if (verbose) printf("x - %s\n", m.name.c_str());
            if (!writeFile(m.name, m.data)) { fprintf(stderr, "p68-ar: cannot write '%s'\n", m.name.c_str()); rc = 1; }
        }
        return rc;
    }
    if (op == 'd') {
        std::vector<ArInput> keep;
        for (const ArInput& m : members) if (argc <= 3 || !wanted(m.name)) keep.push_back(m);
        members.swap(keep);
    } else {
        for (int i = 3; i < argc; i++) {
            ArInput in;
            in.name = baseName(argv[i]);
            if (!readFile(argv[i], in.data)) { fprintf(stderr, "p68-ar: cannot read '%s'\n", argv[i]); return 1; }
            bool replaced = false;
            if (op == 'r')
                for (ArInput& m : members) if (m.name == in.name) { m.data = in.data; replaced = true; break; }
            if (!replaced) members.push_back(in);
            if (verbose) printf("%c - %s\n", replaced ? 'r' : 'a', in.name.c_str());
        }
    }
    Bytes out = buildArchive(members);
    if (!writeFile(arName, out)) { fprintf(stderr, "p68-ar: cannot write '%s'\n", arName.c_str()); return 1; }
    return 0;
}
