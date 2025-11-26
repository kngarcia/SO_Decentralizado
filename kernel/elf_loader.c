/* kernel/elf_loader.c
 * ELF binary loader implementation
 * Parses ELF64 headers and loads LOAD segments into memory
 */

#include "elf_loader.h"
#include "drivers/serial.h"
#include "mm/virtual_memory.h"

/* Forward declare kernel functions */
extern void *kmalloc(unsigned int size);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);

/* Validate ELF header */
int elf_validate(const elf64_hdr_t *hdr) {
    if (!hdr) return 0;
    
    /* Check magic number */
    if (hdr->e_ident[EI_MAG0] != ELFMAG0) return 0;
    if (hdr->e_ident[EI_MAG1] != ELFMAG1) return 0;
    if (hdr->e_ident[EI_MAG2] != ELFMAG2) return 0;
    if (hdr->e_ident[EI_MAG3] != ELFMAG3) return 0;
    
    /* Check class (64-bit) */
    if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
        serial_puts("[elf] Error: not 64-bit ELF\n");
        return 0;
    }
    
    /* Check endianness (little-endian) */
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        serial_puts("[elf] Error: not little-endian\n");
        return 0;
    }
    
    /* Check type (ET_EXEC for position-independent or ET_DYN for PIE) */
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
        serial_puts("[elf] Error: unsupported ELF type\n");
        return 0;
    }
    
    return 1;
}

/* Simple process ID allocator */
static uint64_t next_pid = 1000;

/* Load ELF binary into memory */
process_t *elf_load(const uint8_t *binary_data, size_t size) {
    if (size < sizeof(elf64_hdr_t)) {
        serial_puts("[elf] Error: binary too small\n");
        return NULL;
    }
    
    const elf64_hdr_t *elf_hdr = (const elf64_hdr_t *)binary_data;
    
    if (!elf_validate(elf_hdr)) {
        serial_puts("[elf] Invalid ELF header\n");
        return NULL;
    }
    
    serial_puts("[elf] Valid ELF header found\n");
    
    /* Allocate process control block */
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (!proc) return NULL;
    
    memset(proc, 0, sizeof(process_t));
    proc->pid = next_pid++;
    proc->entry_point = elf_hdr->e_entry;
    proc->state = 0; /* new */
    
    /* Parse program headers and load LOAD segments */
    uint64_t ph_offset = elf_hdr->e_phoff;
    uint16_t ph_num = elf_hdr->e_phnum;
    uint16_t ph_size = elf_hdr->e_phentsize;
    
    serial_puts("[elf] Loading segments...\n");
    
    for (int i = 0; i < ph_num; i++) {
        const elf64_phdr_t *phdr = (const elf64_phdr_t *)(binary_data + ph_offset + i * ph_size);
        
        if (phdr->p_type == PT_LOAD) {
            uint64_t vaddr = phdr->p_vaddr;
            uint64_t memsz = phdr->p_memsz;
            uint64_t filesz = phdr->p_filesz;
            uint64_t offset = phdr->p_offset;
            
            serial_puts("[elf] Segment: vaddr=0x");
            serial_put_hex(vaddr);
            serial_puts(" memsz=0x");
            serial_put_hex(memsz);
            serial_putc('\n');
            
            /* Copy segment data */
            if (offset + filesz <= size) {
                memcpy((void *)vaddr, binary_data + offset, filesz);
                
                /* Zero-fill BSS (memsz > filesz) */
                if (memsz > filesz) {
                    memset((void *)(vaddr + filesz), 0, memsz - filesz);
                }
            } else {
                serial_puts("[elf] Error: segment exceeds binary\n");
                /* (no-op for bump allocator, proc stays allocated) */
                return NULL;
            }
        }
    }
    
    /* Setup initial heap */
    proc->heap_start = 0x10000000;  /* Arbitrary location, post-binary */
    proc->heap_end = proc->heap_start + 0x1000000;
    
    /* Setup initial stack */
    proc->stack_top = 0x20000000 - 0x1000;  /* Arbitrary top, growing down */
    
    serial_puts("[elf] Process loaded: pid=");
    serial_put_hex(proc->pid);
    serial_puts(" entry=0x");
    serial_put_hex(proc->entry_point);
    serial_putc('\n');
    
    return proc;
}

/* Execute process by jumping to entry point in ring-3
 * TODO: Setup ring-3 context (CS/SS selectors), initial stack frame
 */
int elf_exec(process_t *proc, char **argv, char **envp) {
    if (!proc) return -1;
    
    serial_puts("[elf] Executing process (TODO: jump to ring-3)\n");
    
    /* For Phase 1 we'll perform a minimal ring-3 transition using identity
     * mapped memory: we assume user code and user stack are mapped in the
     * current address space (this is true for the current identity mapping).
     *
     * Steps:
     * 1) Allocate a user stack region (virtual_memory_alloc)
     * 2) Align the stack and prepare initial register frame if needed
     * 3) Push SS, RSP, RFLAGS, CS, RIP and execute iretq to drop to ring-3
     *
     * NOTE: This is a minimal approach for Phase 1. A full implementation
     * should create per-process page tables and TSS.
     */

    /* Allocate user stack (64 KB) */
    uint64_t stack_size = 64 * 1024;
    uint64_t user_stack = (uint64_t)virtual_memory_alloc(0, stack_size, PROT_READ | PROT_WRITE);
    if (!user_stack) {
        serial_puts("[elf] Error: cannot allocate user stack\n");
        return -1;
    }

    /* Set stack top (grow down) and align to 16 bytes */
    uint64_t user_sp = (user_stack + stack_size) & ~0xFULL;
    proc->stack_top = user_sp;

    /* User selectors (as configured in gdt.c): */
    const uint64_t USER_CS = 0x1B; /* selector for user code (index 3 << 3 | RPL=3) */
    const uint64_t USER_SS = 0x23; /* selector for user data (index 4 << 3 | RPL=3) */

    /* Make sure interrupts are disabled while preparing iret frame */
    asm volatile("cli");

    /* Push iret frame: SS, RSP, RFLAGS, CS, RIP (push in this order so RIP is on top)
     * Sequence of pushes: push SS; push RSP; pushfq; push CS; push RIP; iretq
     */
    uint64_t rip = proc->entry_point;

    asm volatile(
        "pushq %%rax\n\t"              /* scratch: push old rax to preserve */
        : : : "memory"
    );

    asm volatile(
        "pushq %0\n\t"                /* push USER_SS */
        "pushq %1\n\t"                /* push user_sp (RSP) */
        "pushfq\n\t"                   /* push RFLAGS */
        "pushq %2\n\t"                /* push USER_CS */
        "pushq %3\n\t"                /* push RIP */
        "iretq\n\t"
        :
        : "r"(USER_SS), "r"(user_sp), "r"(USER_CS), "r"(rip)
        : "memory"
    );

    /* Should never return here; if it does, mark process as dead */
    proc->state = 3; /* dead */
    return 0;
}

/* Free process resources */
void elf_free_process(process_t *proc) {
    if (!proc) return;
    
    /* TODO: Free page tables, heap, stack */
    /* (no-op for bump allocator) */
}

