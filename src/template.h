#ifndef ALEPHIUM_TEMPLATE_H
#define ALEPHIUM_TEMPLATE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "messages.h"
#include "blake3.h"
#include "uv.h"
#include "constants.h"

typedef struct mining_template_t {
    job_t *job;
    _Atomic(uint32_t) ref_count;

    uint64_t chain_task_count; // increase this by one everytime the template for the chain is updated
} mining_template_t;

void store_template__ref_count(mining_template_t *template, uint32_t value)
{
    atomic_store(&(template->ref_count), value);
}

uint32_t add_template__ref_count(mining_template_t *template, uint32_t value)
{
    return atomic_fetch_add(&(template->ref_count), value);
}

uint32_t sub_template__ref_count(mining_template_t *template, uint32_t value)
{
    return atomic_fetch_sub(&(template->ref_count), value);
}

void free_template(mining_template_t *template)
{
    uint32_t old_count = sub_template__ref_count(template, 1);
    if (old_count == 0) {
        free_job(template->job);
        free(template);
    }
}

_Atomic(mining_template_t*) mining_templates[chain_nums] = {};
_Atomic(uint64_t) mining_counts[chain_nums];
uint64_t task_counts[chain_nums] = { 0 };
bool mining_templates_initialized = false;

mining_template_t* load_template(ssize_t chain_index)
{
    return atomic_load(&(mining_templates[chain_index]));
}

void store_template(ssize_t chain_index, mining_template_t* new_template)
{
    atomic_store(&(mining_templates[chain_index]), new_template);
}

void update_templates(job_t *job)
{
    mining_template_t *new_template = malloc(sizeof(mining_template_t));
    new_template->job = job;
    store_template__ref_count(new_template, 1); // referred by mining_templates

    ssize_t chain_index = job->from_group * group_nums + job->to_group;
    task_counts[chain_index] += 1;
    new_template->chain_task_count = task_counts[chain_index];

    // TODO: optimize with atomic_exchange
    mining_template_t *last_template = load_template(chain_index);
    if (last_template) {
        free_template(last_template);
    }
    store_template(chain_index, new_template);
}

bool expire_template_for_new_block(mining_template_t *template)
{
    job_t *job = template->job;
    ssize_t chain_index = job->from_group * group_nums + job->to_group;
    uint64_t block_task_count = template->chain_task_count;

    mining_template_t *latest_template = load_template(chain_index);
    if (latest_template) {
        printf("new block mined, remove the outdated template\n");
        store_template(chain_index, NULL);
        free_template(latest_template);
        return true;
    } else {
        return false;
    }
}

int32_t next_chain_to_mine()
{
    int32_t to_mine_index = -1;
    uint64_t least_hash_count = UINT64_MAX;
    for (int32_t i = 0; i < chain_nums; i ++) {
        uint64_t i_hash_count = mining_counts[i];
        if (load_template(i) && (i_hash_count < least_hash_count)) {
            to_mine_index = i;
            least_hash_count = i_hash_count;
        }
    }

    return to_mine_index;
}

#endif // ALEPHIUM_TEMPLATE_H
