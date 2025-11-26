/* tests/sys_fork_test.c - host-side unit test for sys_fork prototype */

#include <stdio.h>

/* For host-side unit tests we include the kernel process and syscall
 * implementations directly. We replace the serial port implementation
 * with simple host-side stubs so the test can run under Linux.
 */
#include "../kernel/tasks/process.c"

/* minimal serial stubs used by syscall.c for host tests */
#include <inttypes.h>
void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t value) { printf("%" PRIx64, value); }

#include "../kernel/syscall.c"

int main(void) {
    int before = proc_count; /* proc_count comes from process.c */

    int child = sys_fork();
    if (child < 0) {
        printf("ERROR: sys_fork failed and returned %d\n", child);
        return 1;
    }

    if (proc_count != before + 1) {
        printf("ERROR: expected proc_count=%d got %d\n", before + 1, proc_count);
        return 1;
    }

    printf("PASS: sys_fork returned pid=%d (proc_count=%d)\n", child, proc_count);
    return 0;
}
