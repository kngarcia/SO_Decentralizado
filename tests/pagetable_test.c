/* tests/pagetable_test.c - host-side test for page table clone helpers */

#define HOST_TEST
#include <stdio.h>
#include <stdint.h>

#include "../kernel/mm/pagetable.c"

int main(void) {
    void *k = pt_get_kernel_pml4();
    if (!k) { printf("FAIL: kernel pml4 is NULL\n"); return 1; }

    void *c = pt_clone_current();
    if (!c) { printf("FAIL: clone returned NULL\n"); return 1; }

    if (c == k) { printf("FAIL: clone returned same pointer as kernel pml4\n"); return 1; }

    printf("PASS: pagetable clone works (kernel=%p clone=%p)\n", k, c);
    return 0;
}
