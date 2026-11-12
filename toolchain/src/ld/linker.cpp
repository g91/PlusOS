// p68-ld - linker of the P68 toolchain.
//
// Links ELF32 big-endian EM_68K relocatable objects and GNU `ar` archives into
//   * PlusOS .pex executables (what GNU ld + plusos.ld + mkexe.py produce),
//   * flat binaries (--oformat binary -Ttext ADDR),
//   * ELF executables (--oformat elf, optionally with -q/--emit-relocs).
// The section layout is the built-in equivalent of sdks/*/ld/plusos.ld; input sections
// are placed in the order GNU ld uses (files in load order, sections in file order),
// including its merging of SHF_MERGE string sections, so the output is byte-identical.
#include "../common/ar.h"
#include "../common/elf.h"
#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace p68;

namespace {

struct Obj;
struct Global;

struct Rel { uint32_t off; uint32_t sym; int32_t addend; uint8_t type; };

struct MGroup;
struct InSec {
    Obj* obj = nullptr;
    int index = 0;
    std::string name;
    uint32_t type = 0, flags = 0, align = 1, size = 0, entsize = 0;
    const unsigned char* data = nullptr;
    std::vector<Rel> rels;
    bool live = false, discarded = false, keep = false;
    int phase = -1;        // placement class, see classify()
    int prio = 0;          // init/fini priority
    uint32_t addr = 0;
    // string/constant merging
    MGroup* mg = nullptr;
    std::vector<std::pair<uint32_t, int>> mmap; // input offset -> entry
    uint32_t outSize = 0;
    Bytes merged;
};

struct Sym {
    std::string name;
    uint32_t value = 0, size = 0;
    int bind = 0, type = 0;
    uint32_t shndx = 0;
    Global* g = nullptr;
};

struct Obj {
    std::string name;
    const unsigned char* d = nullptr;
    size_t n = 0;
    std::deque<InSec> secs;
    std::vector<Sym> syms;
    std::unique_ptr<InSec> common; // synthetic COMMON section
};

struct Global {
    std::string name;
    Obj* obj = nullptr;       // defining object
    InSec* sec = nullptr;
    uint32_t value = 0, size = 0;
    int type = 0;
    bool defined = false, weak = false, abs = false, common = false, linkerDef = false;
    bool undefStrong = false; // referenced by a strong undefined symbol
    uint32_t calign = 1;
    uint32_t addr = 0;
    std::string refBy;
};

struct MEnt { const unsigned char* p; uint32_t len; uint32_t align; int suffix; uint32_t index; };
struct MGroup {
    uint32_t flags, entsize, align;
    std::vector<InSec*> chain;
    std::vector<MEnt> ents;
    std::unordered_map<std::string, int> hash;
    InSec* first = nullptr;
};

enum { PH_CRT0, PH_TEXT, PH_RODATA, PH_ORPHAN_RO, PH_PREINIT, PH_INIT_SORTED, PH_INIT, PH_FINI_SORTED, PH_FINI, PH_DATA, PH_ORPHAN_RW, PH_BSS, PH_COUNT };
enum { FMT_ELF, FMT_PEX, FMT_BIN };

struct Linker {
    // options
    std::string outName = "a.out", mapName, entry = "_start", pexName;
    int format = -1;
    bool gc = false, emitRelocs = false, wholeArchive = false;
    bool keepMacsbug = true; // Retro68's ld keeps "<section>.macsbug" together with <section>
    uint32_t textAddr = 0, dataAddr = 0, bssAddr = 0;
    bool haveText = false, haveData = false, haveBss = false;
    std::vector<std::string> libDirs, undefs;
    std::vector<std::pair<std::string, uint32_t>> defsyms;

    int errors = 0;
    std::deque<Bytes> buffers;
    std::deque<Obj> objs;
    std::unordered_map<std::string, Global*> gtab;
    std::deque<Global> gpool;
    std::vector<Global*> gorder;
    std::unordered_map<std::string, bool> comdat; // seen group signatures / linkonce names
    std::deque<MGroup> mgroups;
    std::vector<std::string> pulled; // "archive(member) by symbol"

    struct Archive { std::string name; const Bytes* data; std::vector<ArMember> members; std::vector<ArSymbol> index; std::vector<char> loaded; std::unordered_map<size_t, size_t> byOff; };
    std::deque<Archive> archives;

    std::vector<InSec*> order[PH_COUNT];
    uint32_t textStart = 0, textEnd = 0, dataStart = 0, dataEnd = 0, bssStart = 0, bssEnd = 0;
    uint32_t initStart = 0, initEnd = 0, finiEnd = 0;
    Bytes image; // [textStart, dataEnd)

    void error(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        if (++errors <= 50) { fprintf(stderr, "p68-ld: error: "); vfprintf(stderr, fmt, ap); fputc('\n', stderr); }
        va_end(ap);
    }
    void warning(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "p68-ld: warning: "); vfprintf(stderr, fmt, ap); fputc('\n', stderr);
        va_end(ap);
    }

    Global* global(const std::string& name) {
        auto it = gtab.find(name);
        if (it != gtab.end()) return it->second;
        gpool.emplace_back();
        Global* g = &gpool.back();
        g->name = name;
        gtab[name] = g;
        gorder.push_back(g);
        return g;
    }

    // ---------------------------------------------------------------- input
    bool loadObject(const std::string& name, const unsigned char* d, size_t n) {
        if (n < 52 || memcmp(d, "\x7f" "ELF", 4) != 0) { error("%s: not an ELF object", name.c_str()); return false; }
        if (d[4] != 1 || d[5] != 2 || rd16(d + 18) != EM_68K) { error("%s: not a 32-bit big-endian m68k ELF object", name.c_str()); return false; }
        if (rd16(d + 16) != ET_REL) { error("%s: not a relocatable object", name.c_str()); return false; }
        uint32_t shoff = rd32(d + 0x20), shentsize = rd16(d + 0x2E), shnum = rd16(d + 0x30), shstrndx = rd16(d + 0x32);
        if (shentsize < 40 || (uint64_t)shoff + (uint64_t)shnum * shentsize > n || shstrndx >= shnum) { error("%s: corrupt section headers", name.c_str()); return false; }
        objs.emplace_back();
        Obj& o = objs.back();
        o.name = name; o.d = d; o.n = n;
        auto sh = [&](uint32_t i, int field) { return rd32(d + shoff + i * shentsize + (uint32_t)field * 4); };
        uint32_t strOff = sh(shstrndx, 4);
        o.secs.resize(shnum);
        uint32_t symtab = 0;
        for (uint32_t i = 0; i < shnum; i++) {
            InSec& s = o.secs[i];
            s.obj = &o; s.index = (int)i;
            s.name = (const char*)d + strOff + sh(i, 0);
            s.type = sh(i, 1); s.flags = sh(i, 2); s.size = sh(i, 5); s.align = sh(i, 8) ? sh(i, 8) : 1; s.entsize = sh(i, 9);
            if (s.type != SHT_NOBITS) {
                if ((uint64_t)sh(i, 4) + s.size > n) { error("%s: section %s is truncated", name.c_str(), s.name.c_str()); return false; }
                s.data = d + sh(i, 4);
            }
            s.outSize = s.size;
            if (s.type == SHT_SYMTAB) symtab = i;
            if (s.type == SHT_REL) { error("%s: REL relocation sections are not supported", name.c_str()); return false; }
        }
        // symbols
        if (symtab) {
            uint32_t off = sh(symtab, 4), cnt = sh(symtab, 5) / 16, strs = sh(sh(symtab, 6), 4);
            o.syms.resize(cnt);
            for (uint32_t i = 0; i < cnt; i++) {
                const unsigned char* e = d + off + i * 16;
                Sym& s = o.syms[i];
                s.name = (const char*)d + strs + rd32(e);
                s.value = rd32(e + 4); s.size = rd32(e + 8); s.bind = e[12] >> 4; s.type = e[12] & 15; s.shndx = rd16(e + 14);
            }
        }
        // COMDAT groups and link-once sections
        for (uint32_t i = 0; i < shnum; i++) {
            InSec& s = o.secs[i];
            if (s.type == SHT_GROUP && s.size >= 4 && (rd32(s.data) & 1)) {
                uint32_t si = sh(i, 7);
                std::string sig = si < o.syms.size() ? o.syms[si].name : std::string();
                if (sig.empty() && si < o.syms.size() && o.syms[si].shndx < shnum) sig = o.secs[o.syms[si].shndx].name;
                bool dup = comdat.count("G:" + sig) != 0;
                comdat["G:" + sig] = true;
                if (dup) for (uint32_t k = 4; k + 4 <= s.size; k += 4) { uint32_t m = rd32(s.data + k); if (m < shnum) o.secs[m].discarded = true; }
            } else if (s.name.compare(0, 14, ".gnu.linkonce.") == 0) {
                if (comdat.count("L:" + s.name)) s.discarded = true;
                comdat["L:" + s.name] = true;
            }
        }
        // relocations
        for (uint32_t i = 0; i < shnum; i++) {
            if (o.secs[i].type != SHT_RELA) continue;
            uint32_t tgt = sh(i, 7);
            if (tgt >= shnum) continue;
            InSec& t = o.secs[tgt];
            const unsigned char* r = o.secs[i].data;
            t.rels.reserve(o.secs[i].size / 12);
            for (uint32_t k = 0; k + 12 <= o.secs[i].size; k += 12) {
                Rel x;
                x.off = rd32(r + k); x.sym = rd32(r + k + 4) >> 8; x.type = r[k + 7]; x.addend = (int32_t)rd32(r + k + 8);
                if (x.sym >= o.syms.size()) { error("%s: bad relocation symbol index", name.c_str()); return false; }
                t.rels.push_back(x);
            }
        }
        // classification and merge chains
        for (uint32_t i = 0; i < shnum; i++) {
            InSec& s = o.secs[i];
            classify(s);
            if (s.phase >= 0 && !s.discarded && (s.flags & SHF_MERGE) && s.entsize && s.type == SHT_PROGBITS && s.rels.empty()) {
                MGroup* mg = nullptr;
                for (MGroup& g : mgroups) if (g.flags == (s.flags & (SHF_MERGE | SHF_STRINGS)) && g.entsize == s.entsize && g.align == s.align) { mg = &g; break; }
                if (!mg) { mgroups.emplace_back(); mg = &mgroups.back(); mg->flags = s.flags & (SHF_MERGE | SHF_STRINGS); mg->entsize = s.entsize; mg->align = s.align; }
                mg->chain.push_back(&s);
                s.mg = mg;
            }
        }
        // global symbols
        for (size_t i = 1; i < o.syms.size(); i++) {
            Sym& s = o.syms[i];
            if (s.bind == STB_LOCAL) continue;
            Global* g = global(s.name);
            s.g = g;
            bool secDead = s.shndx != SHN_UNDEF && s.shndx < shnum && o.secs[s.shndx].discarded;
            if (s.shndx == SHN_UNDEF || secDead) {
                if (s.bind != STB_WEAK && !g->undefStrong) { g->undefStrong = true; if (g->refBy.empty()) g->refBy = o.name; }
                continue;
            }
            if (s.shndx == SHN_COMMON) {
                if (g->defined) continue;
                if (!g->common) { g->common = true; g->obj = &o; g->size = 0; g->calign = 1; }
                if (s.size > g->size) g->size = s.size;
                if (s.value > g->calign) g->calign = s.value;
                continue;
            }
            bool weak = s.bind == STB_WEAK;
            if (g->defined && !g->weak) {
                if (!weak) error("duplicate definition of `%s' in %s (first defined in %s)", s.name.c_str(), o.name.c_str(), g->obj ? g->obj->name.c_str() : "linker");
                continue;
            }
            if (g->defined && weak) continue;
            g->defined = true; g->weak = weak; g->common = false; g->obj = &o; g->type = s.type; g->size = s.size;
            if (s.shndx == SHN_ABS) { g->abs = true; g->sec = nullptr; g->value = s.value; }
            else if (s.shndx < shnum) { g->abs = false; g->sec = &o.secs[s.shndx]; g->value = s.value; }
            else { error("%s: symbol `%s' has an unsupported section index", o.name.c_str(), s.name.c_str()); }
        }
        return true;
    }

    static bool starts(const std::string& n, const char* p) { return n.compare(0, strlen(p), p) == 0; }
    // name == base or name starts with base + "."
    static bool family(const std::string& n, const char* base) {
        size_t l = strlen(base);
        return n.compare(0, l, base) == 0 && (n.size() == l || n[l] == '.');
    }
    void classify(InSec& s) {
        s.phase = -1;
        if (!(s.flags & SHF_ALLOC)) return;
        if (s.type != SHT_PROGBITS && s.type != SHT_NOBITS && s.type != SHT_INIT_ARRAY && s.type != SHT_FINI_ARRAY && s.type != SHT_PREINIT_ARRAY && s.type != SHT_NOTE) return;
        const std::string& n = s.name;
        if (n == ".comment" || family(n, ".note") || n == ".eh_frame" || n == ".eh_frame_hdr" || starts(n, ".gcc_except_table") || n == ".jcr" ||
            n == ".got" || n == ".got.plt" || starts(n, ".igot")) return;
        auto prio = [&](const char* base, bool ctor) {
            const char* p = n.c_str() + strlen(base) + 1;
            int v = atoi(p);
            s.prio = ctor ? 65535 - v : v;
        };
        if (n == ".text.crt0") { s.phase = PH_CRT0; s.keep = true; }
        else if (family(n, ".text") || starts(n, ".gnu.linkonce.t.")) s.phase = PH_TEXT;
        else if (family(n, ".rodata") || starts(n, ".gnu.linkonce.r.")) s.phase = PH_RODATA;
        else if (n == ".preinit_array") { s.phase = PH_PREINIT; s.keep = true; }
        else if (starts(n, ".init_array.")) { s.phase = PH_INIT_SORTED; s.keep = true; prio(".init_array", false); }
        else if (starts(n, ".ctors.")) { s.phase = PH_INIT_SORTED; s.keep = true; prio(".ctors", true); }
        else if (n == ".init_array" || n == ".ctors") { s.phase = PH_INIT; s.keep = true; }
        else if (starts(n, ".fini_array.")) { s.phase = PH_FINI_SORTED; s.keep = true; prio(".fini_array", false); }
        else if (starts(n, ".dtors.")) { s.phase = PH_FINI_SORTED; s.keep = true; prio(".dtors", true); }
        else if (n == ".fini_array" || n == ".dtors") { s.phase = PH_FINI; s.keep = true; }
        else if (family(n, ".data") || starts(n, ".gnu.linkonce.d.")) s.phase = PH_DATA;
        else if (family(n, ".bss") || starts(n, ".gnu.linkonce.b.") || n == "COMMON") s.phase = PH_BSS;
        else if (s.type == SHT_NOBITS) s.phase = PH_BSS;
        else if (s.flags & SHF_WRITE) s.phase = PH_ORPHAN_RW;
        else s.phase = PH_ORPHAN_RO;
    }

    bool isUndefined(const Global* g) const { return !g->defined && !g->common && g->undefStrong; }

    bool loadMember(Archive& a, size_t mi, const std::string& why) {
        if (a.loaded[mi]) return false;
        a.loaded[mi] = 1;
        const ArMember& m = a.members[mi];
        std::string nm = a.name + "(" + m.name + ")";
        if (m.size < 4 || memcmp(a.data->data() + m.off, "\x7f" "ELF", 4) != 0) return false;
        pulled.push_back(nm + " (" + why + ")");
        loadObject(nm, a.data->data() + m.off, m.size);
        return true;
    }
    // One GNU-ld style scan of an archive; returns true when members were added.
    bool scanArchive(Archive& a) {
        bool any = false, loaded;
        do {
            loaded = false;
            for (const ArSymbol& s : a.index) {
                auto mi = a.byOff.find(s.hdrOff);
                if (mi == a.byOff.end() || a.loaded[mi->second]) continue;
                auto it = gtab.find(s.name);
                if (it == gtab.end() || !isUndefined(it->second)) continue;
                if (loadMember(a, mi->second, s.name)) { loaded = true; any = true; }
            }
        } while (loaded);
        return any;
    }
    Archive* openArchive(const std::string& name, const Bytes* data) {
        archives.emplace_back();
        Archive& a = archives.back();
        a.name = name; a.data = data;
        bool hasIndex;
        std::string err;
        if (!parseArchive(*data, a.members, a.index, hasIndex, err)) { error("%s: %s", name.c_str(), err.c_str()); archives.pop_back(); return nullptr; }
        a.loaded.assign(a.members.size(), 0);
        for (size_t i = 0; i < a.members.size(); i++) a.byOff[a.members[i].hdrOff] = i;
        if (!hasIndex) { // no symbol index: build one by scanning the members
            for (const ArMember& m : a.members) {
                std::vector<std::string> names;
                elfDefinedGlobals(data->data() + m.off, m.size, names);
                for (const std::string& s : names) a.index.push_back(ArSymbol{s, m.hdrOff});
            }
        }
        return &a;
    }

    // ---------------------------------------------------------------- symbol addresses
    uint32_t mapOff(const InSec* s, uint32_t off) const;
    uint32_t secBase(const InSec* s) const { return (s->mg && s->mg->first) ? s->mg->first->addr : s->addr; }
    uint32_t gAddr(const Global* g) const {
        if (g->linkerDef || g->abs || g->common) return g->abs ? g->value : g->addr;
        if (g->sec) return mapOff(g->sec, g->value);
        return 0;
    }
    InSec* gSec(const Global* g) const { return (g->defined && !g->abs) ? g->sec : (g->common && g->obj ? g->obj->common.get() : nullptr); }

    void defineLinker(const char* name, uint32_t addr) {
        Global* g = global(name);
        if (g->defined && !g->linkerDef) { error("symbol `%s' is reserved for the linker but defined in %s", name, g->obj ? g->obj->name.c_str() : "?"); return; }
        g->defined = true; g->linkerDef = true; g->weak = false; g->common = false; g->abs = false; g->sec = nullptr; g->addr = addr;
    }

    void gcSections();
    void mergeSections();
    void layout();
    void relocate(std::vector<uint32_t>& pexRelocs, std::vector<Bytes>* emitted);
    bool writeOutput();
    void writeMap();
    int run(int argc, char** argv);
};

uint32_t Linker::mapOff(const InSec* s, uint32_t off) const {
    if (!s->mg || !s->mg->first || s->mmap.empty()) return s->addr + off;
    // last map entry whose input offset is <= off
    size_t lo = 0, hi = s->mmap.size();
    while (hi - lo > 1) { size_t mid = (lo + hi) / 2; if (s->mmap[mid].first <= off) lo = mid; else hi = mid; }
    const MEnt& e = s->mg->ents[(size_t)s->mmap[lo].second];
    return s->mg->first->addr + e.index + (off - s->mmap[lo].first);
}

void Linker::gcSections() {
    std::vector<InSec*> work;
    auto mark = [&](InSec* s) {
        if (!s || s->live || s->discarded || s->phase < 0) return;
        s->live = true;
        work.push_back(s);
        if (keepMacsbug && gc && s->obj) {
            // MacsBug symbol records live in "<function section>.macsbug" and are referenced by nothing
            std::string want = s->name + ".macsbug";
            for (InSec& t : s->obj->secs)
                if (!t.live && !t.discarded && t.phase >= 0 && t.name == want) { t.live = true; work.push_back(&t); }
        }
    };
    for (Obj& o : objs)
        for (InSec& s : o.secs) {
            if (s.phase < 0 || s.discarded) continue;
            if (!gc || s.keep) mark(&s);
        }
    if (gc) {
        auto root = [&](const std::string& n, bool must) {
            auto it = gtab.find(n);
            if (it == gtab.end() || !gSec(it->second)) { if (must) warning("cannot find entry symbol `%s'", n.c_str()); return; }
            mark(gSec(it->second));
        };
        root(entry, format != FMT_BIN);
        for (const std::string& u : undefs) root(u, false);
    }
    while (!work.empty()) {
        InSec* s = work.back();
        work.pop_back();
        if (!s->obj) continue;
        for (const Rel& r : s->rels) {
            const Sym& y = s->obj->syms[r.sym];
            if (y.g) mark(gSec(y.g));
            else if (y.shndx != SHN_UNDEF && y.shndx < s->obj->secs.size()) mark(&s->obj->secs[y.shndx]);
        }
    }
}

// Same algorithm as BFD's merge.c: unique entries in first-seen order, strings additionally
// tail-merged; everything ends up in the first live section of the group.
void Linker::mergeSections() {
    for (MGroup& g : mgroups) {
        bool strings = (g.flags & SHF_STRINGS) != 0;
        uint32_t es = g.entsize;
        auto add = [&](const unsigned char* p, uint32_t len, uint32_t align) -> int {
            std::string key((const char*)p, len);
            auto it = g.hash.find(key);
            if (it != g.hash.end()) { if (g.ents[(size_t)it->second].align < align) g.ents[(size_t)it->second].align = align; return it->second; }
            g.ents.push_back(MEnt{p, len, align, -1, 0});
            g.hash[key] = (int)g.ents.size() - 1;
            return (int)g.ents.size() - 1;
        };
        static const unsigned char zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (InSec* s : g.chain) {
            if (!s->live) continue;
            if (!g.first) g.first = s;
            const unsigned char* c = s->data;
            uint32_t end = s->size, mask = g.align - 1;
            if (strings) {
                bool nul = false;
                for (uint32_t p = 0; p < end;) {
                    uint32_t ea = p;
                    ea = ((ea ^ (ea - 1)) + 1) >> 1;
                    if (!ea || ea > mask) ea = mask + 1;
                    uint32_t q = p;
                    for (;;) {
                        if (q + es > end) { q = end; break; }
                        bool z = true;
                        for (uint32_t k = 0; k < es; k++) if (c[q + k]) z = false;
                        q += es;
                        if (z) break;
                    }
                    s->mmap.push_back(std::make_pair(p, add(c + p, q - p, ea)));
                    p = q;
                    while (p + es <= end) {
                        bool z = true;
                        for (uint32_t k = 0; k < es; k++) if (c[p + k]) z = false;
                        if (!z) break;
                        if (!nul && !(p & mask)) { nul = true; s->mmap.push_back(std::make_pair(p, add(zeros, es > 8 ? 8 : es, mask + 1))); }
                        p += es;
                    }
                }
            } else {
                for (uint32_t p = 0; p + es <= end; p += es) s->mmap.push_back(std::make_pair(p, add(c + p, es, g.align)));
            }
        }
        if (!g.first) continue;
        if (strings && !g.ents.empty()) {
            std::vector<int> arr;
            uint32_t alignment = 0;
            for (size_t i = 0; i < g.ents.size(); i++) {
                arr.push_back((int)i);
                g.ents[i].len -= es;
                if (g.ents[i].align != alignment) alignment = alignment == 0 ? g.ents[i].align : 0xFFFFFFFFu;
            }
            bool plain = alignment != 0xFFFFFFFFu && alignment <= es;
            std::sort(arr.begin(), arr.end(), [&](int ia, int ib) {
                const MEnt& A = g.ents[(size_t)ia];
                const MEnt& B = g.ents[(size_t)ib];
                uint32_t la = A.len, lb = B.len;
                if (!plain) {
                    uint32_t ta = la & (A.align - 1), tb = lb & (B.align - 1);
                    if (ta != tb) return ta < tb;
                }
                uint32_t l = la < lb ? la : lb;
                const unsigned char* sa = A.p + la - 1;
                const unsigned char* sb = B.p + lb - 1;
                while (l) { if (*sa != *sb) return *sa < *sb; sa--; sb--; l--; }
                if (la != lb) return la < lb;
                return ia < ib;
            });
            auto isSuffix = [&](const MEnt& A, const MEnt& B) {
                if (B.len >= A.len) return false; // B must be strictly shorter (lengths include the terminator here)
                return memcmp(A.p + (A.len - B.len), B.p, B.len - 1) == 0;
            };
            size_t a = arr.size() - 1;
            MEnt* e = &g.ents[(size_t)arr[a]];
            int ei = arr[a];
            e->len += es;
            while (a-- > 0) {
                MEnt* cmp = &g.ents[(size_t)arr[a]];
                cmp->len += es;
                if (e->align >= cmp->align && !((e->len - cmp->len) & (cmp->align - 1)) && isSuffix(*e, *cmp)) { cmp->suffix = ei; cmp->align = 0; }
                else { e = cmp; ei = arr[a]; }
            }
        }
        uint32_t size = 0;
        for (MEnt& e : g.ents)
            if (e.align) { size = (size + e.align - 1) & ~(e.align - 1); e.index = size; size += e.len; }
        for (MEnt& e : g.ents)
            if (!e.align && e.suffix >= 0) { const MEnt& s = g.ents[(size_t)e.suffix]; e.index = s.index + (s.len - e.len); }
        for (InSec* s : g.chain) if (s->live) s->outSize = 0;
        g.first->outSize = size;
        g.first->merged.assign(size, 0);
        for (const MEnt& e : g.ents) if (e.align) memcpy(g.first->merged.data() + e.index, e.p, e.len);
    }
}

void Linker::layout() {
    // input section order: files in load order, sections in file order, per placement class
    for (Obj& o : objs) {
        for (InSec& s : o.secs) if (s.live) order[s.phase].push_back(&s);
        if (o.common && o.common->live) order[PH_BSS].push_back(o.common.get());
    }
    for (int ph : {PH_INIT_SORTED, PH_FINI_SORTED})
        std::stable_sort(order[ph].begin(), order[ph].end(), [](const InSec* a, const InSec* b) { return a->prio < b->prio; });
    for (int ph : {PH_ORPHAN_RO, PH_ORPHAN_RW})
        for (InSec* s : order[ph]) warning("%s: orphan section `%s' placed in %s", s->obj->name.c_str(), s->name.c_str(), ph == PH_ORPHAN_RO ? ".text" : ".data");

    auto place = [&](int ph, uint32_t& dot) {
        for (InSec* s : order[ph]) {
            dot = (dot + s->align - 1) & ~(s->align - 1);
            s->addr = dot;
            dot += s->outSize;
        }
    };
    auto maxAlign = [&](std::initializer_list<int> phs) {
        uint32_t a = 1;
        for (int ph : phs) for (InSec* s : order[ph]) if (s->align > a) a = s->align;
        return a;
    };
    uint32_t dot = textAddr;
    uint32_t ta = maxAlign({PH_CRT0, PH_TEXT, PH_RODATA, PH_ORPHAN_RO, PH_PREINIT, PH_INIT_SORTED, PH_INIT, PH_FINI_SORTED, PH_FINI});
    dot = (dot + ta - 1) & ~(ta - 1);
    textStart = dot;
    place(PH_CRT0, dot); place(PH_TEXT, dot); place(PH_RODATA, dot); place(PH_ORPHAN_RO, dot);
    dot = (dot + 3) & ~3u;
    initStart = dot;
    place(PH_PREINIT, dot); place(PH_INIT_SORTED, dot); place(PH_INIT, dot);
    initEnd = dot;
    place(PH_FINI_SORTED, dot); place(PH_FINI, dot);
    finiEnd = dot;
    textEnd = dot;
    if (haveData) dot = dataAddr;
    uint32_t da = maxAlign({PH_DATA, PH_ORPHAN_RW});
    dot = (dot + da - 1) & ~(da - 1);
    dataStart = dot;
    place(PH_DATA, dot); place(PH_ORPHAN_RW, dot);
    dot = (dot + 3) & ~3u;
    dataEnd = dot;
    if (haveBss) dot = bssAddr;
    uint32_t ba = maxAlign({PH_BSS});
    dot = (dot + ba - 1) & ~(ba - 1);
    bssStart = dot;
    place(PH_BSS, dot);
    dot = (dot + 3) & ~3u;
    bssEnd = dot;

    defineLinker("__image_start", textStart);
    defineLinker("__init_array_start", initStart);
    defineLinker("__init_array_end", initEnd);
    defineLinker("__fini_array_start", initEnd);
    defineLinker("__fini_array_end", finiEnd);
    defineLinker("__image_end", dataEnd);
    defineLinker("__bss_start", bssStart);
    defineLinker("__bss_end", bssEnd);
    for (Global* g : gorder) if (g->common && g->obj && g->obj->common) g->addr = g->obj->common->addr + g->value;
}

void Linker::relocate(std::vector<uint32_t>& pexRelocs, std::vector<Bytes>* emitted) {
    if (dataEnd < textStart) { error("data section lies below the text section"); return; }
    image.assign(dataEnd - textStart, 0);
    for (int ph = 0; ph < PH_BSS; ph++)
        for (InSec* s : order[ph]) {
            if (!s->outSize) continue;
            if (s->addr < textStart || s->addr + s->outSize > dataEnd) { error("section %s of %s lies outside the image", s->name.c_str(), s->obj->name.c_str()); continue; }
            unsigned char* dst = &image[s->addr - textStart];
            if (s->mg && s->mg->first == s) memcpy(dst, s->merged.data(), s->outSize);
            else if (s->type == SHT_NOBITS) {}
            else memcpy(dst, s->data, s->outSize);
        }
    int undefReported = 0;
    for (int ph = 0; ph < PH_COUNT; ph++)
        for (InSec* s : order[ph]) {
            if (s->rels.empty() || !s->obj) continue;
            if (s->type == SHT_NOBITS) continue;
            Obj& o = *s->obj;
            for (const Rel& r : s->rels) {
                const Sym& y = o.syms[r.sym];
                uint32_t S = 0;
                int64_t A = r.addend;
                bool isAbs = false, undefWeak = false;
                int tkind = 1; // output section of the target: 1 text, 2 data, 3 bss
                auto kindOf = [&](const InSec* t) { return t->phase == PH_BSS ? 3 : (t->phase == PH_DATA || t->phase == PH_ORPHAN_RW) ? 2 : 1; };
                std::string symName = y.name;
                if (y.g) {
                    Global* g = y.g;
                    if (g->defined || g->common) {
                        S = gAddr(g); isAbs = g->abs;
                        if (g->linkerDef) tkind = g->name == "__image_end" ? 2 : (g->name == "__bss_start" || g->name == "__bss_end") ? 3 : 1;
                        else if (gSec(g)) tkind = kindOf(gSec(g));
                    }
                    else if (!g->undefStrong || y.bind == STB_WEAK) {
                        if (g->undefStrong) { /* strong reference elsewhere decides */ }
                        undefWeak = true;
                    }
                    if (!g->defined && !g->common && g->undefStrong) {
                        if (undefReported++ < 30) error("%s(%s+0x%x): undefined reference to `%s'", o.name.c_str(), s->name.c_str(), r.off, g->name.c_str());
                        else errors++;
                        continue;
                    }
                } else if (r.sym == 0 || y.shndx == SHN_ABS) { S = y.value; isAbs = true; }
                else if (y.shndx == SHN_UNDEF || y.shndx >= o.secs.size()) { error("%s(%s+0x%x): relocation against undefined local symbol", o.name.c_str(), s->name.c_str(), r.off); continue; }
                else {
                    InSec* t = &o.secs[y.shndx];
                    if (symName.empty()) symName = t->name;
                    tkind = kindOf(t);
                    if (!t->live) {
                        // reference into a discarded (garbage-collected or duplicate COMDAT) section
                        S = 0; isAbs = true;
                        if (s->phase >= 0 && !t->discarded) error("%s(%s+0x%x): reference to discarded section `%s'", o.name.c_str(), s->name.c_str(), r.off, t->name.c_str());
                    } else if (t->mg && y.type == STT_SECTION) { S = mapOff(t, (uint32_t)(y.value + A)); A = 0; }
                    else S = mapOff(t, y.value);
                }
                uint32_t P = s->addr + r.off;
                if (r.off + (r.type == R_68K_32 || r.type == R_68K_PC32 ? 4u : r.type == R_68K_16 || r.type == R_68K_PC16 ? 2u : 1u) > s->size) {
                    error("%s(%s+0x%x): relocation outside of section", o.name.c_str(), s->name.c_str(), r.off);
                    continue;
                }
                unsigned char* dst = &image[P - textStart];
                int64_t v = (int64_t)S + A;
                bool pcrel = r.type >= R_68K_PC32 && r.type <= R_68K_PC8;
                if (pcrel) v -= P;
                auto overflow = [&](const char* tn) {
                    error("%s(%s+0x%x): relocation %s against `%s' out of range (value %lld)", o.name.c_str(), s->name.c_str(), r.off, tn, symName.c_str(), (long long)v);
                };
                switch (r.type) {
                case R_68K_NONE: continue;
                case R_68K_32: case R_68K_PC32: wr32(dst, (uint32_t)v); break;
                case R_68K_16: if (v < -32768 || v > 65535) overflow("R_68K_16"); wr16(dst, (uint32_t)v); break;
                case R_68K_PC16: if (v < -32768 || v > 32767) overflow("R_68K_PC16"); wr16(dst, (uint32_t)v); break;
                case R_68K_8: if (v < -128 || v > 255) overflow("R_68K_8"); dst[0] = (unsigned char)v; break;
                case R_68K_PC8: if (v < -128 || v > 127) overflow("R_68K_PC8"); dst[0] = (unsigned char)v; break;
                default: error("%s(%s+0x%x): unsupported relocation type %d", o.name.c_str(), s->name.c_str(), r.off, r.type); continue;
                }
                bool relocatable = !isAbs && !undefWeak;
                if (format == FMT_PEX && !pcrel && relocatable) {
                    if (r.type == R_68K_32) {
                        if (P & 1) error("%s(%s+0x%x): odd relocation address 0x%X (%s)", o.name.c_str(), s->name.c_str(), r.off, P, symName.c_str());
                        pexRelocs.push_back(P);
                    } else error("%s(%s+0x%x): R_68K_%s to relocatable symbol %s at 0x%X cannot be relocated", o.name.c_str(), s->name.c_str(), r.off,
                                 r.type == R_68K_16 ? "16" : "8", symName.c_str(), P);
                }
                if (emitted) {
                    // (offset, type, kind, value): kind 0 abs, 1 text-relative, 2 data, 3 bss, 4 undefined weak
                    int kind = undefWeak ? 4 : isAbs ? 0 : tkind;
                    uint32_t tv = (uint32_t)((int64_t)S + A);
                    Bytes& e = (*emitted)[ph >= PH_DATA ? 1 : 0];
                    put32(e, P); put32(e, ((uint32_t)kind << 8) | r.type); put32(e, tv);
                }
            }
        }
}

void Linker::writeMap() {
    FILE* f = fopen(mapName.c_str(), "wb");
    if (!f) { error("cannot write map file '%s'", mapName.c_str()); return; }
    fprintf(f, "Archive member included to satisfy reference by file (symbol)\n\n");
    for (const std::string& p : pulled) fprintf(f, "%s\n", p.c_str());
    fprintf(f, "\nDiscarded input sections\n\n");
    for (Obj& o : objs)
        for (InSec& s : o.secs)
            if (s.phase >= 0 && !s.live && s.size) fprintf(f, " %-30s 0x%08x %10s %s\n", s.name.c_str(), 0u, ("0x" + [&] { char b[16]; snprintf(b, sizeof b, "%x", s.size); return std::string(b); }()).c_str(), o.name.c_str());
    fprintf(f, "\nLinker script and memory map\n\n");
    struct Out { const char* name; uint32_t a, e; std::vector<int> phs; };
    std::vector<Out> outs;
    outs.push_back(Out{".text", textStart, textEnd, {PH_CRT0, PH_TEXT, PH_RODATA, PH_ORPHAN_RO, PH_PREINIT, PH_INIT_SORTED, PH_INIT, PH_FINI_SORTED, PH_FINI}});
    outs.push_back(Out{".data", dataStart, dataEnd, {PH_DATA, PH_ORPHAN_RW}});
    outs.push_back(Out{".bss", bssStart, bssEnd, {PH_BSS}});
    std::multimap<const InSec*, const Global*> symsBySec;
    for (Global* g : gorder) if (gSec(g) && gSec(g)->live) symsBySec.insert(std::make_pair(gSec(g), g));
    for (const Out& o : outs) {
        fprintf(f, "%-15s 0x%08x %#10x\n", o.name, o.a, o.e - o.a);
        for (int ph : o.phs)
            for (InSec* s : order[ph]) {
                fprintf(f, " %s\n                0x%08x %#10x %s\n", s->name.c_str(), s->addr, s->outSize, s->obj ? s->obj->name.c_str() : "");
                std::vector<const Global*> v;
                auto range = symsBySec.equal_range(s);
                for (auto it = range.first; it != range.second; ++it) v.push_back(it->second);
                std::stable_sort(v.begin(), v.end(), [&](const Global* a, const Global* b) { return gAddr(a) < gAddr(b); });
                for (const Global* g : v) fprintf(f, "                0x%08x                %s\n", gAddr(g), g->name.c_str());
            }
    }
    fprintf(f, "\nSymbols by address\n\n");
    std::vector<const Global*> all;
    for (Global* g : gorder) if (g->linkerDef || g->abs || (gSec(g) && gSec(g)->live)) all.push_back(g);
    std::stable_sort(all.begin(), all.end(), [&](const Global* a, const Global* b) { return gAddr(a) < gAddr(b); });
    for (const Global* g : all) fprintf(f, "0x%08x %c %s\n", gAddr(g), g->abs ? 'A' : g->linkerDef ? 'L' : g->weak ? 'W' : 'T', g->name.c_str());
    fclose(f);
}

bool Linker::writeOutput() {
    std::vector<uint32_t> pexRelocs;
    std::vector<Bytes> emitted(2);
    relocate(pexRelocs, (format == FMT_ELF && emitRelocs) ? &emitted : nullptr);
    if (errors) return false;
    Bytes out;
    if (format == FMT_PEX) {
        if (textStart != 0) { error("a .pex image must be linked at address 0"); return false; }
        if (image.size() < 0x30 || memcmp(&image[4], "PLOS", 4) != 0) { error("no PLOS header at offset 4 (crt0 must be linked first)"); return false; }
        std::sort(pexRelocs.begin(), pexRelocs.end());
        pexRelocs.erase(std::unique(pexRelocs.begin(), pexRelocs.end()), pexRelocs.end());
        uint32_t relocOffset = (uint32_t)image.size();
        uint32_t fileSize = relocOffset + 4 * (uint32_t)pexRelocs.size();
        uint32_t memSize = fileSize > bssEnd ? fileSize : bssEnd;
        memSize = (memSize + 3) & ~3u;
        wr32(&image[0x0C], fileSize); wr32(&image[0x10], memSize); wr32(&image[0x14], relocOffset); wr32(&image[0x18], (uint32_t)pexRelocs.size());
        memset(&image[0x24], 0, 12);
        memcpy(&image[0x24], pexName.data(), pexName.size() > 11 ? 11 : pexName.size());
        out = image;
        for (uint32_t r : pexRelocs) put32(out, r);
    } else if (format == FMT_BIN) {
        out = image;
    } else {
        // ELF executable: .text, .data, .bss (+ .rela.text/.rela.data), .symtab, .strtab, .shstrtab
        struct ESym { std::string name; uint32_t value, size; int info; uint32_t shndx; };
        std::vector<ESym> es;
        es.push_back(ESym{"", 0, 0, 0, 0});
        const uint32_t TEXT = 1, DATA = 2, BSS = 3;
        es.push_back(ESym{"", textStart, 0, STT_SECTION, TEXT});
        es.push_back(ESym{"", dataStart, 0, STT_SECTION, DATA});
        es.push_back(ESym{"", bssStart, 0, STT_SECTION, BSS});
        uint32_t firstGlobal = (uint32_t)es.size();
        uint32_t weakUndef = 0;
        for (Global* g : gorder) {
            bool def = g->linkerDef || g->abs || g->common || (g->defined && g->sec && g->sec->live);
            if (!def) {
                if (!g->defined && !g->common && !g->undefStrong) { es.push_back(ESym{g->name, 0, 0, (STB_WEAK << 4), SHN_UNDEF}); if (!weakUndef) weakUndef = (uint32_t)es.size() - 1; }
                continue;
            }
            uint32_t a = gAddr(g), sx;
            if (g->abs) sx = SHN_ABS;
            else if (g->linkerDef) sx = (g->name == "__image_end") ? DATA : (g->name == "__bss_start" || g->name == "__bss_end") ? BSS : TEXT;
            else { const InSec* s = gSec(g); sx = s->phase == PH_BSS ? BSS : (s->phase == PH_DATA || s->phase == PH_ORPHAN_RW) ? DATA : TEXT; }
            es.push_back(ESym{g->name, a, g->size, ((g->weak ? STB_WEAK : STB_GLOBAL) << 4) | (g->type & 15), sx});
        }
        uint32_t absIdx = 0;
        struct Sh { std::string name; uint32_t type, flags, addr, link, info, align, entsize; Bytes data; uint32_t size; };
        std::vector<Sh> sh;
        sh.push_back(Sh{"", 0, 0, 0, 0, 0, 0, 0, Bytes(), 0});
        sh.push_back(Sh{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, textStart, 0, 0, 4, 0, Bytes(image.begin(), image.begin() + (long)(textEnd - textStart)), textEnd - textStart});
        sh.push_back(Sh{".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, dataStart, 0, 0, 4, 0, Bytes(image.begin() + (long)(dataStart - textStart), image.end()), dataEnd - dataStart});
        sh.push_back(Sh{".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, bssStart, 0, 0, 4, 0, Bytes(), bssEnd - bssStart});
        size_t nrela = 0;
        for (int k = 0; k < 2; k++) if (!emitted[(size_t)k].empty()) nrela++;
        uint32_t symtabIdx = (uint32_t)(4 + nrela);
        for (int k = 0; k < 2; k++) {
            const Bytes& e = emitted[(size_t)k];
            if (e.empty()) continue;
            Sh h{k ? ".rela.data" : ".rela.text", SHT_RELA, SHF_INFO_LINK, 0, symtabIdx, (uint32_t)(k ? DATA : TEXT), 4, 12, Bytes(), 0};
            for (size_t i = 0; i + 12 <= e.size(); i += 12) {
                uint32_t P = rd32(&e[i]), info = rd32(&e[i + 4]), tv = rd32(&e[i + 8]);
                uint32_t kind = info >> 8, type = info & 0xFF, symIdx = 0, addend = tv;
                if (kind == 1) { symIdx = 1; addend = tv - textStart; }
                else if (kind == 2) { symIdx = 2; addend = tv - dataStart; }
                else if (kind == 3) { symIdx = 3; addend = tv - bssStart; }
                else if (kind == 4) { symIdx = weakUndef; addend = tv; }
                else {
                    // absolute: needs an SHN_ABS symbol
                    if (!absIdx) { absIdx = (uint32_t)es.size(); es.push_back(ESym{"*ABS*", 0, 0, (STB_GLOBAL << 4), SHN_ABS}); }
                    symIdx = absIdx;
                }
                put32(h.data, P); put32(h.data, (symIdx << 8) | type); put32(h.data, addend);
            }
            h.size = (uint32_t)h.data.size();
            sh.push_back(h);
        }
        Bytes strtab(1, 0);
        {
            Sh h{".symtab", SHT_SYMTAB, 0, 0, symtabIdx + 1, firstGlobal, 4, 16, Bytes(), 0};
            for (const ESym& e : es) {
                uint32_t no = 0;
                if (!e.name.empty()) { no = (uint32_t)strtab.size(); strtab.insert(strtab.end(), e.name.begin(), e.name.end()); strtab.push_back(0); }
                put32(h.data, no); put32(h.data, e.value); put32(h.data, e.size); put8(h.data, (uint32_t)e.info); put8(h.data, 0); put16(h.data, e.shndx);
            }
            h.size = (uint32_t)h.data.size();
            sh.push_back(h);
        }
        sh.push_back(Sh{".strtab", SHT_STRTAB, 0, 0, 0, 0, 1, 0, strtab, (uint32_t)strtab.size()});
        sh.push_back(Sh{".shstrtab", SHT_STRTAB, 0, 0, 0, 0, 1, 0, Bytes(), 0});
        Bytes shstr(1, 0);
        std::vector<uint32_t> nameOff(sh.size(), 0), offs(sh.size(), 0);
        for (size_t i = 1; i < sh.size(); i++) { nameOff[i] = (uint32_t)shstr.size(); shstr.insert(shstr.end(), sh[i].name.begin(), sh[i].name.end()); shstr.push_back(0); }
        sh.back().data = shstr; sh.back().size = (uint32_t)shstr.size();
        out.assign(52 + 32, 0);
        static const unsigned char ident[8] = {0x7F, 'E', 'L', 'F', 1, 2, 1, 0};
        memcpy(out.data(), ident, 8);
        for (size_t i = 1; i < sh.size(); i++) {
            while (out.size() % 4) out.push_back(0);
            offs[i] = (uint32_t)out.size();
            out.insert(out.end(), sh[i].data.begin(), sh[i].data.end());
        }
        while (out.size() % 4) out.push_back(0);
        uint32_t shoff = (uint32_t)out.size();
        for (size_t i = 0; i < sh.size(); i++) {
            put32(out, nameOff[i]); put32(out, sh[i].type); put32(out, sh[i].flags); put32(out, sh[i].addr); put32(out, offs[i]);
            put32(out, sh[i].size); put32(out, sh[i].link); put32(out, sh[i].info); put32(out, sh[i].align); put32(out, sh[i].entsize);
        }
        auto ent = gtab.find(entry);
        wr16(&out[16], ET_EXEC); wr16(&out[18], EM_68K); wr32(&out[20], 1);
        wr32(&out[24], ent != gtab.end() && (ent->second->defined) ? gAddr(ent->second) : textStart);
        wr32(&out[28], 52); wr32(&out[32], shoff);
        wr16(&out[40], 52); wr16(&out[42], 32); wr16(&out[44], 1); wr16(&out[46], 40); wr16(&out[48], (uint32_t)sh.size()); wr16(&out[50], (uint32_t)sh.size() - 1);
        // one PT_LOAD for text+data (+bss)
        unsigned char* ph = &out[52];
        wr32(ph, 1); wr32(ph + 4, offs[1]); wr32(ph + 8, textStart); wr32(ph + 12, textStart);
        bool contiguous = dataStart == textEnd && offs[2] == offs[1] + (textEnd - textStart);
        wr32(ph + 16, contiguous ? dataEnd - textStart : textEnd - textStart);
        wr32(ph + 20, contiguous ? bssEnd - textStart : textEnd - textStart);
        wr32(ph + 24, 7); wr32(ph + 28, 4);
    }
    if (!writeFile(outName, out)) { error("cannot write '%s'", outName.c_str()); return false; }
    return true;
}

int Linker::run(int argc, char** argv) {
    struct Input { int kind; std::string name; }; // 0 file, 1 group start, 2 group end, 3 whole-archive on, 4 off
    std::vector<Input> inputs;
    auto parseNum = [&](const std::string& s) -> uint32_t { return (uint32_t)strtoul(s.c_str(), nullptr, 0); };
    std::string oformat;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { if (i + 1 >= argc) { fprintf(stderr, "p68-ld: option %s needs an argument\n", a.c_str()); exit(2); } return argv[++i]; };
        auto val = [&](const char* opt, std::string& v) -> bool { // --opt=value or --opt value
            size_t l = strlen(opt);
            if (a == opt) { v = next(); return true; }
            if (a.compare(0, l, opt) == 0 && a.size() > l && a[l] == '=') { v = a.substr(l + 1); return true; }
            return false;
        };
        std::string v;
        if (a == "-o") outName = next();
        else if (a.compare(0, 2, "-o") == 0 && a.size() > 2 && a[2] != 'f') outName = a.substr(2);
        else if (val("--oformat", v) || val("-oformat", v)) oformat = v;
        else if (val("-Map", v) || val("--Map", v)) mapName = v;
        else if (val("--name", v)) pexName = v;
        else if (val("--defsym", v) || val("-defsym", v)) {
            size_t eq = v.find('=');
            if (eq == std::string::npos) { fprintf(stderr, "p68-ld: bad --defsym\n"); return 2; }
            defsyms.push_back(std::make_pair(v.substr(0, eq), parseNum(v.substr(eq + 1))));
        } else if (val("-Ttext", v) || val("--Ttext", v) || val("-Ttext-segment", v)) { textAddr = parseNum(v); haveText = true; }
        else if (val("-Tdata", v) || val("--Tdata", v)) { dataAddr = parseNum(v); haveData = true; }
        else if (val("-Tbss", v) || val("--Tbss", v)) { bssAddr = parseNum(v); haveBss = true; }
        else if (a == "-T" || a == "--script" || a.compare(0, 9, "--script=") == 0 || (a.compare(0, 2, "-T") == 0 && a.size() > 2)) {
            std::string script = a == "-T" || a == "--script" ? next() : a[1] == 'T' ? a.substr(2) : a.substr(9);
            std::string text;
            if (!readFile(script, text)) { fprintf(stderr, "p68-ld: cannot read linker script '%s'\n", script.c_str()); return 1; }
            if (text.find("__image_start") == std::string::npos || text.find(".text.crt0") == std::string::npos) {
                fprintf(stderr, "p68-ld: %s: only the PlusOS linker script (plusos.ld) is supported; its layout is built in\n", script.c_str());
                return 1;
            }
        } else if (a == "-e" || a == "--entry") entry = next();
        else if (a.compare(0, 8, "--entry=") == 0) entry = a.substr(8);
        else if (a == "-u" || a == "--undefined") undefs.push_back(next());
        else if (a.compare(0, 12, "--undefined=") == 0) undefs.push_back(a.substr(12));
        else if (a == "-L") libDirs.push_back(next());
        else if (a.compare(0, 2, "-L") == 0) libDirs.push_back(a.substr(2));
        else if (a == "-l") inputs.push_back(Input{5, next()});
        else if (a.compare(0, 2, "-l") == 0) inputs.push_back(Input{5, a.substr(2)});
        else if (a == "--start-group" || a == "-(") inputs.push_back(Input{1, ""});
        else if (a == "--end-group" || a == "-)") inputs.push_back(Input{2, ""});
        else if (a == "--whole-archive") inputs.push_back(Input{3, ""});
        else if (a == "--no-whole-archive") inputs.push_back(Input{4, ""});
        else if (a == "--gc-sections") gc = true;
        else if (a == "--no-gc-sections") gc = false;
        else if (a == "--gc-keep-macsbug") keepMacsbug = true;
        else if (a == "--no-gc-keep-macsbug") keepMacsbug = false;
        else if (a == "-q" || a == "--emit-relocs") emitRelocs = true;
        else if (a == "-M" || a == "--print-map") mapName = "-";
        else if (a == "--version" || a == "-v") { printf("p68-ld (P68 toolchain) 1.0\n"); if (argc == 2) return 0; }
        else if (a == "--help") { printf("usage: p68-ld [options] objects/archives... -o output\n  formats: --oformat pex|binary|elf (default: by extension, .pex -> pex)\n"); return 0; }
        else if (a == "-m" || a == "-z" || a == "-soname" || a == "-h" || a == "-rpath" || a == "-rpath-link" || a == "--sysroot") next();
        else if (!a.empty() && a[0] == '-' && a.size() > 1) { /* unknown GNU ld flag: ignored */ }
        else inputs.push_back(Input{0, a});
    }
    if (oformat == "pex") format = FMT_PEX;
    else if (oformat == "binary" || oformat == "bin") format = FMT_BIN;
    else if (oformat.empty()) {
        size_t n = outName.size();
        format = (n > 4 && (outName.compare(n - 4, 4, ".pex") == 0 || outName.compare(n - 4, 4, ".PEX") == 0)) ? FMT_PEX : FMT_ELF;
    } else if (oformat.compare(0, 3, "elf") == 0) format = FMT_ELF;
    else { fprintf(stderr, "p68-ld: unknown output format '%s'\n", oformat.c_str()); return 2; }
    if (format == FMT_PEX && haveText && textAddr != 0) { fprintf(stderr, "p68-ld: .pex images are linked at address 0\n"); return 2; }
    if (inputs.empty()) { fprintf(stderr, "p68-ld: no input files\n"); return 2; }

    for (const auto& d : defsyms) {
        Global* g = global(d.first);
        g->defined = true; g->abs = true; g->value = d.second;
    }
    // load inputs in command-line order; archive groups are rescanned until nothing new is pulled
    std::vector<Archive*> group;
    bool inGroup = false;
    for (size_t ii = 0; ii < inputs.size(); ii++) {
        const Input& in = inputs[ii];
        if (in.kind == 1) { inGroup = true; group.clear(); continue; }
        if (in.kind == 2) {
            bool again = true;
            while (again) { again = false; for (Archive* a : group) if (scanArchive(*a)) again = true; }
            inGroup = false;
            continue;
        }
        if (in.kind == 3) { wholeArchive = true; continue; }
        if (in.kind == 4) { wholeArchive = false; continue; }
        std::string path = in.name;
        buffers.emplace_back();
        Bytes& b = buffers.back();
        if (in.kind == 5) {
            bool found = false;
            for (const std::string& dir : libDirs) {
                path = dir + ((dir.empty() || dir.back() == '/' || dir.back() == '\\') ? "" : "/") + "lib" + in.name + ".a";
                if (readFile(path, b)) { found = true; break; }
            }
            if (!found) { error("cannot find -l%s", in.name.c_str()); continue; }
        } else if (!readFile(path, b)) { error("cannot open '%s'", path.c_str()); continue; }
        if (isArchive(b)) {
            Archive* a = openArchive(path, &b);
            if (!a) continue;
            if (wholeArchive) { for (size_t m = 0; m < a->members.size(); m++) loadMember(*a, m, "--whole-archive"); }
            else if (inGroup) { group.push_back(a); scanArchive(*a); }
            else scanArchive(*a);
        } else loadObject(path, b.data(), b.size());
    }
    if (errors) return 1;
    // common symbols: one synthetic section per defining object
    for (Global* g : gorder) {
        if (!g->common || g->defined) continue;
        Obj* o = g->obj;
        if (!o->common) {
            o->common.reset(new InSec());
            o->common->obj = o; o->common->name = "COMMON"; o->common->type = SHT_NOBITS; o->common->flags = SHF_ALLOC | SHF_WRITE; o->common->phase = PH_BSS;
            o->common->live = true; o->common->size = 0;
        }
        InSec& c = *o->common;
        uint32_t al = g->calign ? g->calign : 1;
        c.size = (c.size + al - 1) & ~(al - 1);
        g->value = c.size;
        c.size += g->size;
        if (al > c.align) c.align = al;
        c.outSize = c.size;
    }
    gcSections();
    mergeSections();
    layout();
    if (errors) return 1;
    bool ok = writeOutput();
    if (ok && !mapName.empty() && mapName != "-") writeMap();
    return (ok && !errors) ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    Linker l;
    return l.run(argc, argv);
}
