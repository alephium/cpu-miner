#ifndef ALEPHIUM_WORKER_H
#define ALEPHIUM_WORKER_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "messages.h"
#include "blake3.h"
#include "uv.h"

typedef struct mining_work_t {
	job_t *job;
	uint32_t worker_id;
} mining_work_t;

typedef struct mining_worker_t {
	blake3_hasher hasher;
	uint8_t hash[32];
	uint32_t hash_count;
	uint8_t nonce[24];
	uint32_t nonce_update_index;
	bool found_good_hash;
} mining_worker_t;

void reset_worker(mining_worker_t *worker)
{
	worker->hash_count = 0;
	for (int i = 0; i < 24; i++) {
		worker->nonce[i] = rand();
	}
	worker->nonce_update_index = 0;
	worker->found_good_hash = false;
}

void update_nonce(mining_worker_t *worker)
{
	uint32_t old_index = worker->nonce_update_index;
	worker->nonce[old_index] += 1;
	worker->nonce_update_index = (old_index + 1) % 24;
}

mining_work_t mining_works[parallel_mining_works];
uv_work_t req[parallel_mining_works];
mining_worker_t mining_workers[parallel_mining_works];

#endif // ALEPHIUM_WORKER_H
