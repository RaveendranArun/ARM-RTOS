ENTRY(Reset_Handler)

MEMORY
{
    FLASH(rx):ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM(rwx):ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .text :
    {
        *(.isr_vector)
        *(.text)
        *(.text.*)
        *(.init)
        *(.fini)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);   /* align to word boundary */
        _etext = .;
    }> FLASH AT > FLASH /* just "> FLASH" would be sufficient */

    _la_data = LOADADDR(.data); /* load address of .data section */
    
    .data :
    {
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);   /* align to word boundary */
        _edata = .;
    }> SRAM AT > FLASH /*vma is SRAM and lma is FLASH*/

    .bss :
    {
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);   /* align to word boundary */
        _ebss = .;
        __bss_end__ = _ebss;
        . = ALIGN(4);   /* align to word boundary */
        end = .;
        __end__ = end;
    }> SRAM


}