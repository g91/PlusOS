// p68-as - command line front end of the P68 assembler.
#include "assembler.h"
#include "../common/elf.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace p68;

static void usage() {
    fprintf(stderr,
            "usage: p68-as [-m68000|-m68010|-m68020|-m68030|-m68040|-m68060|-mcpu32|-mcpu=NAME|-march=NAME]\n"
            "              [-m68881] [-m68851] [-mno-float] [--motorola] [--register-prefix-optional]\n"
            "              [-I dir] [--defsym sym=value] -o out.o in.s\n");
}

static bool cpuFromName(const std::string& n0, AsOptions& o) {
    std::string n = n0;
    if (!n.empty() && n[0] == 'm') n = n.substr(1);
    if (n.compare(0, 2, "c6") == 0) n = n.substr(1);
    if (n == "cpu32") { o.cpu = 68332; return true; }
    if (n == "68881" || n == "68882") { o.fpu = true; return true; }
    if (n == "68851") { o.mmu = true; return true; }
    int v = atoi(n.c_str());
    if (v == 68000 || v == 68008 || v == 68010 || v == 68020 || v == 68030 || v == 68040 || v == 68060 || (v >= 68302 && v <= 68360)) {
        o.cpu = v;
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    AsOptions o;
    std::string out = "a.out", in;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { if (i + 1 >= argc) { usage(); exit(2); } return argv[++i]; };
        if (a == "-o") out = next();
        else if (a == "-I") o.includeDirs.push_back(next());
        else if (a.compare(0, 2, "-I") == 0) o.includeDirs.push_back(a.substr(2));
        else if (a == "--defsym" || a == "-defsym") {
            std::string d = next();
            size_t eq = d.find('=');
            if (eq == std::string::npos) { fprintf(stderr, "p68-as: bad --defsym '%s'\n", d.c_str()); return 2; }
            o.defsyms.push_back(std::make_pair(d.substr(0, eq), d.substr(eq + 1)));
        } else if (a == "--motorola" || a == "-motorola" || a == "--mri" || a == "-M") o.motorolaSyntax = true;
        else if (a == "--register-prefix-optional") o.regPrefixOptional = true;
        else if (a == "-mno-float" || a == "-mno-68881" || a == "-msoft-float") o.noFpu = true;
        else if (a.compare(0, 6, "-mcpu=") == 0 || a.compare(0, 7, "-march=") == 0) {
            if (!cpuFromName(a.substr(a.find('=') + 1), o)) { fprintf(stderr, "p68-as: unknown cpu '%s'\n", a.c_str()); return 2; }
        } else if (a.compare(0, 2, "-m") == 0 && cpuFromName(a.substr(1), o)) {
        } else if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (a == "--version") { printf("p68-as (P68 toolchain) 1.0\n"); return 0; }
        else if (!a.empty() && a[0] == '-' && a.size() > 1) {
            // unknown GNU as options are ignored (-g, --no-warn, -W, -a..., --32, -l, ...)
        } else in = a;
    }
    if (in.empty()) { usage(); return 2; }
    std::string src;
    if (!readFile(in, src)) { fprintf(stderr, "p68-as: cannot read '%s'\n", in.c_str()); return 1; }
    std::vector<unsigned char> elf;
    std::string diag;
    bool ok = assemble(src, in, o, elf, diag);
    if (!diag.empty()) fputs(diag.c_str(), stderr);
    if (!ok) return 1;
    if (!writeFile(out, elf)) { fprintf(stderr, "p68-as: cannot write '%s'\n", out.c_str()); return 1; }
    return 0;
}
