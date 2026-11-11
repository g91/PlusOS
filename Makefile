# PlusOS - bare-metal OS for the Macintosh Plus and Macintosh SE (68000)
#
#   make            both SDK libraries, boot loader, kernel, apps
#   make install    write boot block + kernel + apps to the disk images
#   make sdks       regenerate the two SDK folders from the ROMs (sdks/macplus, sdks/macse)
#   make sdktest    run both SDKs' self-tests
#   make clean
#
# The SDKs are separate, self-contained libraries with their own Makefiles;
# this file only builds the OS and the example apps against them.
# Apps use the Mac Plus SDK (runs on both machines) unless their folder
# contains a file named SE, which selects the Mac SE SDK.
include config.mk

BUILD  := build
UPPER   = $(shell echo $(1) | tr a-z A-Z)

# Smart MD5 build ledger — two-phase system:
#
#   Phase 1  prescan  (runs once before the build):
#     Loads build/.build_ledger.json, hashes every tracked source file, and
#     pre-touches .o files for unchanged sources so make skips their compiler
#     invocations.  Prints "compile environment instructions" — a per-file
#     build plan showing exactly what will (and won't) be recompiled.
#
#   Phase 2  SMARTCC  (wraps every individual CC/CXX/AS call):
#     On each compiler invocation checks the ledger; on a hit touches .o and
#     returns immediately; on a miss compiles and writes the new hash back.
#     Covers .c, .cpp, .S, .s, .asm files for both cross-compiler and host.
#
# Falls back gracefully when Python3 is unavailable.
HASHCC_PY := $(CURDIR)/tools/hashcc.py
_PYTHON3  := $(shell command -v python3 2>/dev/null)
SMARTCC    = $(if $(_PYTHON3),python3 $(HASHCC_PY),)
_PRESCAN   = $(if $(_PYTHON3),python3 $(HASHCC_PY) --prescan --machine $(MACHINE),)

# Machine stamp: make keys on file timestamps, not the MACHINE value, so
# switching targets (e.g. lc3 -> se) would otherwise reuse stale objects built
# with the wrong -DPLUSOS_MACHINE_*. This stamp's name encodes MACHINE; when it
# changes, the old stamp is removed and the new one is newer than every
# machine-specific object, forcing the loader and kernel to rebuild.
MACHINE_STAMP := $(BUILD)/.machine.$(MACHINE)
$(MACHINE_STAMP):
	@mkdir -p $(BUILD)
	@rm -f $(BUILD)/.machine.* 2>/dev/null || true
	@touch $@

# ---- kernel module selection -------------------------------------------------
# The kernel is built from modules (see kernel/core/kmodule.h, kconfig.h). Pick
# which optional ones go in with a PROFILE preset and/or per-module switches:
#
#     make PROFILE=fast install            preset (see below)
#     make DISABLE="sound shm" install     preset + drop these modules
#     make PROFILE=tiny ENABLE=sound       preset, but keep sound
#     make modules                         show what the current config builds
#
#   full  everything (default)
#   lean  drop profiler, entropy, power           - slimmer, same features apps use
#   fast  lean + no watchdog                       - least work in the tick/switch/syscall paths
#   tiny  drop every optional module               - smallest kernel image, quickest boot
#
# Optional modules: sound profiler entropy power pram shm ramdisk scrap macloader
#                   stackguard watchdog
# A disabled module's source is not linked at all and its hooks compile away;
# its syscall just returns -1.
# Disk Studio's "Kernel Build Settings" writes kernel.cfg (make syntax: PROFILE=,
# DISABLE=, ENABLE=). Command-line values still win over the file.
-include kernel.cfg
PROFILE ?= full
DISABLE ?=
ENABLE  ?=
# module lists may be separated by spaces, commas or '+' (the last two survive build.bat)
_comma := ,
_mlist  = $(subst +, ,$(subst $(_comma), ,$(1)))
KMODS_ALL      := sound profiler entropy power pram shm ramdisk scrap macloader users perms stackguard watchdog
KMODS_OFF_full :=
KMODS_OFF_lean := profiler entropy power
KMODS_OFF_fast := profiler entropy power watchdog
KMODS_OFF_tiny := sound profiler entropy power pram shm ramdisk scrap macloader users perms watchdog
ifeq ($(origin KMODS_OFF_$(PROFILE)),undefined)
$(error Unknown PROFILE '$(PROFILE)' - use full, lean, fast or tiny)
endif
KMODS_OFF := $(sort $(filter-out $(call _mlist,$(ENABLE)),$(KMODS_OFF_$(PROFILE)) $(call _mlist,$(DISABLE))))
$(foreach m,$(KMODS_OFF) $(call _mlist,$(ENABLE)),$(if $(filter $(m),$(KMODS_ALL)),,$(error Unknown kernel module '$(m)'. Known: $(KMODS_ALL))))
KCONFIG_FLAGS := $(foreach m,$(KMODS_OFF),-DCONFIG_$(call UPPER,$(m))=0)
KMODS_ON := $(filter-out $(KMODS_OFF),$(KMODS_ALL))

# Source file of each optional module (stackguard / watchdog live in shared files)
KSRC_sound    := kernel/drivers/sound/sound.c
KSRC_profiler := kernel/services/profiler.c
KSRC_entropy  := kernel/services/entropy.c
KSRC_power    := kernel/services/power.c
KSRC_pram     := kernel/services/pram.c
KSRC_shm      := kernel/services/shm.c
KSRC_ramdisk  := kernel/services/ramdisk.c
KSRC_scrap    := kernel/services/scrap.c
KSRC_macloader:= kernel/services/macloader.c
KSRC_users    := kernel/services/users.c
KSRC_perms    := kernel/services/perms.c
KSRC_OFF      := $(foreach m,$(KMODS_OFF),$(KSRC_$(m)))

# The build ledger (tools/hashcc.py) hashes sources, not compiler flags, so it
# is told the kernel's configuration: changing it forces the kernel to rebuild.
export PLUSOS_CFG_SIG = $(KCONFIG_FLAGS) $(EXTRA_CPPFLAGS)

# Kernel sources across all subdirectories (core, cpu, proc, drivers, services)
KSRC   := $(filter-out $(KSRC_OFF),$(wildcard kernel/*.c kernel/*.S kernel/*/*.c kernel/*/*.S kernel/*/*/*.c kernel/*/*/*.S))
KOBJ   := $(patsubst %,$(BUILD)/%.o,$(KSRC))
KERNEL := $(BUILD)/kernel/kernel.pex
LOADER := $(BUILD)/boot/loader.bin

# Apps categorized by type: games, development, tools, network, system (and top-level fallback)
app_dir = $(firstword $(wildcard apps/*/$(1) apps/$(1)))
APPS    := $(sort $(notdir $(wildcard apps/*/*)))
APPPEX  := $(foreach a,$(APPS),$(BUILD)/apps/$(a).pex)

# Live network debugger (netdbg) auto-start is OFF by default for security and
# performance. Build with NETDBG=1 to compile in its boot-time auto-start:
#     make NETDBG=1 install        (or:  build.bat netdbg install)
NETDBG ?= 0
ifeq ($(NETDBG),1)
EXTRA_CPPFLAGS += -DPLUSOS_ENABLE_NETDBG
endif

# The kernel includes all kernel subdirectories and SE SDK catalog
KERNEL_CPPFLAGS := $(PLUSOS_CPPFLAGS) $(PLUSOS_MACHINE_CPPFLAGS) $(EXTRA_CPPFLAGS) $(KCONFIG_FLAGS) -Ikernel -Ikernel/core -Ikernel/cpu -Ikernel/proc \
                   -Ikernel/drivers -Ikernel/drivers/video -Ikernel/drivers/input -Ikernel/drivers/sound \
                   -Ikernel/services -idirafter $(SDK_SE)/include

.PHONY: all sdks sdklibs sdktest install clean kernel loader apps floppy studio prescan dump-cache clean-cache modules test-kmodule
# test-kmodule: host-side unit test of the module registry, driver table and watchdog
test-kmodule:
	@mkdir -p $(BUILD)
	@gcc -Wall -Wextra -DKMOD_HOST -D'KMOD_NOW()=(fake_now++)' -Ikernel/core \
	    tools/tests/test_kmodule.c kernel/core/kmodule.c -o $(BUILD)/test_kmodule && $(BUILD)/test_kmodule
# modules: show which optional kernel modules the current PROFILE/DISABLE/ENABLE builds
modules:
	@echo "kernel profile : $(PROFILE)"
	@echo "built in       : $(KMODS_ON)"
	@echo "left out       : $(if $(KMODS_OFF),$(KMODS_OFF),(none))"
	@echo "kernel sources : $(words $(KSRC)) files"
# prescan: phase 1 — scan ledger and pre-touch cached .o files before make runs
prescan:
	@if [ -n "$(_PYTHON3)" ]; then $(_PRESCAN); fi

all: prescan sdklibs $(LOADER) $(KERNEL) apps
	@echo "  kernel profile: $(PROFILE)$(if $(KMODS_OFF), (without: $(KMODS_OFF)),)"
	@$(SIZE) $(BUILD)/kernel/kernel.elf | tail -1 | awk '{print "kernel: text " $$1 "  data " $$2 "  bss " $$3}'

sdklibs:
	@$(MAKE) --no-print-directory -C $(SDK_PLUS) all
	@$(MAKE) --no-print-directory -C $(SDK_SE) all
sdktest:
	@$(MAKE) --no-print-directory -C $(SDK_PLUS) test
	@$(MAKE) --no-print-directory -C $(SDK_SE) test
kernel: prescan sdklibs $(KERNEL)
loader: $(LOADER)
# Best-effort: build each app on its own so one that fails to compile is skipped
# with a warning instead of stopping the whole build. `make strict-apps` fails hard.
apps: prescan sdklibs
	@mkdir -p $(BUILD)/apps
	@ok=; bad=; for a in $(APPS); do \
	  if $(MAKE) --no-print-directory $(BUILD)/apps/$$a.pex >/dev/null 2>$(BUILD)/apps/$$a.log; then ok="$$ok $$a"; \
	  else bad="$$bad $$a"; echo "  SKIP $$a (see $(BUILD)/apps/$$a.log)"; fi; done; \
	  echo "  apps built:$$ok"; [ -z "$$bad" ] || echo "  apps skipped:$$bad"
	@if [ -n "$(_PYTHON3)" ]; then \
	  hits=$$(find $(BUILD) -name '*.hit' 2>/dev/null | wc -l | tr -d ' '); \
	  objs=$$(find $(BUILD) -name '*.o'   2>/dev/null | wc -l | tr -d ' '); \
	  echo "  cache: $$hits/$$objs objects reused (hash unchanged)"; \
	fi
strict-apps: prescan sdklibs $(APPPEX)

# ---- header-dependency tracking: changing any .h rebuilds what includes it ---
DEPFLAGS = -MMD -MP
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

# ---- kernel: compiled from modular subfolders (core, cpu, proc, drivers, services) -
$(BUILD)/kernel/%.c.o: kernel/%.c $(MACHINE_STAMP)
	@mkdir -p $(dir $@)
	@echo "  CC   $<"; $(SMARTCC) $(CC) $(KERNEL_CPPFLAGS) $(PLUSOS_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD)/kernel/%.S.o: kernel/%.S $(MACHINE_STAMP)
	@mkdir -p $(dir $@)
	@echo "  AS   $<"; $(SMARTCC) $(CC) $(KERNEL_CPPFLAGS) $(PLUSOS_ASFLAGS) -c $< -o $@

$(KERNEL): $(KOBJ) $(PLUSOS_LIBS_DEPS) $(PLUSOS_EXTRA_LIBS) $(PLUSOS_SDK)/ld/plusos.ld
	$(call plusos_link_kernel,$@,$(KOBJ),PLUSOS)

# ---- apps: each built against the Mac Plus SDK, or the Mac SE SDK if SE file exists
define APP_RULES
$(1)_DIR := $$(call app_dir,$(1))
$(1)_SDK := $$(if $$(wildcard $$($(1)_DIR)/SE),$$(SDK_SE),$$(SDK_PLUS))
$(1)_ROM := $$(if $$(wildcard $$($(1)_DIR)/SE),serom,plusrom)
$(1)_CPP := -nostdinc -isystem $$(GCCINC) -I$$($(1)_SDK)/include/libc -I$$($(1)_SDK)/include
$(1)_LIB := $$($(1)_SDK)/build/lib
$(1)_SRC := $$(wildcard $$($(1)_DIR)/*.c $$($(1)_DIR)/*.cpp $$($(1)_DIR)/*.S)
$(1)_OBJ := $$(patsubst %,$(BUILD)/%.o,$$($(1)_SRC))

$$(BUILD)/$$($(1)_DIR)/%.c.o: $$($(1)_DIR)/%.c
	@mkdir -p $$(dir $$@)
	@echo "  CC   $$< ($$(notdir $$($(1)_SDK)))"; $(SMARTCC) $$(CC) $$($(1)_CPP) $$(PLUSOS_CFLAGS) $$(DEPFLAGS) -c $$< -o $$@

$$(BUILD)/$$($(1)_DIR)/%.cpp.o: $$($(1)_DIR)/%.cpp
	@mkdir -p $$(dir $$@)
	@echo "  CXX  $$< ($$(notdir $$($(1)_SDK)))"; $(SMARTCC) $$(CXX) $$($(1)_CPP) $$(PLUSOS_CXXFLAGS) $$(DEPFLAGS) -c $$< -o $$@

$$(BUILD)/apps/$(1).pex: $$($(1)_OBJ) sdklibs
	@mkdir -p $$(dir $$@)
	$$(LD) -T $$($(1)_SDK)/ld/plusos.ld -q --gc-sections --no-warn-rwx-segments -o $$(BUILD)/apps/$(1).elf \
	    $$($(1)_LIB)/crt0_app.o $$($(1)_OBJ) --start-group $$($(1)_LIB)/librt_app.a $$($(1)_LIB)/libplusgfx.a $$($(1)_LIB)/libplusnet.a $$($(1)_LIB)/libplusos.a \
	    $$($(1)_LIB)/lib$$($(1)_ROM).a $$($(1)_LIB)/libc.a $$(LIBGCC) --end-group
	$$(PY) $$($(1)_SDK)/tools/mkexe.py $$(BUILD)/apps/$(1).elf $$@ --name $$(call UPPER,$(1))
endef
$(foreach a,$(APPS),$(eval $(call APP_RULES,$(a))))

# ---- boot block: raw binary, at most 1024 bytes -----------------------------
$(LOADER): boot/loader.S config.mk $(MACHINE_STAMP)
	@mkdir -p $(dir $@)
	$(CC) -m68000 $(PLUSOS_MACHINE_CPPFLAGS) -DKERNEL_OFFSET="($(KERNEL_BLOCK)*512)" -DKERNEL_BYTES="($(KERNEL_BLOCKS)*512)" \
	    -c $< -o $(BUILD)/boot/loader.o
	@echo "  loader: MACHINE=$(MACHINE)$(if $(filter -DPLUSOS_MACHINE_LC3,$(PLUSOS_MACHINE_CPPFLAGS)), (LC III QuickDraw dialog), (compact-Mac minimal path))"
	$(OBJCOPY) -O binary $(BUILD)/boot/loader.o $@
	@sz=$$(stat -c %s $@); echo "loader: $$sz bytes (limit 1024)"; test $$sz -le 1024

# ---- regenerate both SDKs from the ROM sources -------------------------------
sdks:
	bash tools/sdkgen/build_roms.sh $(MACROM) $(BUILD)/roms $(CROSS)
	rm -rf $(BUILD)/split && mkdir -p $(BUILD)/split/inc $(BUILD)/split/src
	cd $(MULTIVERSAL) && ruby $(CURDIR)/tools/sdkgen/split_multiverse.rb \
	    $(CURDIR)/$(BUILD)/split/inc $(CURDIR)/$(BUILD)/split/src | grep -v '^Reading'
	$(PY) tools/sdkgen/make_sdk.py --machine plus --out sdks/macplus \
	    --rom-bin $(BUILD)/roms/rom-mac_plus_v3.bin --rom-map $(MACROM)/maps/rom-mac_plus_v3.lst.txt \
	    --split-inc $(BUILD)/split/inc --split-src $(BUILD)/split/src --defs $(MULTIVERSAL)/defs --macrom $(MACROM)
	$(PY) tools/sdkgen/make_sdk.py --machine se --out sdks/macse \
	    --rom-bin $(BUILD)/roms/rom-mac_se.bin --rom-map $(MACROM)/maps/rom-mac_se.lst.txt \
	    --split-inc $(BUILD)/split/inc --split-src $(BUILD)/split/src --defs $(MULTIVERSAL)/defs --macrom $(MACROM)

# ---- install to disk images ------------------------------------------------
install: all
	HFSUTILS=$(TOOLCHAIN)/bin $(PY) tools/install.py \
	    --image "$(IMAGE_LOCAL)" --image "$(IMAGE_REAL)" --image "$(IMAGE_TEST)" \
	    --loader $(LOADER) --kernel $(KERNEL) \
	    $(foreach a,$(APPS),$(if $(wildcard $(BUILD)/apps/$(a).pex),--app $(call UPPER,$(a))=$(BUILD)/apps/$(a).pex,)) \
	    --kernel-block $(KERNEL_BLOCK) --kernel-blocks $(KERNEL_BLOCKS) --apps-block $(APPS_BLOCK) --hfs

# ---- minimal installer floppy (.dsk) ----------------------------------------
floppy: all
	@mkdir -p $(BUILD)/boot
	@$(CC) -m68000 -DKERNEL_OFFSET=51200 -DKERNEL_BYTES=184320 -c boot/loader.S -o $(BUILD)/boot/loader_floppy.o
	@$(OBJCOPY) -O binary $(BUILD)/boot/loader_floppy.o $(BUILD)/boot/loader_floppy.bin
	@python3 tools/make_floppy.py --output images/PlusOS_Floppy.dsk

# ---- native linux disk studio & image manager -------------------------------
studio:
	@echo "Compiling native Linux PlusOS Disk Studio..."
	@g++ -O2 -std=c++17 tools/disk_studio/studio_linux.cpp tools/disk_studio/disk_image.cpp tools/disk_studio/hfs_filesystem.cpp -o PlusOSDiskStudio_linux
	@chmod +x PlusOSDiskStudio_linux
	@echo "Ready: ./PlusOSDiskStudio_linux"

clean:
	rm -rf $(BUILD)
	-@$(MAKE) --no-print-directory -C $(SDK_PLUS) clean
	-@$(MAKE) --no-print-directory -C $(SDK_SE) clean

# Remove hash-cache sidecars and central ledger (forces full recompile next build)
clean-cache:
	@find $(BUILD) \( -name '*.hashcc' -o -name '*.hit' \) -delete 2>/dev/null || true
	@rm -f $(BUILD)/.build_ledger.json 2>/dev/null || true
	@echo "  build cache cleared (next build will recompile all objects)"

# Print the full build ledger as a human-readable document
dump-cache:
	@if [ -n "$(_PYTHON3)" ]; then \
	  python3 $(HASHCC_PY) --dump --machine $(MACHINE); \
	else echo "dump-cache requires Python3"; fi
