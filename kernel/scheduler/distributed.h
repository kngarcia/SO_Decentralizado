/* kernel/scheduler/distributed.h */
#ifndef DISTRIBUTED_SCHEDULER_H
#define DISTRIBUTED_SCHEDULER_H

#include <stdint.h>
#include "../process_manager.h"

/* Initialize distributed scheduler */
int dsched_init(uint32_t node_id);

/* Register remote node in cluster */
int dsched_register_node(uint32_t node_id);

/* Migrate process to remote node */
int dsched_migrate_process(process_t *proc, uint32_t dest_node_id);

/* Request task from remote node */
int dsched_request_remote_task(uint32_t node_id);

/* Balance load across cluster */
int dsched_balance_load(void);

/* Update node load information */
int dsched_update_node_load(uint32_t node_id, uint32_t load);

/* Get node load */
uint32_t dsched_get_node_load(uint32_t node_id);

/* Print cluster statistics */
void dsched_print_cluster_stats(void);

#endif /* DISTRIBUTED_SCHEDULER_H */
