/* kernel/arch/x86/idt.c - minimal IDT install (x86-64 stub) */
#include <stdint.h>

#define NUM_INTERRUPTS 256

/* x86-64 IDT entry (16 bytes) */
struct idt_entry {
    uint16_t base_lo;      /* base address (low 16 bits) */
    uint16_t sel;          /* code segment selector */
    uint8_t ist;           /* IST (Interrupt Stack Table) index */
    uint8_t flags;         /* type and attributes */
    uint16_t base_mid;     /* base address (middle 16 bits) */
    uint32_t base_hi;      /* base address (high 32 bits) */
    uint32_t zero;         /* reserved */
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;         /* 64-bit base in long mode */
} __attribute__((packed));

static struct idt_entry idt[NUM_INTERRUPTS];
static struct idt_ptr idtp;

extern void idt_flush(uint64_t);

/* Set an IDT entry for x86-64
 * num: vector number
 * base: handler address
 * sel: code selector (e.g. 0x08)
 * flags: type and attributes (0x8E = interrupt gate, present)
 * ist: interrupt stack table index (0 = none)
 */
void idt_set_gate(unsigned char num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].sel = sel;
    idt[num].ist = ist & 0x7;
    idt[num].flags = flags;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_hi = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void idt_install(void) {
    idtp.limit = (sizeof(struct idt_entry) * NUM_INTERRUPTS) - 1;
    idtp.base = (uint64_t)&idt;

    /* IDT entries are left as zeros (stub handlers)
     * Real interrupt handlers would be set up here
     */

    /* Register syscall software interrupt (vector 0x80)
       isr_0x80 is implemented in assembly (interrupts.S)
    */
    extern void isr_0x80(void);
    /* Make syscall gate DPL=3 so user-mode can invoke int 0x80 */
    idt_set_gate(0x80, (uint64_t)isr_0x80, 0x08, 0xEE, 0);

    /* Register timer IRQ0 (mapped at vector 0x20 after PIC remap) */
    extern void isr_0x20(void);
    idt_set_gate(0x20, (uint64_t)isr_0x20, 0x08, 0x8E, 0);

    /* Register page fault handler (vector 0x0E) */
    extern void isr_0x0e(void);
    idt_set_gate(0x0e, (uint64_t)isr_0x0e, 0x08, 0x8E, 0);

    idt_flush((uint64_t)&idtp);
}

