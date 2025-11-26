/* kernel/process_manager.h - simple process registry used for Phase1
 * Provides a global process list and helpers to register/clone processes.
 */
#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "elf_loader.h"

#define PM_MAX_PROCS 32

/* Register a process (takes ownership of proc pointer) and returns pid */
uint64_t pm_register_process(process_t *proc);

/* Clone current process and return new process pointer (or NULL) */
process_t *pm_clone_process(process_t *parent);

/* Helpers */
process_t *pm_get_current(void);
void pm_set_current(process_t *p);
int pm_count(void);

#endif
