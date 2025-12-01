/* kernel/sync/distributed_lock.h */
#ifndef DISTRIBUTED_LOCK_H
#define DISTRIBUTED_LOCK_H

#include <stdint.h>

/* Initialize distributed locking */
int dlock_init(uint32_t node_id);

/* Acquire distributed lock */
int dlock_acquire(const char *lock_name, uint32_t timeout_ms);

/* Release distributed lock */
int dlock_release(const char *lock_name);

/* Try to acquire without blocking */
int dlock_try_acquire(const char *lock_name);

/* Handle incoming lock request */
int dlock_handle_request(uint32_t requester, uint32_t timestamp, const char *lock_name);

/* Handle incoming lock release */
int dlock_handle_release(uint32_t releaser, const char *lock_name);

/* Print lock statistics */
void dlock_print_stats(void);

#endif /* DISTRIBUTED_LOCK_H */
