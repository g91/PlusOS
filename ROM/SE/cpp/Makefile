# Macintosh SE ROM - C++ scaffold library
# Builds libserom_cpp.a for the 68000 with the PlusOS Retro68 toolchain.
# Run inside WSL Ubuntu:  make

TC       ?= /home/evil/Retro68-build/toolchain/bin
CXX      := $(TC)/m68k-apple-macos-g++
AR       := $(TC)/m68k-apple-macos-ar

CXXFLAGS := -m68000 -Os -std=c++17 -ffreestanding \
            -fno-exceptions -fno-rtti -fno-threadsafe-statics \
            -ffunction-sections -fdata-sections -Wall -Wextra -Iinclude

SRC := $(shell find src -name '*.cpp')
OBJ := $(SRC:.cpp=.o)

LIB := libserom_cpp.a

.PHONY: all clean check
all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)
	@echo "built $@ from $(words $(OBJ)) translation units"

src/%.o: src/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Prove the umbrella header is self-consistent (every module header parses together).
check:
	$(CXX) $(CXXFLAGS) -fsyntax-only -include include/serom/all.hpp -xc++ /dev/null
	@echo "umbrella header OK"

clean:
	@find src -name '*.o' -delete
	@rm -f $(LIB)
