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

typedef struct mining_worker_t {
    uint32_t id;
	blake3_hasher hasher;
	uint8_t hash[32];
	uint32_t hash_count;
	uint8_t nonce[24];
	uint32_t nonce_update_index;
	bool found_good_hash;

    job_t *job;
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

uv_work_t req[parallel_mining_works];
mining_worker_t mining_workers[parallel_mining_works];
uint8_t write_buffers[parallel_mining_works][2048 * 1024];

ssize_t write_new_block(mining_worker_t *worker)
{
	uint32_t worker_id = worker->id;
	job_t *job = worker->job;
	uint8_t *nonce = worker->nonce;
	uint8_t *write_pos = write_buffers[worker_id];

	ssize_t block_size = 24 + job->header_blob.len + job->txs_blob.len;
	ssize_t message_size = 1 + 4 + block_size;

	printf("message: %ld\n", message_size);
	write_size(&write_pos, message_size);
	write_byte(&write_pos, 0); // message type
	write_size(&write_pos, block_size);
	write_bytes(&write_pos, nonce, 24);
	write_blob(&write_pos, &job->header_blob);
	write_blob(&write_pos, &job->txs_blob);

	return message_size + 4;
}

#endif // ALEPHIUM_WORKER_H
