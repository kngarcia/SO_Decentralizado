/* kernel/fs.c - very small in-kernel file table for Phase1
 * Provides a trivial 'open' allocation and reference counting so fork can
 * duplicate file descriptors safely for Phase1 tests.
 */
#include "fs.h"
#include <string.h>

#define MAX_FILES 64

static file_t files[MAX_FILES];

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; ++i) {
        files[i].ref = 0;
    }
}

int fs_alloc(void) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files[i].ref == 0) {
            files[i].ref = 1;
            return i;
        }
    }
    return -1;
}

void fs_incref(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return;
    files[fd].ref++;
}

void fs_decref(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return;
    if (files[fd].ref > 0) files[fd].ref--;
}

int fs_refcount(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return 0;
    return files[fd].ref;
}
