/* kernel/process_manager.c - phase1 process registry (simple) */
#include "process_manager.h"
#include "drivers/serial.h"
#include <string.h>

static process_t *pm_proc_table[PM_MAX_PROCS];
static int proc_cnt = 0;
static process_t *current = NULL;

uint64_t pm_register_process(process_t *proc) {
    if (!proc) return 0;
    if (proc_cnt >= PM_MAX_PROCS) return 0;
    pm_proc_table[proc_cnt] = proc;
    proc->pid = (uint64_t)(1000 + proc_cnt);
     /* Assign default page table (kernel PML4) only when not already assigned
         (elf_load or callers may provide a per-process page table before
         registering the process). */
     extern void *pt_get_kernel_pml4(void);
     if (!proc->page_table) proc->page_table = pt_get_kernel_pml4();
    proc_cnt++;
    /* Initialize FDs to -1 to indicate unused */
    for (int i = 0; i < 16; ++i) proc->fds[i] = -1;
    if (!current) current = proc;
    serial_puts("[pm] registered process pid=");
    serial_put_hex(proc->pid);
    serial_putc('\n');
    return proc->pid;
}

process_t *pm_clone_process(process_t *parent) {
    if (!parent) return NULL;
    if (proc_cnt >= PM_MAX_PROCS) return NULL;
    /* allocate via kmalloc (legacy stub) */
    extern void *kmalloc(unsigned int);
    process_t *child = (process_t *)kmalloc(sizeof(process_t));
    if (!child) return NULL;
    memcpy(child, parent, sizeof(process_t));
    /* assign new pid and set state to new */
    child->pid = (uint64_t)(1000 + proc_cnt);
    /* Clone page table for child so it has its own PML4 (shallow/complete copy for Phase1) */
    /* Clone page tables using COW-aware helper in kernel; host test uses pt_clone_current */
#ifdef HOST_TEST
    extern void *pt_clone_current(void);
    void *new_pml4 = pt_clone_current();
#else
    extern void *pt_clone_for_cow(void *parent_pml4);
    void *new_pml4 = pt_clone_for_cow(parent->page_table);
#endif
    if (new_pml4) child->page_table = new_pml4; else child->page_table = parent->page_table;

    /* Share heap region (copy-on-write semantics) if present */
    if (parent->heap_start && parent->heap_end && parent->heap_end > parent->heap_start) {
        extern void virtual_memory_share(uint64_t, uint64_t);
        virtual_memory_share(parent->heap_start, parent->heap_end - parent->heap_start);
    }

    /* Clone stack memory if parent has a stack_base/size (we give child its own stack) */
    if (parent->stack_base && parent->stack_size) {
#ifdef HOST_TEST
        extern void *kmalloc(unsigned int);
        void *new_stack = kmalloc((unsigned int)parent->stack_size);
#else
        extern void *virtual_memory_alloc(uint64_t, uint64_t, int);
        void *new_stack = virtual_memory_alloc(0, parent->stack_size, 0);
#endif
        if (new_stack) {
            /* copy whole stack region */
            memcpy(new_stack, (void *)parent->stack_base, parent->stack_size);
            /* compute child's stack_top relative to new base */
            uint64_t offset = parent->stack_top - parent->stack_base;
            child->stack_base = (uint64_t)new_stack;
            child->stack_size = parent->stack_size;
            child->stack_top = child->stack_base + offset;

                /* Ensure child will return 0 from fork by clearing saved RAX slot.
               Saved regs layout used by our ISR places r15..rax in the first
               REG_COUNT words; rax is at index REG_COUNT-1 (14). */
            uint64_t *saved = (uint64_t *)child->stack_top;
            const int REG_COUNT = 15;
            saved[REG_COUNT - 1] = 0; /* set child RAX=0 */
        }
    }

    /* Duplicate file descriptor table (simple shallow copy; future: refcounts) */
    for (int i = 0; i < 16; ++i) {
        child->fds[i] = parent->fds[i];
        if (child->fds[i] >= 0) {
            extern void fs_incref(int fd);
            fs_incref(child->fds[i]);
        }
    }
    
    child->state = 0; /* new */
    pm_proc_table[proc_cnt++] = child;
    /* Add new process to scheduler if available */
    extern int sched_add_existing_process(process_t *p);
    (void)sched_add_existing_process(child);
    serial_puts("[pm] cloned process pid=");
    serial_put_hex(child->pid);
    serial_putc('\n');
    return child;
}

process_t *pm_get_current(void) { return current; }
void pm_set_current(process_t *p) { current = p; }
int pm_count(void) { return proc_cnt; }

process_t *pm_find_by_pid(uint64_t pid) {
    for (int i = 0; i < proc_cnt; ++i) {
        if (pm_proc_table[i] && pm_proc_table[i]->pid == pid) return pm_proc_table[i];
    }
    return NULL;
}
