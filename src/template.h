#ifndef ALEPHIUM_TEMPLATE_H
#define ALEPHIUM_TEMPLATE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "messages.h"
#include "blake3.h"
#include "uv.h"
#include "constants.h"

typedef struct mining_template_t {
    job_t *job;
    volatile uint32_t ref_count;

    uint64_t chain_task_count; // increase this by one everytime the template for the chain is updated
} mining_template_t;

void free_template(mining_template_t *template)
{
    template->ref_count -= 1;
    if (template->ref_count == 0) {
        free_job(template->job);
        free(template);
    }
}

volatile mining_template_t* mining_templates[chain_nums] = {};
volatile uint64_t mining_counts[chain_nums];
uint64_t task_counts[chain_nums] = { 0 };
bool mining_templates_initialized = false;

void update_templates(job_t *job)
{
    mining_template_t *new_template = malloc(sizeof(mining_template_t));
    new_template->job = job;
    new_template->ref_count = 1; // referred by mining_templates

    ssize_t chain_index = job->from_group * group_nums + job->to_group;
    task_counts[chain_index] += 1;
    new_template->chain_task_count = task_counts[chain_index];

    mining_template_t *last_template = (mining_template_t *)mining_templates[chain_index];
    if (last_template) {
        free_template(last_template);
    }
    mining_templates[chain_index] = new_template;
}

bool expire_template_for_new_block(mining_template_t *template)
{
    job_t *job = template->job;
    ssize_t chain_index = job->from_group * group_nums + job->to_group;
    uint64_t block_task_count = template->chain_task_count;

    mining_template_t *latest_template = (mining_template_t *)(mining_templates[chain_index]);
    if (latest_template) {
        printf("new block mined, remove the outdated template\n");
        mining_templates[chain_index] = NULL;
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
        if (mining_templates[i] && (i_hash_count < least_hash_count)) {
            to_mine_index = i;
            least_hash_count = i_hash_count;
        }
    }

    return to_mine_index;
}

#endif // ALEPHIUM_TEMPLATE_H
