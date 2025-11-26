/* kernel/scheduler/round_robin.c - cooperative round-robin scheduler stub */
#include <stdint.h>
#include <stddef.h>

typedef void (*task_fn)(void);

#define MAX_TASKS 8
static task_fn tasks[MAX_TASKS];
static int tcount = 0;
static int current = 0;

int rr_task_create(task_fn fn) {
    if (tcount >= MAX_TASKS) return -1;
    tasks[tcount++] = fn;
    return tcount-1;
}

/* Very simple scheduler loop */
void rr_scheduler_start(void) {
    extern void show_string(const char *);
    show_string("scheduler: starting\n");
    while (1) {
        if (tcount==0) { show_string("no tasks\n"); break; }
        tasks[current]();
        current = (current+1) % tcount;
    }
}

/* Example task helper for tests (defined in tasks/process.c) */
