#ifndef ALEPHIUM_TEMPLATE_H
#define ALEPHIUM_TEMPLATE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
        printf("free template: %llu\n", template->chain_task_count);
        free(template->job);
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

#endif // ALEPHIUM_TEMPLATE_H
