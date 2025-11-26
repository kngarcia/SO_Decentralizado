/* kernel/arch/x86/paging.c - minimal identity paging (x86-64) */
#include <stdint.h>

#define PAGE_ALIGN __attribute__((aligned(4096)))

/* Note: 64-bit paging structures are already set up in start.S
   This function is kept for compatibility but mostly a no-op now.
   Real 64-bit paging was done in _start32/_start64 transition.
*/

void paging_enable(void) {
    /* In 64-bit mode, paging is already enabled by start.S
     * (PML4 -> PDPT -> PD with 2MB pages)
     * This is just a safe no-op for kernel initialization flow.
     */
    asm volatile(
        "mov %%cr0, %%rax \n\t"
        "orl $0x80000000, %%eax \n\t"
        "mov %%rax, %%cr0"
        : : : "rax"
    );
}

