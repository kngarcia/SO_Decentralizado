/* kernel/mm/physical_memory.c - simple physical memory bitmap (stub) */
#include <stdint.h>
#include <string.h>
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

/* Initialize physical memory allocator: mark first 0x2000000 (32MB) as reserved
   for kernel/early alloc, rest as free. Called once at boot. */
void physical_memory_init(void) {
    extern void serial_puts(const char *);
    serial_puts("[phys_mem] init start\n");
    /* Don't zero everything - BSS is already zero-initialized.
       Just reserve first 32MB (frames 0..8191) for kernel + early structures */
    for (uint32_t i = 0; i < 8192 && i < MAX_FRAMES; i++) {
        set_frame(i);
        frame_refcount[i] = 1; /* mark as used */
    }
    serial_puts("[phys_mem] init complete\n");
}

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

/* Get total physical memory in bytes */
uint64_t get_phys_mem_total(void) {
    return (uint64_t)MAX_FRAMES * FRAME_SIZE;
}

/* Get free physical memory in bytes */
uint64_t get_phys_mem_free(void) {
    uint32_t free_frames = 0;
    for (uint32_t i = 0; i < MAX_FRAMES; i++) {
        if (!test_frame(i)) {
            free_frames++;
        }
    }
    return (uint64_t)free_frames * FRAME_SIZE;
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
