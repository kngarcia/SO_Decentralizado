/* kernel/tasks/process.c - a tiny process abstraction (stub) */
#include <stdint.h>
#include "../elf_loader.h"

process_t proc_table[16];
int proc_count = 0;

int process_create(void (*entry)(void)) {
    if (proc_count >= 16) return -1;
    proc_table[proc_count].pid = proc_count;
    proc_table[proc_count].entry_point = (uint64_t)entry;
    proc_table[proc_count].stack_top = 0; /* simple stub */
    return proc_count++;
}
