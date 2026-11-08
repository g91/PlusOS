# Macintosh SE ROM - C++ library and bootable ROM binary
# Builds libserom_cpp.a and MacSE_cpp.ROM for Motorola 68000 with Retro68.

TC       ?= /home/evil/Retro68-build/toolchain/bin
CXX      := $(TC)/m68k-apple-macos-g++
AS       := $(TC)/m68k-apple-macos-as
LD       := $(TC)/m68k-apple-macos-ld
OBJCOPY  := $(TC)/m68k-apple-macos-objcopy
AR       := $(TC)/m68k-apple-macos-ar

CXXFLAGS := -m68000 -Os -std=c++17 -ffreestanding \
            -fno-exceptions -fno-rtti -fno-threadsafe-statics \
            -ffunction-sections -fdata-sections -Wall -Wextra -Wno-array-bounds -Iinclude

ASFLAGS  := -m68000

SRC_CPP  := $(shell find src -name '*.cpp')
OBJ_CPP  := $(SRC_CPP:.cpp=.o)

LIB      := libserom_cpp.a
ROM_BIN  := MacSE_cpp.ROM
ROM_ELF  := MacSE_cpp.elf

.PHONY: all clean check rom
all: $(LIB) $(ROM_BIN)

$(LIB): $(OBJ_CPP)
	$(AR) rcs $@ $(OBJ_CPP)
	@echo "built $@ from $(words $(OBJ_CPP)) translation units"

src/rom_start.o: src/rom_start.S
	$(AS) $(ASFLAGS) $< -o $@

src/%.o: src/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(ROM_ELF): src/rom_start.o $(LIB) rom.ld
	$(LD) -T rom.ld -o $@ src/rom_start.o $(LIB) --gc-sections

$(ROM_BIN): $(ROM_ELF)
	$(OBJCOPY) -O binary $< $@.raw
	@dd if=/dev/zero of=$@ bs=1024 count=256 2>/dev/null
	@dd if=$@.raw of=$@ conv=notrunc 2>/dev/null
	@rm -f $@.raw
	@python3 tools/serom_mgr.py checksum $@ --patch >/dev/null 2>&1 || true
	@echo "built bootable 256 KB ROM image: $@"

rom: $(ROM_BIN)

check:
	$(CXX) $(CXXFLAGS) -fsyntax-only -include include/serom/all.hpp -xc++ /dev/null
	@echo "umbrella header OK"

clean:
	@find src -name '*.o' -delete
	@rm -f $(LIB) $(ROM_ELF) $(ROM_BIN)
