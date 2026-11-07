# PlusOS SDK for the Macintosh Plus - standalone library build
#
#   make            build the libraries into build/lib
#   make test       compile-check the SDK in C and C++
#   make clean
#
# The headers and C files under include/plusrom and src/plusrom are generated
# from the Macintosh Plus ROM by PlusOS's tools/sdkgen (`build sdks` in PlusOS).
PLUSOS_SDK := $(abspath .)
include sdk.mk

B   := build
OBJ  = $(patsubst %,$(B)/obj/%.o,$(1))

LIBC_SRC   := $(wildcard src/libc/*.c) $(wildcard src/libc/*.S)
ROM_SRC    := $(wildcard src/plusrom/*.c) $(wildcard src/plusrom/glue/*.c)
PLUSOS_SRC := src/runtime/rt_common.c src/runtime/cxx.cpp src/runtime/font8x8.c
RTAPP_SRC  := src/runtime/rt_app.c src/runtime/thread.c
GFX_SRC    := src/gfx/gfx.c src/gfx/gfx_asm.S src/gfx/plus3d.c
# Graphics is the hot path: optimize for speed, not size (the 68000 pays dearly
# for -Os register spills to slow RAM in per-pixel and per-scanline loops).
$(B)/obj/src/gfx/%.c.o: PLUSOS_CFLAGS += -O2

.PHONY: all test clean
all: $(addprefix $(B)/lib/,libc.a libplusrom.a libplusos.a librt_app.a libplusgfx.a crt0_app.o crt0_kernel.o)
	@echo "Macintosh Plus SDK libraries ready in $(B)/lib"

$(B)/obj/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC   $<"; $(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS) -c $< -o $@
$(B)/obj/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "  CXX  $<"; $(CXX) $(PLUSOS_CPPFLAGS) $(PLUSOS_CXXFLAGS) -c $< -o $@
$(B)/obj/%.S.o: %.S
	@mkdir -p $(dir $@)
	@echo "  AS   $<"; $(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_ASFLAGS) -c $< -o $@

$(B)/lib/libc.a: $(call OBJ,$(LIBC_SRC))
$(B)/lib/libplusrom.a: $(call OBJ,$(ROM_SRC))
$(B)/lib/libplusos.a: $(call OBJ,$(PLUSOS_SRC))
$(B)/lib/librt_app.a: $(call OBJ,$(RTAPP_SRC))
$(B)/lib/libplusgfx.a: $(call OBJ,$(GFX_SRC))
$(B)/lib/%.a:
	@mkdir -p $(dir $@)
	@rm -f $@
	@echo "  AR   $@ ($(words $^) objects)"; $(AR) rcs $@ $^

$(B)/lib/crt0_app.o: src/runtime/crt0.S include/plusos/exe.h.inc
	@mkdir -p $(dir $@)
	$(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_ASFLAGS) -c $< -o $@
$(B)/lib/crt0_kernel.o: src/runtime/crt0.S include/plusos/exe.h.inc
	@mkdir -p $(dir $@)
	$(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_ASFLAGS) -DPLUSOS_KERNEL -c $< -o $@

# The test: every header compiles in C and C++; calls this ROM lacks are rejected.
test: all
	@mkdir -p $(B)/test
	$(CC)  $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS)   -c tests/sdktest.c   -o $(B)/test/c.o
	$(CXX) $(PLUSOS_CPPFLAGS) $(PLUSOS_CXXFLAGS) -c tests/sdktest.cpp -o $(B)/test/cpp.o
	@if $(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS) -DEXPECT_UNAVAILABLE -c tests/sdktest.c -o /dev/null 2>$(B)/test/u.txt; \
	  then echo "FAIL: calls that are not in the Macintosh Plus ROM compiled"; exit 1; \
	  else echo "rejected (not in the Macintosh Plus ROM):"; grep -o "is unavailable: .*" $(B)/test/u.txt | sed 's/^/  /'; fi
	@if [ "plus" = "se" ]; then \
	  $(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS) -DADB_TEST -c tests/sdktest.c -o $(B)/test/adb.o && \
	  echo "ADB Manager calls compile (they are in the Mac SE ROM)"; \
	elif $(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS) -DADB_TEST -c tests/sdktest.c -o /dev/null 2>$(B)/test/adb.txt; then \
	  echo "FAIL: ADB calls compiled for the Mac Plus"; exit 1; \
	else echo "rejected (ADB needs a Mac SE):"; grep -o "is unavailable: .*" $(B)/test/adb.txt | sed 's/^/  /'; fi
	@echo "Macintosh Plus SDK OK: headers compile in C and C++"

clean:
	rm -rf $(B)
