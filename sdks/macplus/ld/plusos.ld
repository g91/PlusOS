/* plusos.ld - linker script for PlusOS executables (kernel and apps).
 * Everything is linked at address 0; tools/mkexe.py turns the --emit-relocs
 * output into a relocation table so the image runs at any address. */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

SECTIONS
{
    . = 0;
    .text : {
        __image_start = .;
        KEEP(*(.text.crt0))                 /* header + entry: must be first */
        *(.text .text.* .gnu.linkonce.t.*)
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(.preinit_array))
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        __init_array_end = .;
        __fini_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        __fini_array_end = .;
    }
    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(4);
    }
    __image_end = .;                        /* relocation table goes here */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.* .gnu.linkonce.b.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    }
    /DISCARD/ : {
        *(.comment) *(.note .note.*) *(.eh_frame) *(.eh_frame_hdr)
        *(.gcc_except_table*) *(.jcr) *(.got) *(.got.plt) *(.igot*)
    }
}
