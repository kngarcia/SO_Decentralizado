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
    /* Assign default page table (kernel PML4) */
    extern void *pt_get_kernel_pml4(void);
    proc->page_table = pt_get_kernel_pml4();
    proc_cnt++;
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
    /* By default copy parent's page_table pointer (shallow clone for Phase1) */
    child->page_table = parent->page_table;
    child->state = 0; /* new */
    pm_proc_table[proc_cnt++] = child;
    serial_puts("[pm] cloned process pid=");
    serial_put_hex(child->pid);
    serial_putc('\n');
    return child;
}

process_t *pm_get_current(void) { return current; }
void pm_set_current(process_t *p) { current = p; }
int pm_count(void) { return proc_cnt; }
