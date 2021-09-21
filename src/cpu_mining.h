#ifndef ALEPHIUM_CPU_MINING_H
#define ALEPHIUM_CPU_MINING_H

#include "worker.h"

void mine_internal(mining_worker_t *worker)
{
    worker->hash_count++;
    update_nonce(worker);

    blake3_hasher *hasher = &worker->hasher;
    job_t *job = worker->template->job;
    blob_t *header = &job->header_blob;

    blake3_hasher_init(hasher);
    blake3_hasher_update(hasher, worker->nonce, 24);
    blake3_hasher_update(hasher, header->blob, header->len);
    blake3_hasher_finalize(hasher, worker->hash, BLAKE3_OUT_LEN);
    blake3_hasher_init(hasher);
    blake3_hasher_update(hasher, worker->hash, BLAKE3_OUT_LEN);
    blake3_hasher_finalize(hasher, worker->hash, BLAKE3_OUT_LEN);

    if (check_hash(worker->hash, &job->target, job->from_group, job->to_group)) {
        print_hex("found", worker->hash, 32);
        print_hex("with nonce", worker->nonce, 24);
        printf("with hash count: %d\n", worker->hash_count);
        print_hex("with target", job->target.blob, job->target.len);
        printf("with groups: %d %d\n", job->from_group, job->to_group);
        worker->found_good_hash = true;
        return;
    } else if (worker->hash_count == mining_steps) {
        return;
    } else {
        mine_internal(worker);
    }
}

#endif // ALEPHIUM_CPU_MINING_H
