/* tests/ipc_scheduler_integration_test.c - test IPC + scheduler integration */
#include <stdio.h>
#include <string.h>

/* Include IPC from kernel */
#include "../kernel/ipc/message.c"

/* Include scheduler logic */
#define MAX_TASKS 8
typedef void (*task_fn)(void);

static task_fn tasks[MAX_TASKS];
static int tcount = 0;
static int current = 0;

int task_create(task_fn fn) {
    if (tcount >= MAX_TASKS) return -1;
    tasks[tcount++] = fn;
    return tcount-1;
}

void scheduler_run(int iterations) {
    for (int i = 0; i < iterations && tcount > 0; i++) {
        tasks[current]();
        current = (current + 1) % tcount;
    }
}

/* Test scenario: producer sends 3 messages, consumer receives them */
static int producer_state = 0;
static int consumer_state = 0;

void producer_task(void) {
    if (producer_state < 3) {
        char buf[64];
        snprintf(buf, sizeof(buf), "msg-%d", producer_state);
        if (ipc_send(buf) == 0) {
            printf("  [producer] sent: %s\n", buf);
            producer_state++;
        }
    }
}

void consumer_task(void) {
    if (consumer_state < 3) {
        char buf[128];
        if (ipc_recv(buf, sizeof(buf)) == 0) {
            printf("  [consumer] recv: %s\n", buf);
            consumer_state++;
        } else {
            printf("  [consumer] queue empty, waiting...\n");
        }
    }
}

int main(void) {
    printf("=== IPC + Scheduler Integration Test ===\n\n");

    task_create(producer_task);
    task_create(consumer_task);
    
    printf("Created 2 tasks: producer and consumer\n");
    printf("Producer will send 3 messages, consumer will receive them.\n\n");

    printf("Running scheduler for 20 iterations (cooperative round-robin):\n");
    scheduler_run(20);

    printf("\n=== Results ===\n");
    printf("Producer sent: %d messages (expected 3)\n", producer_state);
    printf("Consumer received: %d messages (expected 3)\n", consumer_state);

    if (producer_state == 3 && consumer_state == 3) {
        printf("PASS: IPC + scheduler integration works\n");
        return 0;
    } else {
        printf("FAIL: mismatch in producer/consumer state\n");
        return 1;
    }
}
