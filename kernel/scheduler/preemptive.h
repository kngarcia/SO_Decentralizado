/* kernel/scheduler/preemptive.h */
#ifndef PREEMPTIVE_H
#define PREEMPTIVE_H

#include <stdint.h>
#include "../elf_loader.h"

/* Create a kernel task (entry is function pointer). Returns pid or -1. */
int task_create(void (*entry)(void));

/* Start the scheduler — transfers control to tasks (non-returning) */
void scheduler_start(void);

/* Called by IRQ handler: pass pointer to saved regs (current RSP). Returns new RSP */
uint64_t scheduler_tick(uint64_t *saved_regs_ptr);

/* Test helper: add an existing process to tasks list */
int sched_add_existing_process(process_t *p);

#endif
