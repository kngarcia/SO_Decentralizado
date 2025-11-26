/* kernel/mm/pagetable.h - simple helpers for cloning/setting CR3 */
#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <stdint.h>

void *pt_clone_current(void);
void pt_set_cr3(void *p);
void *pt_get_kernel_pml4(void);

#endif
