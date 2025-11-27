/* tests/fd_fork_test.c - test that file descriptors are duplicated on fork */

#include <stdio.h>
#include <string.h>
#define HOST_TEST

#include "../kernel/process_manager.c"
#include "../kernel/mm/pagetable.c"
#include "../kernel/scheduler/preemptive.c"
#include "../kernel/mm/virtual_memory.c"

void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t v) { printf("%llx", (unsigned long long)v); }

void enable_interrupts(void) { }
void pic_send_eoi(int irq) { (void)irq; }

int main(void) {
    process_t *p = (process_t *)kmalloc(sizeof(process_t));
    if (!p) { printf("ERROR: kmalloc failed\n"); return 1; }
    memset(p, 0, sizeof(process_t));

    /* Setup some fake FDs */
    for (int i = 0; i < 16; ++i) p->fds[i] = -1;
    p->fds[0] = 10; p->fds[1] = 11; p->fds[2] = 12;

    pm_register_process(p);
    pm_set_current(p);

    process_t *child = pm_clone_process(p);
    if (!child) { printf("ERROR: pm_clone failed\n"); return 1; }

    for (int i = 0; i < 3; ++i) {
        if (child->fds[i] != p->fds[i]) {
            printf("ERROR: fd %d mismatched parent=%d child=%d\n", i, p->fds[i], child->fds[i]);
            return 1;
        }
    }

    printf("PASS: FDs duplicated on clone (0=%d,1=%d,2=%d)\n", child->fds[0], child->fds[1], child->fds[2]);
    return 0;
}
