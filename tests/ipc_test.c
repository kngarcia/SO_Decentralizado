#include <stdio.h>
#include <string.h>

/* Pull in the kernel IPC implementation for host-side unit test. */
#include "../kernel/ipc/message.c"

int main(void) {
    char buf[128];

    if (ipc_recv(buf, sizeof(buf)) == 0) {
        printf("ERROR: recv succeeded when queue empty\n");
        return 1;
    }

    if (ipc_send("hello") != 0) {
        printf("ERROR: send failed\n");
        return 1;
    }
    if (ipc_send("world") != 0) {
        printf("ERROR: send failed 2\n");
        return 1;
    }

    if (ipc_recv(buf, sizeof(buf)) != 0) {
        printf("ERROR: recv failed\n");
        return 1;
    }
    printf("recv1: %s\n", buf);

    if (ipc_recv(buf, sizeof(buf)) != 0) {
        printf("ERROR: recv failed 2\n");
        return 1;
    }
    printf("recv2: %s\n", buf);

    /* Fill the queue */
    for (int i = 0; i < 32; i++) {
        char t[32];
        snprintf(t, sizeof(t), "m-%d", i);
        if (ipc_send(t) != 0) {
            printf("ERROR: send failed at %d\n", i);
            return 1;
        }
    }
    /* Now queue is full; next send should fail */
    if (ipc_send("overflow") == 0) {
        printf("ERROR: expected overflow but send succeeded\n");
        return 1;
    }

    printf("IPC tests passed\n");
    return 0;
}
