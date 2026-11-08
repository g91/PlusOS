MEMORY
{
    rom (rx) : ORIGIN = 0x00400000, LENGTH = 256K
}

ENTRY(_reset_entry)

SECTIONS
{
    .text 0x00400000 : {
        KEEP(*rom_start.o(.text))
        *(.text*)
        *(.rodata*)
    } > rom

    .data : {
        *(.data*)
    } > rom

    .bss : {
        *(.bss*)
        *(COMMON)
    } > rom

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
