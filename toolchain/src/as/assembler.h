// P68 assembler - public API.
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace p68 {

struct AsOptions {
    int cpu = 68000;             // 68000, 68008, 68010, 68020, 68030, 68040, 68060, 68332 (CPU32)
    bool fpu = false;            // 68881/68882 (implied true for 68040/060 unless disabled)
    bool motorolaSyntax = false; // registers without % allowed, Motorola directives
    std::vector<std::string> includeDirs;
    std::vector<std::pair<std::string, std::string>> defsyms; // --defsym
    // --- additions (all optional) ---
    bool noFpu = false;          // disable the FPU even on 68040/68060
    bool mmu = false;            // 68851 PMMU instructions on a 68020
    bool regPrefixOptional = false; // accept d0 as well as %d0 without the rest of Motorola mode
};

// Assemble `source` (already C-preprocessed if it was a .S). Returns true on success;
// `elf` receives an ELF32 big-endian EM_68K relocatable object; `diag` gets
// "file:line: error: ..." lines.
bool assemble(const std::string& source, const std::string& fileName, const AsOptions& opts,
              std::vector<unsigned char>& elf, std::string& diag);

} // namespace p68
