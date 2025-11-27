/* kernel/fs.h - tiny file table API for Phase1 */
#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <stdint.h>

typedef struct { int ref; } file_t;

void fs_init(void);
int fs_alloc(void);
void fs_incref(int fd);
void fs_decref(int fd);
int fs_refcount(int fd);

#endif
