OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 1M;
    stext = .;
    .text : { *(.init) *(.text) }
    . = ALIGN(4K);
    .data : { *(.data) }
    . = ALIGN(16);
    .bss : { *(.bss) }
    . = ALIGN(4K);
    edata = .;
    end_of_memory = 128M;
}

