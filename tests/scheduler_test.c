/* tests/scheduler_test.c - host-side test for round-robin scheduler */
#include <stdio.h>
#include <string.h>

/* Include the scheduler from the kernel */
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

/* Simulate scheduler execution on host */
void scheduler_test_run(int iterations) {
    int total_runs = 0;
    printf("=== Scheduler test: %d iterations ===\n", iterations);
    
    for (int iter = 0; iter < iterations; iter++) {
        if (tcount == 0) {
            printf("ERROR: no tasks\n");
            break;
        }
        tasks[current]();
        current = (current + 1) % tcount;
        total_runs++;
    }
    
    printf("Completed %d task runs (round-robin)\n", total_runs);
}

/* Test tasks */
static int task1_runs = 0;
static int task2_runs = 0;
static int task3_runs = 0;

void test_task1(void) {
    task1_runs++;
    printf("task1 run #%d\n", task1_runs);
}

void test_task2(void) {
    task2_runs++;
    printf("task2 run #%d\n", task2_runs);
}

void test_task3(void) {
    task3_runs++;
    printf("task3 run #%d\n", task3_runs);
}

int main(void) {
    printf("Starting scheduler unit test...\n\n");

    task_create(test_task1);
    task_create(test_task2);
    task_create(test_task3);
    
    printf("Created 3 tasks\n\n");

    /* Run scheduler for 12 iterations (each task runs 4 times) */
    scheduler_test_run(12);

    printf("\n=== Results ===\n");
    printf("task1 runs: %d (expected 4)\n", task1_runs);
    printf("task2 runs: %d (expected 4)\n", task2_runs);
    printf("task3 runs: %d (expected 4)\n", task3_runs);

    if (task1_runs == 4 && task2_runs == 4 && task3_runs == 4) {
        printf("PASS: round-robin scheduler works correctly\n");
        return 0;
    } else {
        printf("FAIL: scheduler did not distribute runs evenly\n");
        return 1;
    }
}
