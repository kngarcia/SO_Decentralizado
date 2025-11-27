/* kernel/mm/physical_memory.c - simple physical memory bitmap (stub) */
#include <stdint.h>
#define FRAME_SIZE 4096
#define MAX_FRAMES 32768  /* 128MB / 4KB */

static uint8_t frame_bitmap[MAX_FRAMES/8];
/* Per-frame reference counts to support copy-on-write (number of owners).
     Each frame index corresponds to frame_bitmap index; refcount=0 => free.
 */
static uint16_t frame_refcount[MAX_FRAMES];

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
    /* Initialize refcount for allocated frame */
    frame_refcount[f] = 1;
    return f * FRAME_SIZE;
}

void free_frame(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;
    clear_frame(frame);
    frame_refcount[frame] = 0;
}

/* Increase reference count for a physical frame (address must be frame aligned) */
void frame_incref(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;
    if (frame < MAX_FRAMES) frame_refcount[frame]++;
}

/* Decrease reference count and free frame if reaches zero */
void frame_decref(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;
    if (frame >= MAX_FRAMES) return;
    if (frame_refcount[frame] > 0) frame_refcount[frame]--;
    if (frame_refcount[frame] == 0) clear_frame(frame);
}

/* Return refcount for a given frame address */
int frame_refcount_get(uint32_t addr) {
    uint32_t frame = addr / FRAME_SIZE;
    if (frame >= MAX_FRAMES) return 0;
    return frame_refcount[frame];
}
