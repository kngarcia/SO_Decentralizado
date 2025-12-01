/* kernel/sync/distributed_lock.c
 * Distributed synchronization primitives (locks, semaphores)
 */

#include <stdint.h>
#include <string.h>
#include "../drivers/serial.h"

#define MAX_LOCKS 64
#define LOCK_NAME_LEN 32
#define LOCK_TIMEOUT_DEFAULT 5000  /* 5 seconds */

typedef struct {
    char name[LOCK_NAME_LEN];
    uint32_t owner_node;
    uint64_t owner_pid;
    uint32_t timestamp;
    int active;
    int local_held;
} distributed_lock_t;

static distributed_lock_t locks[MAX_LOCKS];
static uint32_t local_node_id = 1;
static uint32_t logical_clock = 0;
static int dlock_initialized = 0;

/* Initialize distributed locking */
int dlock_init(uint32_t node_id) {
    local_node_id = node_id;
    logical_clock = 0;
    
    for (int i = 0; i < MAX_LOCKS; i++) {
        locks[i].active = 0;
        locks[i].local_held = 0;
    }
    
    dlock_initialized = 1;
    
    serial_puts("[dlock] Distributed locking initialized for node ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    return 0;
}

/* Lamport timestamp comparison */
static int timestamp_less_than(uint32_t ts1, uint32_t node1, uint32_t ts2, uint32_t node2) {
    if (ts1 < ts2) return 1;
    if (ts1 > ts2) return 0;
    /* Tie-breaker: lower node ID wins */
    return node1 < node2;
}

/* Find lock by name */
static distributed_lock_t *find_lock(const char *lock_name) {
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (locks[i].active && strcmp(locks[i].name, lock_name) == 0) {
            return &locks[i];
        }
    }
    return NULL;
}

/* Create new lock */
static distributed_lock_t *create_lock(const char *lock_name) {
    /* Find free slot */
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (!locks[i].active) {
            locks[i].active = 1;
            strncpy(locks[i].name, lock_name, LOCK_NAME_LEN - 1);
            locks[i].name[LOCK_NAME_LEN - 1] = '\0';
            locks[i].owner_node = 0;
            locks[i].owner_pid = 0;
            locks[i].timestamp = 0;
            locks[i].local_held = 0;
            return &locks[i];
        }
    }
    return NULL;
}

/* Acquire distributed lock (Ricart-Agrawala algorithm) */
int dlock_acquire(const char *lock_name, uint32_t timeout_ms) {
    if (!dlock_initialized || !lock_name) return -1;
    
    serial_puts("[dlock] Acquiring lock '");
    serial_puts(lock_name);
    serial_puts("'\n");
    
    /* Increment logical clock */
    logical_clock++;
    
    distributed_lock_t *lock = find_lock(lock_name);
    if (!lock) {
        lock = create_lock(lock_name);
        if (!lock) {
            serial_puts("[dlock] ERROR: No free lock slots\n");
            return -1;
        }
    }
    
    /* Check if already held locally */
    if (lock->local_held && lock->owner_node == local_node_id) {
        serial_puts("[dlock] Lock already held locally (reentrant)\n");
        return 0;
    }
    
    /* Build REQUEST message */
    typedef struct {
        uint8_t type;       /* 0x20 = LOCK_REQUEST */
        uint32_t requester;
        uint32_t timestamp;
        uint64_t pid;
        char lock_name[LOCK_NAME_LEN];
    } lock_request_t;
    
    lock_request_t req;
    req.type = 0x20;
    req.requester = local_node_id;
    req.timestamp = logical_clock;
    req.pid = 0;  /* Current PID (stub) */
    strncpy(req.lock_name, lock_name, LOCK_NAME_LEN - 1);
    
    /* Send REQUEST to all nodes (stub) */
    serial_puts("[dlock] Broadcasting lock request (timestamp=");
    serial_put_hex(logical_clock);
    serial_puts(")\n");
    
    /* Wait for REPLY from all nodes (simplified - assume immediate grant) */
    uint32_t wait_time = 0;
    while (wait_time < timeout_ms) {
        /* Check if all nodes replied (stub - assume yes) */
        break;
    }
    
    /* Acquire lock */
    lock->owner_node = local_node_id;
    lock->owner_pid = 0;
    lock->timestamp = logical_clock;
    lock->local_held = 1;
    
    serial_puts("[dlock] Lock '");
    serial_puts(lock_name);
    serial_puts("' acquired\n");
    
    return 0;
}

/* Release distributed lock */
int dlock_release(const char *lock_name) {
    if (!dlock_initialized || !lock_name) return -1;
    
    serial_puts("[dlock] Releasing lock '");
    serial_puts(lock_name);
    serial_puts("'\n");
    
    distributed_lock_t *lock = find_lock(lock_name);
    if (!lock) {
        serial_puts("[dlock] ERROR: Lock not found\n");
        return -1;
    }
    
    if (!lock->local_held || lock->owner_node != local_node_id) {
        serial_puts("[dlock] ERROR: Lock not held by this node\n");
        return -1;
    }
    
    /* Build RELEASE message */
    typedef struct {
        uint8_t type;       /* 0x21 = LOCK_RELEASE */
        uint32_t releaser;
        char lock_name[LOCK_NAME_LEN];
    } lock_release_t;
    
    lock_release_t rel;
    rel.type = 0x21;
    rel.releaser = local_node_id;
    strncpy(rel.lock_name, lock_name, LOCK_NAME_LEN - 1);
    
    /* Broadcast RELEASE to all nodes (stub) */
    serial_puts("[dlock] Broadcasting lock release\n");
    
    /* Release locally */
    lock->local_held = 0;
    lock->owner_node = 0;
    lock->owner_pid = 0;
    
    serial_puts("[dlock] Lock '");
    serial_puts(lock_name);
    serial_puts("' released\n");
    
    return 0;
}

/* Handle incoming lock request */
int dlock_handle_request(uint32_t requester, uint32_t req_timestamp, const char *lock_name) {
    serial_puts("[dlock] Received lock request from node ");
    serial_put_hex(requester);
    serial_puts(" for lock '");
    serial_puts(lock_name);
    serial_puts("'\n");
    
    /* Update logical clock */
    if (req_timestamp > logical_clock) {
        logical_clock = req_timestamp;
    }
    logical_clock++;
    
    distributed_lock_t *lock = find_lock(lock_name);
    
    /* If we don't hold the lock, send REPLY immediately */
    if (!lock || !lock->local_held) {
        serial_puts("[dlock] Sending REPLY (lock not held)\n");
        /* Send REPLY packet (stub) */
        return 0;
    }
    
    /* If we hold the lock, defer REPLY based on timestamp */
    if (timestamp_less_than(req_timestamp, requester, lock->timestamp, local_node_id)) {
        serial_puts("[dlock] Request has higher priority, sending REPLY\n");
        /* Send REPLY packet (stub) */
    } else {
        serial_puts("[dlock] Request has lower priority, deferring REPLY\n");
        /* Queue request for later (stub) */
    }
    
    return 0;
}

/* Handle incoming lock release */
int dlock_handle_release(uint32_t releaser, const char *lock_name) {
    serial_puts("[dlock] Received lock release from node ");
    serial_put_hex(releaser);
    serial_puts(" for lock '");
    serial_puts(lock_name);
    serial_puts("'\n");
    
    distributed_lock_t *lock = find_lock(lock_name);
    if (lock && lock->owner_node == releaser) {
        lock->owner_node = 0;
        lock->owner_pid = 0;
        lock->local_held = 0;
    }
    
    /* Process deferred requests (stub) */
    
    return 0;
}

/* Try to acquire lock without blocking */
int dlock_try_acquire(const char *lock_name) {
    if (!dlock_initialized || !lock_name) return -1;
    
    distributed_lock_t *lock = find_lock(lock_name);
    if (!lock) {
        lock = create_lock(lock_name);
        if (!lock) return -1;
    }
    
    if (lock->owner_node == 0) {
        /* Lock is free */
        lock->owner_node = local_node_id;
        lock->local_held = 1;
        lock->timestamp = ++logical_clock;
        return 0;
    }
    
    return -1;  /* Lock held by another node */
}

/* Print lock statistics */
void dlock_print_stats(void) {
    serial_puts("\n[dlock] === Lock Statistics ===\n");
    
    int active_count = 0;
    int held_count = 0;
    
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (locks[i].active) {
            active_count++;
            
            serial_puts("  Lock '");
            serial_puts(locks[i].name);
            serial_puts("': ");
            
            if (locks[i].local_held) {
                serial_puts("HELD (local)");
                held_count++;
            } else if (locks[i].owner_node != 0) {
                serial_puts("HELD by node ");
                serial_put_hex(locks[i].owner_node);
            } else {
                serial_puts("FREE");
            }
            serial_putc('\n');
        }
    }
    
    serial_puts("[dlock] Total locks: ");
    serial_put_hex(active_count);
    serial_puts("\n[dlock] Held locally: ");
    serial_put_hex(held_count);
    serial_puts("\n[dlock] Logical clock: ");
    serial_put_hex(logical_clock);
    serial_puts("\n[dlock] ===========================\n\n");
}
