// P68 toolchain - shared ELF32 big-endian (EM_68K) definitions and small helpers.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace p68 {

enum : uint32_t {
    SHT_NULL = 0, SHT_PROGBITS = 1, SHT_SYMTAB = 2, SHT_STRTAB = 3, SHT_RELA = 4,
    SHT_NOTE = 7, SHT_NOBITS = 8, SHT_REL = 9, SHT_INIT_ARRAY = 14, SHT_FINI_ARRAY = 15,
    SHT_PREINIT_ARRAY = 16, SHT_GROUP = 17
};
enum : uint32_t {
    SHF_WRITE = 1, SHF_ALLOC = 2, SHF_EXECINSTR = 4, SHF_MERGE = 0x10, SHF_STRINGS = 0x20,
    SHF_INFO_LINK = 0x40, SHF_GROUP = 0x200, SHF_TLS = 0x400, SHF_EXCLUDE = 0x80000000u
};
enum : uint16_t { SHN_UNDEF = 0, SHN_ABS = 0xFFF1, SHN_COMMON = 0xFFF2 };
enum { STB_LOCAL = 0, STB_GLOBAL = 1, STB_WEAK = 2 };
enum { STT_NOTYPE = 0, STT_OBJECT = 1, STT_FUNC = 2, STT_SECTION = 3, STT_FILE = 4, STT_COMMON = 5 };
enum { R_68K_NONE = 0, R_68K_32 = 1, R_68K_16 = 2, R_68K_8 = 3, R_68K_PC32 = 4, R_68K_PC16 = 5, R_68K_PC8 = 6 };
enum { EM_68K = 4, ET_REL = 1, ET_EXEC = 2 };

typedef std::vector<unsigned char> Bytes;

inline uint16_t rd16(const unsigned char* p) { return (uint16_t)((p[0] << 8) | p[1]); }
inline uint32_t rd32(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
inline void wr16(unsigned char* p, uint32_t v) { p[0] = (unsigned char)(v >> 8); p[1] = (unsigned char)v; }
inline void wr32(unsigned char* p, uint32_t v) {
    p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v;
}
inline void put8(Bytes& b, uint32_t v) { b.push_back((unsigned char)v); }
inline void put16(Bytes& b, uint32_t v) { b.push_back((unsigned char)(v >> 8)); b.push_back((unsigned char)v); }
inline void put32(Bytes& b, uint32_t v) { put16(b, v >> 16); put16(b, v & 0xFFFF); }

inline bool readFile(const std::string& path, Bytes& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.clear();
    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.insert(out.end(), buf, buf + n);
    fclose(f);
    return true;
}
inline bool readFile(const std::string& path, std::string& out) {
    Bytes b;
    if (!readFile(path, b)) return false;
    out.assign(b.begin(), b.end());
    return true;
}
inline bool writeFile(const std::string& path, const unsigned char* data, size_t n) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = n == 0 || fwrite(data, 1, n, f) == n;
    return fclose(f) == 0 && ok;
}
inline bool writeFile(const std::string& path, const Bytes& b) { return writeFile(path, b.data(), b.size()); }

// Last path component, accepting both separators.
inline std::string baseName(const std::string& p) {
    size_t i = p.find_last_of("/\\");
    return i == std::string::npos ? p : p.substr(i + 1);
}
inline std::string dirName(const std::string& p) {
    size_t i = p.find_last_of("/\\");
    return i == std::string::npos ? std::string() : p.substr(0, i + 1);
}

} // namespace p68
