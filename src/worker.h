#ifndef ALEPHIUM_WORKER_H
#define ALEPHIUM_WORKER_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "messages.h"
#include "blake3.h"
#include "uv.h"
#include "template.h"

typedef struct mining_worker_t {
    uint32_t id;
    blake3_hasher hasher;
    uint8_t hash[32];
    uint32_t hash_count;
    uint8_t nonce[24];
    _Atomic(bool) found_good_hash;

    _Atomic(mining_template_t *) template;
} mining_worker_t;

bool load_worker__found_good_hash(mining_worker_t *worker)
{
    return atomic_load(&(worker->found_good_hash));
}

void store_worker_found_good_hash(mining_worker_t *worker, bool value)
{
    atomic_store(&(worker->found_good_hash), value);
}

mining_template_t *load_worker__template(mining_worker_t *worker)
{
    return atomic_load(&(worker->template));
}

void store_worker__template(mining_worker_t *worker, mining_template_t *template)
{
    atomic_store(&(worker->template), template);
}

void reset_worker(mining_worker_t *worker)
{
    worker->hash_count = 0;
    for (int i = 0; i < 24; i++) {
        worker->nonce[i] = rand();
    }
    store_worker_found_good_hash(worker, false);
}

void update_nonce(mining_worker_t *worker)
{
    int64_t *short_nonce = (int64_t *)worker->nonce;
    // printf("%s\n", bytes_to_hex(worker->nonce, 24));
    *short_nonce += 1;
}

typedef struct mining_req {
    _Atomic(mining_worker_t *) worker;
} mining_req_t;

uv_work_t req[parallel_mining_works] = {};
mining_worker_t mining_workers[parallel_mining_works];
uint8_t write_buffers[parallel_mining_works][2048 * 1024];

mining_worker_t *load_req_worker(uv_work_t *req)
{
    mining_req_t *mining_req = req->data;
    return atomic_load(&(mining_req->worker));
}

void store_req_data(ssize_t worker_id, mining_worker_t *worker)
{
    if (!req[worker_id].data) {
        req[worker_id].data = malloc(sizeof(mining_req_t));
    }
    mining_req_t *mining_req = req[worker_id].data;
    atomic_store(&(mining_req->worker), worker);
}

ssize_t write_new_block(mining_worker_t *worker)
{
    uint32_t worker_id = worker->id;
    job_t *job = load_worker__template(worker)->job;
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

void setup_template(mining_worker_t *worker, mining_template_t *template)
{
    add_template__ref_count(template, 1);
    store_worker__template(worker, template);
}

#endif // ALEPHIUM_WORKER_H
