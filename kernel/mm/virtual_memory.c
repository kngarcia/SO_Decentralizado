/* kernel/mm/virtual_memory.c - stubs for virtual memory interface */
#include <stdint.h>
#include "virtual_memory.h"
#include <string.h>
#include "../drivers/serial.h"
#include "../process_manager.h"
#ifdef HOST_TEST
#include <stdlib.h>
#endif

/* Simple region tracking + bump allocator for Phase 1
 * Supports a minimal host-backed copy-on-write emulation when HOST_TEST
 * is enabled. Regions are represented with a small array of metadata
 * so tests can share/clone regions and validate refcounts.
 */
static uint64_t heap_ptr = 0x10000000;

typedef struct vm_region {
    uint64_t vaddr;
    uint64_t size;
    int prot;
    void *host_ptr;   /* only valid in HOST_TEST; NULL in kernel builds */
    int refcount;
} vm_region_t;

static vm_region_t regions[64];
static int region_count = 0;

static vm_region_t *find_region(uint64_t vaddr) {
    for (int i = 0; i < region_count; ++i) {
        if (regions[i].vaddr == vaddr) return &regions[i];
    }
    return NULL;
}

void *virtual_memory_alloc(uint64_t vaddr, uint64_t size, int prot) {
    if (vaddr == 0) {
        /* Auto-allocate at bump heap (virtual address returned is symbolic)
           For HOST_TEST, also allocate host memory so tests may memcpy to it. */
        uint64_t addr = heap_ptr;
        heap_ptr += (size + 0xFFF) & ~0xFFF;  /* align to 4K */

        if (region_count < (int)(sizeof(regions)/sizeof(regions[0]))) {
            regions[region_count].vaddr = addr;
            regions[region_count].size = size;
            regions[region_count].prot = prot;
#ifdef HOST_TEST
            regions[region_count].host_ptr = malloc((size_t)size);
            regions[region_count].refcount = regions[region_count].host_ptr ? 1 : 0;
#else
            regions[region_count].host_ptr = NULL;
            regions[region_count].refcount = 1;
#endif
            ++region_count;
        }

        return (void *)addr;
    } else {
        /* Fixed-address allocation: record region for COW tracking */
        if (region_count < (int)(sizeof(regions)/sizeof(regions[0]))) {
            regions[region_count].vaddr = vaddr;
            regions[region_count].size = size;
            regions[region_count].prot = prot;
#ifdef HOST_TEST
            regions[region_count].host_ptr = malloc((size_t)size);
            regions[region_count].refcount = regions[region_count].host_ptr ? 1 : 0;
#else
            regions[region_count].host_ptr = NULL;
            regions[region_count].refcount = 1;
#endif
            ++region_count;
        }
        return (void *)vaddr;
    }
}

void virtual_memory_free(uint64_t vaddr, uint64_t size) {
    (void)vaddr;
    (void)size;
    /* TODO: implement heap defragmentation */
}

int virtual_memory_map(uint64_t vaddr, uint64_t paddr, int prot) {
    (void)vaddr;
    (void)paddr;
    (void)prot;
    /* TODO: update page tables */
    return 0;
}

/* Mark an existing region as shared (increase refcount). If the exact
   region is not found, do nothing. Symbolic in kernel mode; host tests
   will increment refcount and share underlying allocation.
 */
void virtual_memory_share(uint64_t vaddr, uint64_t size) {
    vm_region_t *r = find_region(vaddr);
    if (!r) return;
    /* only if sizes match or requested size fits inside region */
    if (r->size >= size) {
        r->refcount++;
    }
}

/* Ensure region containing vaddr is writable by obtaining a private copy
   if refcount>1. Returns host pointer to the writable memory (HOST_TEST)
   or NULL if not applicable. */
void *virtual_memory_make_writable(uint64_t vaddr, uint64_t size) {
    vm_region_t *r = NULL;
    for (int i = 0; i < region_count; ++i) {
        if (vaddr >= regions[i].vaddr && vaddr < regions[i].vaddr + regions[i].size) {
            r = &regions[i];
            break;
        }
    }
    if (!r) return NULL;

    if (r->refcount <= 1) {
        /* already private */
        return r->host_ptr;
    }

#ifdef HOST_TEST
    /* allocate new host buffer and copy contents */
    void *newbuf = malloc((size_t)r->size);
    if (!newbuf) return NULL;
    memcpy(newbuf, r->host_ptr, (size_t)r->size);
    r->refcount--; /* existing region loses one owner */

    /* create new region entry for the private copy at same vaddr */
    if (region_count < (int)(sizeof(regions)/sizeof(regions[0]))) {
        regions[region_count].vaddr = r->vaddr;
        regions[region_count].size = r->size;
        regions[region_count].prot = r->prot;
        regions[region_count].host_ptr = newbuf;
        regions[region_count].refcount = 1;
        ++region_count;
        return newbuf;
    } else {
        /* fallback: make existing region private */
        r->host_ptr = newbuf;
        r->refcount = 1;
        return newbuf;
    }
#else
    /* On kernel builds we don't have host-backed storage; return NULL */
    return NULL;
#endif
}

int virtual_memory_refcount(uint64_t vaddr) {
    vm_region_t *r = find_region(vaddr);
    if (!r) return 0;
    return r->refcount;
}

void general_protection_handler(uint64_t *saved_regs_ptr) {
    serial_puts("[GP] General Protection Fault!\n");
    serial_puts("[GP] saved_regs_ptr=0x"); serial_put_hex((uint64_t)saved_regs_ptr); serial_putc('\n');
    
    /* The error code is pushed before the interrupt frame by the CPU */
    /* Stack layout: ... regs ... error_code RIP CS RFLAGS RSP SS */
    /* saved_regs_ptr points to the first pushed register (R15 in our case) */
    /* Error code is at saved_regs_ptr[15] (after 15 pushed registers) */
    uint64_t error_code = saved_regs_ptr[15];
    serial_puts("[GP] error_code=0x"); serial_put_hex(error_code); serial_putc('\n');
    
    /* Dump stack */
    serial_puts("[GP] stack dump:\n");
    for (int i = 0; i < 25; i++) {
        serial_put_hex(saved_regs_ptr[i]);
        serial_putc(' ');
        if ((i+1) % 5 == 0) serial_putc('\n');
    }
    
    serial_puts("[GP] Halting...\n");
    for(;;) asm volatile("hlt");
}

uint64_t page_fault_handler(uint64_t *saved_regs_ptr, uint64_t fault_addr) {
    (void)saved_regs_ptr;
    serial_puts("[pf] page fault at 0x");
    serial_put_hex((uint64_t)fault_addr);
    serial_putc('\n');

#ifdef HOST_TEST
    void *w = virtual_memory_make_writable(fault_addr, 4096);
    if (w) {
        serial_puts("[pf] resolved COW in host-mode\n");
        return (uint64_t)saved_regs_ptr; /* continue execution */
    }
#else
    /* Kernel-mode: attempt PTE-level COW
       1) Find PTE for faulting address
       2) If PTE present, not writable and frame_refcount > 1 -> allocate new frame
       3) Copy page contents and update PTE to new frame with writable bit
    */
    extern uint64_t *pt_find_pte_for_vaddr(void *pml4_base, uint64_t vaddr);
    extern void *pt_get_kernel_pml4(void);
    extern uint32_t alloc_frame(void);
    extern void frame_decref(uint32_t addr);
    extern int frame_refcount_get(uint32_t addr);

    /* Read current CR3 to obtain active PML4 (value is physical, identity mapped) */
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r" (cr3));
    uint64_t *pte_ptr = pt_find_pte_for_vaddr((void *)cr3, fault_addr);
    if (!pte_ptr) {
        serial_puts("[pf] no page-table entry -> killing\n");
        /* kill process */
        extern process_t *pm_get_current(void);
        process_t *cur = pm_get_current();
        if (cur) { cur->state = 3; cur->exit_code = -1; }
        for(;;) asm volatile("hlt");
    }

    uint64_t pte = *pte_ptr;
    /* debug: print pte contents to help diagnose faults */
    /* try to read CPU-provided error-code (pushed before our register pushes)
       we pushed 15 registers in the ISR wrapper, so error code should be
       located at saved_regs_ptr[15] */
    uint64_t errcode = 0;
    if (saved_regs_ptr) {
        /* Dump stack words around saved_regs_ptr (helpful to locate CPU-pushed RIP/CS/RFLAGS/ERRCODE)
           saved_regs_ptr points to the current RSP at C handler entry (after our pushes).
           We'll print a small window of words before and after this pointer. */
        serial_puts("[pf] saved_regs_ptr=0x"); serial_put_hex((uint64_t)(uintptr_t)saved_regs_ptr); serial_putc('\n');
        serial_puts("[pf] stack dump (words @ saved_regs_ptr-4 .. +20):\n");
        for (int i = -4; i < 21; ++i) {
            uint64_t val = 0;
            /* Avoid invalid pointer arithmetic in some builds */
            val = saved_regs_ptr[i];
            serial_put_hex((uint64_t)val);
            serial_putc(' ');
            if ((i + 5) % 8 == 0) serial_putc('\n');
        }
        serial_putc('\n');

        /* Try to find an error code nearby (scan local window for small values that look like error codes) */
        for (int i = -4; i < 21; ++i) {
            uint64_t v = saved_regs_ptr[i];
            if (v <= 0xFFF) { /* likely an errcode */
                errcode = v;
                serial_puts("[pf] guessed errcode=0x"); serial_put_hex(errcode); serial_putc('\n');
                break;
            }
        }
    }
    serial_puts("[pf] pte=0x"); serial_put_hex(pte); serial_putc('\n');
    if (!(pte & 1)) {
        serial_puts("[pf] pte not present -> killing\n");
        extern process_t *pm_get_current(void);
        process_t *cur = pm_get_current();
        if (cur) { cur->state = 3; cur->exit_code = -1; }
        for(;;) asm volatile("hlt");
    }

    /* If U/S (user) bit not set, this may be a user-mode access - set and continue */
    if (!(pte & 0x4)) {
        serial_puts("[pf] PTE missing U/S bit -> setting user bit and continuing\n");
        *pte_ptr = pte | 0x4ULL; /* mark as user-accessible */
        /* invalidate TLB for this address so the CPU sees the updated PTE */
        asm volatile("invlpg (%0)" :: "r"(fault_addr) : "memory");
        return (uint64_t)saved_regs_ptr;
    }

    /* if writable - not a write fault COW, escalate */
    if (pte & 0x2) {
        serial_puts("[pf] page already writable -> escalate\n");
        extern process_t *pm_get_current(void);
        process_t *cur = pm_get_current();
        if (cur) { cur->state = 3; cur->exit_code = -1; }
        for(;;) asm volatile("hlt");
    }

    uint32_t frame = (uint32_t)(pte & ~0xFFFULL);
    int refc = frame_refcount_get(frame);
    if (refc <= 1) {
        serial_puts("[pf] frame refcount<=1 -> make writable and continue\n");
        *pte_ptr = pte | 0x2; /* make writable */
        asm volatile("invlpg (%0)" :: "r"(fault_addr) : "memory");
        return (uint64_t)saved_regs_ptr;
    }

    /* allocate new frame and copy page contents */
    uint32_t newframe = alloc_frame();
    if (!newframe) { serial_puts("[pf] alloc_frame failed\n");
        /* kill process */
        extern process_t *pm_get_current(void);
        process_t *cur = pm_get_current();
        if (cur) {
            cur->state = 3;
            cur->exit_code = -1;
        }
        for(;;) asm volatile("hlt");
    }
    uint64_t page_base = fault_addr & ~0xFFFULL;
    /* identity mapping guarantees page_base can be used as pointer */
    memcpy((void *)(uintptr_t)newframe, (const void *)page_base, 4096);

    /* Update PTE to new frame and set writable */
    uint64_t newpte = (uint64_t)newframe | (pte & 0xFFFULL) | 0x2ULL; /* keep flags and set writable */
    *pte_ptr = newpte;

    /* reduce old frame refcount */
    frame_decref(frame);

    serial_puts("[pf] performed kernel COW, continuing\n");
    return (uint64_t)saved_regs_ptr;
#endif

    /* Could not handle page fault — terminate current process if any */
    extern process_t *pm_get_current(void);
    process_t *cur = pm_get_current();
    if (cur) {
        serial_puts("[pf] killing process pid=");
        serial_put_hex(cur->pid);
        serial_putc('\n');
        cur->state = 3; /* dead */
        cur->exit_code = -1;
    }

    /* Stop here — in kernel mode we don't attempt recovery yet */
    for(;;) asm volatile("hlt");
}

/* Legacy kmalloc stub */
void *kmalloc(unsigned int size) {
#ifdef HOST_TEST
    /* When running host-side unit tests, fall back to malloc so pointers
       are valid in the host process address space. */
    return malloc(size);
#else
    static uint64_t heap = 0x01000000; /* 16MB start */
    void *ptr = (void *)heap;
    heap += size;
    return ptr;
#endif
}

