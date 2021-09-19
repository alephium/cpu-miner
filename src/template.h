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
	uint64_t chain_task_count; // increase this by one everytime the template for the chain is updated
} mining_template_t;

mining_template_t* mining_templates[chain_nums] = {};
uint64_t mining_counts[chain_nums];
uint64_t task_counts[chain_nums];
bool mining_templates_initialized = false;

void update_templates(job_t *job)
{
	mining_template_t *mining_template = malloc(sizeof(mining_template_t));
	ssize_t chain_index = job->from_group * group_nums + job->to_group;
	mining_template->job = job;
	mining_template->chain_task_count = mining_templates[chain_index] ? mining_templates[chain_index]->chain_task_count + 1 : 0;
	mining_templates[chain_index] = mining_template;
}

#endif // ALEPHIUM_TEMPLATE_H
