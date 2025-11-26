/* kernel/scheduler/preemptive.c - basic preemptive scheduler for Phase1 */
#include "preemptive.h"
#include "../process_manager.h"
#include "../mm/pagetable.h"
#include "../drivers/serial.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_TASKS 16
#define KERNEL_STACK_SIZE (16*1024)

static process_t *tasks[MAX_TASKS];
static int task_count = 0;
static int sched_current = -1; /* index of current task */

/* Helper: build initial stack frame for a new kernel task */
static uint64_t *prepare_initial_frame(void *stack_top, uint64_t entry_point) {
    /* We will create a stack with saved registers (15 qwords) followed
       by the interrupt frame (RIP, CS, RFLAGS). The ISR expects the
       saved registers to be present so pop restores them and then iretq
       uses the interrupt frame to set RIP and continue.
    */
    const int REG_COUNT = 15; /* r15..rbp..rax order used by ISR */
    uint64_t *sp = (uint64_t *)stack_top;

    /* Align down */
    sp = (uint64_t *)((uintptr_t)sp & ~0xFULL);

    /* Reserve space for regs + irq frame */
    sp -= (REG_COUNT + 3);

    /* we'll fill regs in order the ISR expects to pop: r15..r14.. ..rax */
    for (int i = 0; i < REG_COUNT; i++) sp[i] = 0; /* zero regs */

    /* Interrupt frame: RIP, CS, RFLAGS */
    sp[REG_COUNT + 0] = entry_point; /* RIP */
    sp[REG_COUNT + 1] = 0x08;        /* CS (kernel code) */
    sp[REG_COUNT + 2] = 0x202;       /* RFLAGS (IF = 1) */

    /* Return pointer to location of r15 (top of saved regs) */
    return sp;
}

int task_create(void (*entry)(void)) {
    if (task_count >= MAX_TASKS) return -1;

    extern void *virtual_memory_alloc(uint64_t, uint64_t, int);
    extern uint64_t pm_register_process(process_t *p);
    extern void *kmalloc(unsigned int size);

    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (!proc) return -1;
    memset(proc, 0, sizeof(process_t));
    proc->entry_point = (uint64_t)entry;

    /* allocate kernel stack */
    void *stk = virtual_memory_alloc(0, KERNEL_STACK_SIZE, 0);
    if (!stk) return -1;
    uint64_t stktop = (uint64_t)stk + KERNEL_STACK_SIZE;

    uint64_t *frame = prepare_initial_frame((void*)stktop, (uint64_t)entry);
    proc->stack_top = (uint64_t)frame; /* initial RSP for context */
    proc->state = 0;

    pm_register_process(proc);

    tasks[task_count++] = proc;
    return 0;
}

/* Test helper: add an existing process_t into the scheduler's task list
 * Returns index or -1 on failure.
 */
int sched_add_existing_process(process_t *p) {
    if (task_count >= MAX_TASKS) return -1;
    tasks[task_count++] = p;
    return task_count - 1;
}

void scheduler_start(void) {
    /* choose first task and switch to it by changing stack and iret via assembly
       call. We'll simply pause here and let timer interrupts perform switching.
    */
    extern void enable_interrupts(void);
    extern void disable_interrupts(void);

    /* select first task if present */
    if (task_count == 0) return;
    sched_current = 0;
    /* Switch to the next task's page table (CR3) */
    if (tasks[sched_current] && tasks[sched_current]->page_table) {
        pt_set_cr3((void *)tasks[sched_current]->page_table);
    }
    pm_set_current(tasks[sched_current]);

    /* Jump directly into the first task by switching stack and performing an iret
       sequence: write the stack pointer to tasks[current]->stack_top and then
       return. We'll simulate by just enabling interrupts and returning so the
       next timer interrupt will schedule tasks properly. In a full kernel we
       would perform an immediate context switch here.
    */
    enable_interrupts();

    /* Wait forever — scheduler will run via interrupts */
    for (;;)
        asm volatile ("hlt");
}

/* Scheduler tick called from IRQ handler: saved_regs_ptr points to region
   where ISR pushed registers. We must save this pointer for current task and
   return the next task's saved RSP so the ISR will switch stacks.
*/
uint64_t scheduler_tick(uint64_t *saved_regs_ptr) {
    /* Save current task rip */
    /* Advance current index round-robin */
    if (sched_current >= 0 && sched_current < task_count) {
        tasks[sched_current]->stack_top = (uint64_t)saved_regs_ptr;
    }

    int next = (sched_current + 1) % (task_count == 0 ? 1 : task_count);
    sched_current = next;
    pm_set_current(tasks[sched_current]);

    /* Send EOI to PIC */
    extern void pic_send_eoi(int irq);
    pic_send_eoi(0);

    return tasks[sched_current]->stack_top;
}
