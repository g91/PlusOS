// pinject.cpp - native Windows console tool to inject a freshly-compiled
// kernel / app / loader into a PlusOS .hda WITHOUT rebuilding the whole OS.
//
// 100% native: reuses Disk Studio's own DiskImage + HFSFileSystem (no WSL,
// no hfsutils, no Python).
//
//   pinject info    <image>
//   pinject ls      <image> [ :path ]
//   pinject kernel  <image> <kernel.pex>           (raw kernel block)
//   pinject loader  <image> <loader.bin>           (boot blocks)
//   pinject app     <image> <app.pex> [NAME]       (-> :bin:NAME)
//   pinject put     <image> <localfile> <:hfsname>
//   pinject extract <image> <:hfspath> <localfile>
//   pinject rm      <image> <:hfspath>
//
// Build (MSVC):  cl /EHsc /O2 pinject.cpp disk_image.cpp hfs_filesystem.cpp
#include "disk_image.h"
#include "hfs_filesystem.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>

// must match the Makefile install defaults (partition-relative 512-byte blocks)
static const uint32_t KERNEL_BLOCK  = 60000;
static const uint32_t KERNEL_BLOCKS = 512;
static const uint32_t PLOS = 0x504C4F53; // 'PLOS'

static std::wstring to_w(const char* s) {
    std::wstring w; while (*s) w.push_back((wchar_t)(unsigned char)*s++); return w;
}
static std::string to_upper(std::string s) { for (char& c : s) if (c >= 'a' && c <= 'z') c -= 32; return s; }
static std::string base_noext(const char* path) {
    std::string p = path; size_t sl = p.find_last_of("/\\"); if (sl != std::string::npos) p = p.substr(sl + 1);
    size_t dot = p.find_last_of('.'); if (dot != std::string::npos) p = p.substr(0, dot); return p;
}
static bool read_local(const char* path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? n : 0); if (n > 0) { if (fread(out.data(), 1, n, f) != (size_t)n) { fclose(f); return false; } }
    fclose(f); return true;
}
static bool write_local(const char* path, const std::vector<uint8_t>& d) {
    FILE* f = fopen(path, "wb"); if (!f) return false;
    if (!d.empty()) fwrite(d.data(), 1, d.size(), f); fclose(f); return true;
}

static int die(const char* msg) { fprintf(stderr, "pinject: %s\n", msg); return 1; }

static int do_kernel(DiskImage& disk, const char* file, uint32_t block, const char* label) {
    std::vector<uint8_t> data;
    if (!read_local(file, data)) return die("cannot read input file");
    uint32_t cap = KERNEL_BLOCKS * 512;
    if (block == KERNEL_BLOCK && data.size() > cap) { fprintf(stderr, "pinject: %s is %zu bytes, region holds %u\n", label, data.size(), cap); return 1; }
    uint32_t blocks = (uint32_t)((data.size() + 511) / 512);
    std::vector<uint8_t> buf(blocks * 512, 0);
    memcpy(buf.data(), data.data(), data.size());
    if (!disk.write_hfs_blocks(block, blocks, buf.data())) return die("write_hfs_blocks failed");
    disk.flush();
    printf("  injected %s: %zu bytes (%u blocks) at HFS block %u\n", label, data.size(), blocks, block);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: pinject <info|ls|kernel|loader|app|put|extract|rm> <image> [args]\n");
        return 1;
    }
    std::string cmd = argv[1];
    const char* image = argv[2];

    DiskImage disk;
    std::string err;
    bool ro = (cmd == "info" || cmd == "ls" || cmd == "extract");
    if (!disk.open(to_w(image), ro, &err)) { fprintf(stderr, "pinject: cannot open image: %s\n", err.c_str()); return 1; }

    if (cmd == "kernel") return do_kernel(disk, argv[3], KERNEL_BLOCK, "kernel");
    if (cmd == "loader") {
        std::vector<uint8_t> d; if (!read_local(argv[3], d)) return die("cannot read loader");
        if (d.size() > 1024) return die("loader exceeds 1024-byte boot blocks");
        std::vector<uint8_t> buf(1024, 0); memcpy(buf.data(), d.data(), d.size());
        if (!disk.write_hfs_blocks(0, 2, buf.data())) return die("write boot blocks failed");
        disk.flush(); printf("  injected loader: %zu bytes into boot blocks\n", d.size()); return 0;
    }

    // HFS file operations
    HFSFileSystem fs;
    if (!fs.mount(&disk, &err)) { fprintf(stderr, "pinject: HFS mount failed: %s\n", err.c_str()); return 1; }

    if (cmd == "info") {
        printf("  volume '%s'  %.1f MB total, %.1f MB free\n", fs.get_volume_name().c_str(),
               fs.get_total_bytes() / 1048576.0, fs.get_free_bytes() / 1048576.0);
        printf("  files %u  folders %u  alloc block %u bytes\n", fs.get_total_files(), fs.get_total_folders(), fs.get_alloc_block_size());
        printf("  kernel block %u (max %u bytes)\n", KERNEL_BLOCK, KERNEL_BLOCKS * 512);
        return 0;
    }
    if (cmd == "ls") {
        const char* path = argc > 3 ? argv[3] : ":";
        const HFSItem* dir = fs.find_by_path(path);
        if (!dir) return die("path not found");
        for (const HFSItem& it : fs.get_children(dir->cnid)) {
            char t[5] = {0}; uint32_t ty = it.type;
            t[0] = (char)(ty >> 24); t[1] = (char)(ty >> 16); t[2] = (char)(ty >> 8); t[3] = (char)ty;
            printf("  %-28s %-6s %8u bytes  cnid=%u\n", it.name.c_str(),
                   it.is_directory ? "<DIR>" : t, it.data_size, it.cnid);
        }
        return 0;
    }
    if (cmd == "app") {
        std::vector<uint8_t> data; if (!read_local(argv[3], data)) return die("cannot read app .pex");
        std::string name = argc > 4 ? to_upper(argv[4]) : to_upper(base_noext(argv[3]));
        const HFSItem* bin = fs.find_by_path("bin");
        if (!bin) return die("no :bin folder on the volume");
        // replace any existing copy
        for (const HFSItem& it : fs.get_children(bin->cnid))
            if (it.name == name) { fs.delete_item(it.cnid); break; }
        uint32_t bin_cnid = bin->cnid;
        if (!fs.import_file(bin_cnid, name, PLOS, PLOS, data)) return die("import_file failed");
        disk.flush();
        // verify: re-mount and confirm the catalog is still consistent
        fs.unmount();
        HFSFileSystem v;
        if (!v.mount(&disk, &err) || !v.find_by_path("bin") ||
            !v.find_by_path((std::string("bin:") + name).c_str())) {
            fprintf(stderr,
                "pinject: WARNING - the HFS catalog could not be re-read after inserting into\n"
                "         :bin (a known B-tree-split limitation on full volumes). The image may\n"
                "         be inconsistent - do NOT boot it; restore from a backup and add new\n"
                "         apps with a full 'make install'. (Kernel/loader/root-file inject are safe.)\n");
            return 2;
        }
        printf("  injected app -> :bin:%s (%zu bytes)  [verified]\n", name.c_str(), data.size());
        return 0;
    }
    if (cmd == "put") {
        std::vector<uint8_t> data; if (!read_local(argv[3], data)) return die("cannot read local file");
        std::string hfsname = argv[4]; if (!hfsname.empty() && hfsname[0] == ':') hfsname.erase(hfsname.begin());
        for (const HFSItem& it : fs.get_children(2))
            if (it.name == hfsname) { fs.delete_item(it.cnid); break; }
        if (!fs.import_file(2, hfsname, PLOS, 0x54455854 /*'TEXT'*/, data)) return die("import_file failed");
        disk.flush(); printf("  put -> :%s (%zu bytes)\n", hfsname.c_str(), data.size()); return 0;
    }
    if (cmd == "extract") {
        const HFSItem* it = fs.find_by_path(argv[3]);
        if (!it) return die("file not found in image");
        std::vector<uint8_t> data;
        if (!fs.read_file_fork(*it, false, data)) return die("read_file_fork failed");
        if (!write_local(argv[4], data)) return die("cannot write local file");
        printf("  extracted %s -> %s (%zu bytes)\n", argv[3], argv[4], data.size()); return 0;
    }
    if (cmd == "rm") {
        const HFSItem* it = fs.find_by_path(argv[3]);
        if (!it) return die("file not found");
        if (!fs.delete_item(it->cnid)) return die("delete failed");
        disk.flush(); printf("  removed %s\n", argv[3]); return 0;
    }
    return die("unknown command");
}
