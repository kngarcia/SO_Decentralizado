/* kernel/linker.ld - x86-64 (64-bit) kernel linker script
   Booteable on any UEFI/BIOS system (VM, physical hardware)
   GRUB loads as 32-bit, we transition to 64-bit in start.S
*/
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start32)

SECTIONS
{
  . = 0x00100000; /* load at 1MB (GRUB multiboot standard) */

  .multiboot_header : { *(.multiboot_header) }

  .text : {
    *(.text*)
  }

  .rodata : { *(.rodata*) }

  .data : {
    *(.data*)
  }

  .paging : {
    *(.paging)
  }

  __bss_start = .;
  .bss : {
    *(.bss*)
  }
  __bss_end = .;

  .linker_symbols : {
    *(.linker_symbols)
  }

  /DISCARD/ : { *(.note.GNU-stack) }
}
