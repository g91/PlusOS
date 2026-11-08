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

# Kernel sources across all subdirectories (core, cpu, proc, drivers, services)
KSRC   := $(wildcard kernel/*.c kernel/*.S kernel/*/*.c kernel/*/*.S kernel/*/*/*.c kernel/*/*/*.S)
KOBJ   := $(patsubst %,$(BUILD)/%.o,$(KSRC))
KERNEL := $(BUILD)/kernel/kernel.pex
LOADER := $(BUILD)/boot/loader.bin

# Apps categorized by type: games, development, tools, network, system (and top-level fallback)
app_dir = $(firstword $(wildcard apps/*/$(1) apps/$(1)))
APPS    := $(sort $(notdir $(wildcard apps/*/*)))
APPPEX  := $(foreach a,$(APPS),$(BUILD)/apps/$(a).pex)

# The kernel includes all kernel subdirectories and SE SDK catalog
KERNEL_CPPFLAGS := $(PLUSOS_CPPFLAGS) -Ikernel -Ikernel/core -Ikernel/cpu -Ikernel/proc \
                   -Ikernel/drivers -Ikernel/drivers/video -Ikernel/drivers/input -Ikernel/drivers/sound \
                   -Ikernel/services -idirafter $(SDK_SE)/include

.PHONY: all sdks sdklibs sdktest install clean kernel loader apps floppy studio
all: sdklibs $(LOADER) $(KERNEL) apps
	@$(SIZE) $(BUILD)/kernel/kernel.elf | tail -1 | awk '{print "kernel: text " $$1 "  data " $$2 "  bss " $$3}'

sdklibs:
	@$(MAKE) --no-print-directory -C $(SDK_PLUS) all
	@$(MAKE) --no-print-directory -C $(SDK_SE) all
sdktest:
	@$(MAKE) --no-print-directory -C $(SDK_PLUS) test
	@$(MAKE) --no-print-directory -C $(SDK_SE) test
kernel: sdklibs $(KERNEL)
loader: $(LOADER)
# Best-effort: build each app on its own so one that fails to compile is skipped
# with a warning instead of stopping the whole build. `make strict-apps` fails hard.
apps: sdklibs
	@mkdir -p $(BUILD)/apps
	@ok=; bad=; for a in $(APPS); do \
	  if $(MAKE) --no-print-directory $(BUILD)/apps/$$a.pex >/dev/null 2>$(BUILD)/apps/$$a.log; then ok="$$ok $$a"; \
	  else bad="$$bad $$a"; echo "  SKIP $$a (see $(BUILD)/apps/$$a.log)"; fi; done; \
	  echo "  apps built:$$ok"; [ -z "$$bad" ] || echo "  apps skipped:$$bad"
strict-apps: sdklibs $(APPPEX)

# ---- header-dependency tracking: changing any .h rebuilds what includes it ---
DEPFLAGS = -MMD -MP
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

# ---- kernel: compiled from modular subfolders (core, cpu, proc, drivers, services) -
$(BUILD)/kernel/%.c.o: kernel/%.c
	@mkdir -p $(dir $@)
	@echo "  CC   $<"; $(CC) $(KERNEL_CPPFLAGS) $(PLUSOS_CFLAGS) $(DEPFLAGS) -c $< -o $@
$(BUILD)/kernel/%.S.o: kernel/%.S
	@mkdir -p $(dir $@)
	@echo "  AS   $<"; $(CC) $(KERNEL_CPPFLAGS) $(PLUSOS_ASFLAGS) -c $< -o $@

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
	@echo "  CC   $$< ($$(notdir $$($(1)_SDK)))"; $$(CC) $$($(1)_CPP) $$(PLUSOS_CFLAGS) $$(DEPFLAGS) -c $$< -o $$@

$$(BUILD)/$$($(1)_DIR)/%.cpp.o: $$($(1)_DIR)/%.cpp
	@mkdir -p $$(dir $$@)
	@echo "  CXX  $$< ($$(notdir $$($(1)_SDK)))"; $$(CXX) $$($(1)_CPP) $$(PLUSOS_CXXFLAGS) $$(DEPFLAGS) -c $$< -o $$@

$$(BUILD)/apps/$(1).pex: $$($(1)_OBJ) sdklibs
	@mkdir -p $$(dir $$@)
	$$(LD) -T $$($(1)_SDK)/ld/plusos.ld -q --gc-sections --no-warn-rwx-segments -o $$(BUILD)/apps/$(1).elf \
	    $$($(1)_LIB)/crt0_app.o $$($(1)_OBJ) --start-group $$($(1)_LIB)/librt_app.a $$($(1)_LIB)/libplusgfx.a $$($(1)_LIB)/libplusnet.a $$($(1)_LIB)/libplusos.a \
	    $$($(1)_LIB)/lib$$($(1)_ROM).a $$($(1)_LIB)/libc.a $$(LIBGCC) --end-group
	$$(PY) $$($(1)_SDK)/tools/mkexe.py $$(BUILD)/apps/$(1).elf $$@ --name $$(call UPPER,$(1))
endef
$(foreach a,$(APPS),$(eval $(call APP_RULES,$(a))))

# ---- boot block: raw binary, at most 1024 bytes -----------------------------
$(LOADER): boot/loader.S config.mk
	@mkdir -p $(dir $@)
	$(CC) -m68000 -DKERNEL_OFFSET="($(KERNEL_BLOCK)*512)" -DKERNEL_BYTES="($(KERNEL_BLOCKS)*512)" \
	    -c $< -o $(BUILD)/boot/loader.o
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
