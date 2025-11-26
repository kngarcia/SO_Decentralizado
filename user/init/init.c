/* user/init/init.c - early userland "init", here compiled as a kernel function for demo */
#include <stdint.h>

/* For this minimal demo, init is run by kernel scheduler as a task (no real userland) */
void init_task(void) {
    extern void show_string(const char *);
    show_string("init: user-mode init running (simulated)\n");
    /* Simulate a distributed daemon registration (placeholder) */
    for (volatile int i=0;i<10000000;i++);
    show_string("init: registration complete\n");
    while (1) {
        /* idle */
    }
}
