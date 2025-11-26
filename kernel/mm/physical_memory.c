/* kernel/mm/physical_memory.c - simple physical memory bitmap (stub) */
#include <stdint.h>
#define FRAME_SIZE 4096
#define MAX_FRAMES 32768  /* 128MB / 4KB */

static uint8_t frame_bitmap[MAX_FRAMES/8];

static inline void set_frame(uint32_t frame) { frame_bitmap[frame/8] |= (1 << (frame%8)); }
static inline void clear_frame(uint32_t frame) { frame_bitmap[frame/8] &= ~(1 << (frame%8)); }
static inline int test_frame(uint32_t frame) { return frame_bitmap[frame/8] & (1 << (frame%8)); }

uint32_t first_free_frame(void) {
    for (uint32_t i=0;i<MAX_FRAMES;i++) if (!test_frame(i)) return i;
    return (uint32_t)-1;
}

uint32_t alloc_frame(void) {
    uint32_t f = first_free_frame();
    if (f == (uint32_t)-1) return 0;
    set_frame(f);
    return f * FRAME_SIZE;
}

void free_frame(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;
    clear_frame(frame);
}
