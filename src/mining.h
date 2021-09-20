#ifndef ALEPHIUM_MINING_H
#define ALEPHIUM_MINING_H

#include "cpu_mining.h"

void start_worker_mining(mining_worker_t *worker)
{
    // printf("start mine: %d %d\n", work->job->from_group, work->job->to_group);
    reset_worker(worker);
    mine_internal(worker);
}

#endif // ALEPHIUM_MINING_H
