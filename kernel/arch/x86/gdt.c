/* kernel/arch/x86/gdt.c - minimal GDT setup (x86-64 stub) */
#include <stdint.h>

/* In long mode (x86-64), GDT is still used but simplified.
   Most fields are ignored. We keep this minimal for compatibility.
*/

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;  /* 64-bit base address in long mode */
} __attribute__((packed));

extern void gdt_flush(uint64_t);

/* Expand GDT size to accommodate TSS descriptor (uses two slots) */
static struct gdt_entry gdt[7];
static struct gdt_ptr gp;

/* 64-bit TSS structure (minimal: RSP0 only) */
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

static void gdt_set_gate(int num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_install(void) {
    gp.limit = (sizeof(struct gdt_entry) * 7) - 1;
    gp.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                /* Null segment */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF); /* Kernel code segment */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xAF); /* Kernel data segment */
    /* User segments (DPL=3) */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF); /* User code (RPL=3) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xAF); /* User data (RPL=3) */

    /* Prepare a minimal TSS and install its descriptor in GDT (entries 5 & 6) */
    static struct tss_entry tss = {0};
    /* Small kernel interrupt stack for privilege transitions (RSP0) */
    static uint8_t kernel_stack[8192];
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(struct tss_entry) - 1;

    /* Initialize RSP0 to top of kernel_stack */
    tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));
    tss.iomap_base = 0;

    /* Encode TSS descriptor (first 8 bytes) at gdt[5] */
    gdt[5].limit_low = (tss_limit & 0xFFFF);
    gdt[5].base_low = (tss_base & 0xFFFF);
    gdt[5].base_middle = (tss_base >> 16) & 0xFF;
    gdt[5].access = 0x89; /* present, DPL=0, type=9 (available 64-bit TSS) */
    gdt[5].granularity = (tss_limit >> 16) & 0x0F;
    gdt[5].granularity |= 0x00;
    gdt[5].base_high = (tss_base >> 24) & 0xFF;

    /* Second 8 bytes (store high base) in gdt[6] */
    uint64_t *slot = (uint64_t *)&gdt[6];
    *slot = (tss_base >> 32) & 0xFFFFFFFFULL;

    /* Load GDT and then load TR to activate TSS */
    gdt_flush((uint64_t)&gp);

    /* Load Task Register (selector = index 5 << 3) */
    uint16_t tss_selector = (5 << 3);
    asm volatile ("ltr %0" :: "r" (tss_selector));
}

