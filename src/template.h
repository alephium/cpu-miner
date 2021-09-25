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
    uint32_t ref_count;

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

mining_template_t* mining_templates[chain_nums] = {};
uint64_t mining_counts[chain_nums];
uint64_t task_counts[chain_nums];
bool mining_templates_initialized = false;

void update_templates(job_t *job)
{
    mining_template_t *new_template = malloc(sizeof(mining_template_t));
    new_template->job = job;
    new_template->ref_count = 1; // referred by mining_templates

    ssize_t chain_index = job->from_group * group_nums + job->to_group;
    mining_template_t *last_template = mining_templates[chain_index];
    if (last_template) {
        new_template->chain_task_count = last_template->chain_task_count + 1;
        free_template(last_template);
    } else {
        new_template->chain_task_count = 0;
    }

    mining_templates[chain_index] = new_template;
}

uint32_t next_chain_to_mine()
{
    uint32_t to_mine_index = 0;
    uint64_t least_hash_count = mining_counts[0];
    for (int i = 1; i < chain_nums; i ++) {
        uint64_t i_hash_count = mining_counts[i];
        if (i_hash_count < least_hash_count) {
            to_mine_index = i;
            least_hash_count = i_hash_count;
        }
    }

    return to_mine_index;
}

#endif // ALEPHIUM_TEMPLATE_H
