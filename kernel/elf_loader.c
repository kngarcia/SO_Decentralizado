/* kernel/elf_loader.c
 * ELF binary loader implementation
 * Parses ELF64 headers and loads LOAD segments into memory
 */

#include "elf_loader.h"
#include "mm/pagetable.h"
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
/* next_pid no longer used; processes are registered via process_manager */

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
    proc->pid = 0; /* will be set by process manager on registration */
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
    
    /* register process with process manager (takes ownership) */
    extern uint64_t pm_register_process(process_t *proc);
    /* Create a per-process page table by shallow clone */
    extern void *pt_clone_current(void);
    void *pml4 = pt_clone_current();
    proc->page_table = (uint64_t *)pml4;

    pm_register_process(proc);

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
    (void)argv; (void)envp; /* unused */
    if (!proc) return -1;
    
    serial_puts("[elf_exec] Starting user process exec\n");
    serial_puts("[elf_exec] Entry point: 0x");
    serial_put_hex(proc->entry_point);
    serial_putc('\n');

    /* IMPORTANT: For Phase 1, we use a simpler approach:
     * - Keep using kernel's identity-mapped page table (no CR3 switch)
     * - The ELF segments are already loaded in kernel memory (identity mapped)
     * - We just need to mark the pages as user-accessible
     * 
     * This avoids the complexity of copying code to a new address space.
     */

    /* Mark the code region as user-accessible in the CURRENT (kernel) page table */
    extern void *pt_get_kernel_pml4(void);
    extern void pt_mark_user_recursive(void *pml4_base, uint64_t vaddr);
    void *kernel_pml4 = pt_get_kernel_pml4();
    
    /* Mark pages around entry point as user-accessible (2MB range) */
    uint64_t code_base = proc->entry_point & ~0x1FFFFFULL; /* align to 2MB */
    for (uint64_t addr = code_base; addr < code_base + 0x200000; addr += 4096) {
        pt_mark_user_recursive(kernel_pml4, addr);
    }
    serial_puts("[elf_exec] Marked code pages as user-accessible\n");
    
    /* Flush TLB to ensure page table updates take effect */
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
    
    /* DEBUG: Verify ALL levels of page table hierarchy have USER bit */
    uint64_t *pml4 = (uint64_t *)kernel_pml4;
    uint64_t vaddr = proc->entry_point;
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FF;
    
    serial_puts("[elf_exec] Page table hierarchy check for 0x");
    serial_put_hex(vaddr);
    serial_putc('\n');
    
    uint64_t pml4e = pml4[pml4_idx];
    serial_puts("  PML4E[");
    serial_put_hex(pml4_idx);
    serial_puts("]: 0x");
    serial_put_hex(pml4e);
    if (pml4e & 0x4) {
        serial_puts(" (USER)\n");
    } else {
        serial_puts(" (NO USER BIT!)\n");
    }
    
    uint64_t *pdpt = (uint64_t *)(pml4e & ~0xFFFULL);
    uint64_t pdpte = pdpt[pdpt_idx];
    serial_puts("  PDPTE[");
    serial_put_hex(pdpt_idx);
    serial_puts("]: 0x");
    serial_put_hex(pdpte);
    if (pdpte & 0x4) {
        serial_puts(" (USER)\n");
    } else {
        serial_puts(" (NO USER BIT!)\n");
    }
    
    uint64_t *pd = (uint64_t *)(pdpte & ~0xFFFULL);
    uint64_t pde = pd[pd_idx];
    serial_puts("  PDE[");
    serial_put_hex(pd_idx);
    serial_puts("]: 0x");
    serial_put_hex(pde);
    if (pde & 0x4) {
        serial_puts(" (USER)\n");
    } else {
        serial_puts(" (NO USER BIT!)\n");
    }
    
    /* Old PTE check kept for compatibility */
    extern uint64_t *pt_find_pte_for_vaddr(void *pml4_base, uint64_t vaddr);
    uint64_t *pte = pt_find_pte_for_vaddr(kernel_pml4, proc->entry_point);
    if (pte) {
        serial_puts("[elf_exec] Final PTE/PDE: 0x");
        serial_put_hex(*pte);
        if (*pte & 0x4) {
            serial_puts(" (USER bit set)\n");
        } else {
            serial_puts(" (WARNING: USER bit NOT set!)\n");
        }
    } else {
        serial_puts("[elf_exec] WARNING: No PTE found for entry point!\n");
    }

    /* Allocate user stack (64 KB) using kmalloc (gives identity-mapped physical memory) */
    uint64_t stack_size = 64 * 1024;
    uint64_t user_stack = (uint64_t)kmalloc((unsigned int)stack_size);
    if (!user_stack) {
        serial_puts("[elf_exec] Error: cannot allocate user stack\n");
        return -1;
    }
    
    /* Ensure stack base is 16-byte aligned */
    user_stack = (user_stack + 15) & ~0xFULL;
    
    /* Zero the stack */
    memset((void *)user_stack, 0, stack_size);

    /* Mark stack pages as user-accessible */
    for (uint64_t addr = user_stack; addr < user_stack + stack_size; addr += 4096) {
        pt_mark_user_recursive(kernel_pml4, addr);
    }
    
    /* Flush TLB again after marking stack pages */
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    /* Set stack top (grow down) - subtract 8 to maintain 16-byte alignment after push */
    uint64_t user_sp = (user_stack + stack_size - 8) & ~0xFULL;
    proc->stack_top = user_sp;
    
    serial_puts("[elf_exec] User stack at 0x");
    serial_put_hex(user_stack);
    serial_puts(" - 0x");
    serial_put_hex(user_sp);
    serial_putc('\n');
    
    /* DEBUG: Verify stack PDE has USER bit */
    uint64_t stack_pml4_idx = (user_sp >> 39) & 0x1FFULL;
    uint64_t stack_pdpt_idx = (user_sp >> 30) & 0x1FFULL;
    uint64_t stack_pd_idx = (user_sp >> 21) & 0x1FFULL;
    uint64_t stack_pml4e = pml4[stack_pml4_idx];
    uint64_t *stack_pdpt = (uint64_t *)(stack_pml4e & ~0xFFFULL);
    uint64_t stack_pdpte = stack_pdpt[stack_pdpt_idx];
    uint64_t *stack_pd = (uint64_t *)(stack_pdpte & ~0xFFFULL);
    uint64_t stack_pde = stack_pd[stack_pd_idx];
    serial_puts("[elf_exec] Stack PDE[");
    serial_put_hex(stack_pd_idx);
    serial_puts("]: 0x");
    serial_put_hex(stack_pde);
    if (stack_pde & 0x4) {
        serial_puts(" (USER)\n");
    } else {
        serial_puts(" (NO USER BIT!)\n");
    }


    /* User selectors (as configured in gdt.c): */
    const uint64_t USER_CS = 0x1B; /* selector for user code (index 3 << 3 | RPL=3) */
    const uint64_t USER_SS = 0x23; /* selector for user data (index 4 << 3 | RPL=3) */

    /* Build iretq frame and jump to ring-3 */
    uint64_t rip = proc->entry_point;

    serial_puts("[elf_exec] About to jump to ring-3 via jump_to_ring3()\n");
    serial_puts("[elf_exec]   RIP=0x");
    serial_put_hex(rip);
    serial_puts(" RSP=0x");
    serial_put_hex(user_sp);
    serial_puts(" CS=0x");
    serial_put_hex(USER_CS);
    serial_puts(" SS=0x");
    serial_put_hex(USER_SS);
    serial_putc('\n');
    
    /* DEBUG: Dump first 16 bytes at entry point */
    serial_puts("[elf_exec] First 16 bytes at entry: ");
    uint8_t *code = (uint8_t *)rip;
    for (int i = 0; i < 16; i++) {
        uint8_t b = code[i];
        uint8_t hi = (b >> 4) & 0xF;
        uint8_t lo = b & 0xF;
        serial_putc(hi < 10 ? '0' + hi : 'a' + hi - 10);
        serial_putc(lo < 10 ? '0' + lo : 'a' + lo - 10);
        serial_putc(' ');
    }
    serial_putc('\n');
    
    /* DEBUG: Print CR0, CR3, CR4 */
    uint64_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %%rax" : "=a"(cr0));
    asm volatile("mov %%cr3, %%rax" : "=a"(cr3));
    asm volatile("mov %%cr4, %%rax" : "=a"(cr4));
    serial_puts("[elf_exec] CR0=0x");
    serial_put_hex(cr0);
    serial_puts(" CR3=0x");
    serial_put_hex(cr3);
    serial_puts(" CR4=0x");
    serial_put_hex(cr4);
    serial_putc('\n');
    
    /* CRITICAL: Disable ALL PIC interrupts before ring-3 transition */
    extern void pic_set_mask(uint8_t mask);
    pic_set_mask(0xFF); /* Mask all IRQs */
    
    /* Call assembly function to perform ring-3 transition */
    extern void jump_to_ring3(uint64_t rip, uint64_t rsp, uint64_t cs, uint64_t ss);
    jump_to_ring3(rip, user_sp, USER_CS, USER_SS);

    /* Should never return here; if it does, mark process as dead */
    serial_puts("[elf_exec] ERROR: returned from jump_to_ring3!\n");
    proc->state = 3; /* dead */
    return -1;
}

/* Free process resources */
void elf_free_process(process_t *proc) {
    if (!proc) return;
    
    /* TODO: Free page tables, heap, stack */
    /* (no-op for bump allocator) */
}

