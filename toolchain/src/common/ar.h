// P68 toolchain - reader/writer for GNU-format `ar` archives (symbol index "/" and
// long-name table "//"), shared by p68-ld and p68-ar.
#pragma once
#include "elf.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace p68 {

struct ArMember {
    std::string name;
    size_t hdrOff = 0;   // offset of the 60-byte member header
    size_t off = 0;      // offset of the data
    size_t size = 0;
};
struct ArSymbol { std::string name; size_t hdrOff; };

inline bool isArchive(const Bytes& b) { return b.size() >= 8 && memcmp(b.data(), "!<arch>\n", 8) == 0; }

// Parses the archive. `hasIndex` tells whether a "/" symbol index was present.
inline bool parseArchive(const Bytes& b, std::vector<ArMember>& members, std::vector<ArSymbol>& index, bool& hasIndex, std::string& err) {
    hasIndex = false;
    if (!isArchive(b)) { err = "not an archive"; return false; }
    size_t p = 8;
    std::string longNames;
    while (p + 60 <= b.size()) {
        const unsigned char* h = &b[p];
        if (h[58] != '`' || h[59] != '\n') { err = "corrupt archive member header"; return false; }
        std::string nm((const char*)h, 16), sz((const char*)h + 48, 10);
        size_t size = (size_t)strtoul(sz.c_str(), nullptr, 10);
        size_t data = p + 60;
        if (data + size > b.size()) { err = "truncated archive"; return false; }
        while (!nm.empty() && nm.back() == ' ') nm.pop_back();
        if (nm == "/") {
            hasIndex = true;
            if (size >= 4) {
                uint32_t n = rd32(&b[data]);
                size_t names = data + 4 + (size_t)n * 4;
                for (uint32_t i = 0; i < n && names < data + size; i++) {
                    size_t e = names;
                    while (e < data + size && b[e]) e++;
                    index.push_back(ArSymbol{std::string((const char*)&b[names], e - names), (size_t)rd32(&b[data + 4 + (size_t)i * 4])});
                    names = e + 1;
                }
            }
        } else if (nm == "//") {
            longNames.assign((const char*)&b[data], size);
        } else if (nm == "/SYM64/") {
            err = "64-bit archive index is not supported"; return false;
        } else {
            ArMember m;
            if (nm.size() > 1 && nm[0] == '/' && nm[1] >= '0' && nm[1] <= '9') {
                size_t o = (size_t)strtoul(nm.c_str() + 1, nullptr, 10);
                size_t e = o;
                while (e < longNames.size() && longNames[e] != '\n') e++;
                m.name = o < longNames.size() ? longNames.substr(o, e - o) : nm;
                if (!m.name.empty() && m.name.back() == '/') m.name.pop_back();
            } else {
                if (!nm.empty() && nm.back() == '/') nm.pop_back();
                m.name = nm;
            }
            m.hdrOff = p; m.off = data; m.size = size;
            members.push_back(m);
        }
        p = data + size + (size & 1);
    }
    return true;
}

// Global defined symbols of an ELF32 BE relocatable object (for archive indexes).
inline void elfDefinedGlobals(const unsigned char* d, size_t n, std::vector<std::string>& out) {
    if (n < 52 || memcmp(d, "\x7f" "ELF", 4) != 0 || d[4] != 1 || d[5] != 2) return;
    uint32_t shoff = rd32(d + 0x20);
    uint32_t shentsize = rd16(d + 0x2E), shnum = rd16(d + 0x30);
    if (shentsize < 40 || (uint64_t)shoff + (uint64_t)shnum * shentsize > n) return;
    for (uint32_t i = 0; i < shnum; i++) {
        const unsigned char* s = d + shoff + i * shentsize;
        if (rd32(s + 4) != SHT_SYMTAB) continue;
        uint32_t off = rd32(s + 16), size = rd32(s + 20), link = rd32(s + 24);
        if (link >= shnum) continue;
        const unsigned char* st = d + shoff + link * shentsize;
        uint32_t stroff = rd32(st + 16), strsize = rd32(st + 20);
        if ((uint64_t)off + size > n || (uint64_t)stroff + strsize > n) continue;
        for (uint32_t k = 1; k < size / 16; k++) {
            const unsigned char* e = d + off + k * 16;
            uint32_t name = rd32(e), info = e[12], shndx = rd16(e + 14);
            int bind = (int)(info >> 4);
            if ((bind != STB_GLOBAL && bind != STB_WEAK) || shndx == SHN_UNDEF || name >= strsize) continue;
            out.push_back(std::string((const char*)d + stroff + name));
        }
    }
}

struct ArInput { std::string name; Bytes data; };

// Builds a GNU archive with symbol index and long-name table.
inline Bytes buildArchive(const std::vector<ArInput>& in) {
    std::string longNames;
    std::vector<std::string> hdrNames;
    for (const ArInput& m : in) {
        std::string n = baseName(m.name);
        if (n.size() > 15) { hdrNames.push_back("/" + std::to_string(longNames.size())); longNames += n + "/\n"; }
        else hdrNames.push_back(n + "/");
    }
    std::vector<std::vector<std::string>> syms(in.size());
    size_t nsyms = 0, namesLen = 0;
    for (size_t i = 0; i < in.size(); i++) {
        elfDefinedGlobals(in[i].data.data(), in[i].data.size(), syms[i]);
        nsyms += syms[i].size();
        for (const std::string& s : syms[i]) namesLen += s.size() + 1;
    }
    auto header = [](Bytes& out, const std::string& name, size_t size) {
        char h[96];
        snprintf(h, sizeof h, "%-16s%-12s%-6s%-6s%-8s%-10lu`\n", name.c_str(), "0", "0", "0", "644", (unsigned long)size);
        out.insert(out.end(), h, h + 60);
    };
    size_t idxSize = 4 + nsyms * 4 + namesLen;
    size_t pos = 8 + 60 + idxSize + (idxSize & 1);
    if (!longNames.empty()) pos += 60 + longNames.size() + (longNames.size() & 1);
    std::vector<size_t> offs(in.size());
    for (size_t i = 0; i < in.size(); i++) { offs[i] = pos; pos += 60 + in[i].data.size() + (in[i].data.size() & 1); }
    Bytes out;
    out.insert(out.end(), (const unsigned char*)"!<arch>\n", (const unsigned char*)"!<arch>\n" + 8);
    header(out, "/", idxSize);
    put32(out, (uint32_t)nsyms);
    for (size_t i = 0; i < in.size(); i++) for (size_t k = 0; k < syms[i].size(); k++) put32(out, (uint32_t)offs[i]);
    for (size_t i = 0; i < in.size(); i++) for (const std::string& s : syms[i]) { out.insert(out.end(), s.begin(), s.end()); out.push_back(0); }
    if (out.size() & 1) out.push_back('\n');
    if (!longNames.empty()) {
        header(out, "//", longNames.size());
        out.insert(out.end(), longNames.begin(), longNames.end());
        if (out.size() & 1) out.push_back('\n');
    }
    for (size_t i = 0; i < in.size(); i++) {
        header(out, hdrNames[i], in[i].data.size());
        out.insert(out.end(), in[i].data.begin(), in[i].data.end());
        if (out.size() & 1) out.push_back('\n');
    }
    return out;
}

} // namespace p68
