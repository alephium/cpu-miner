#ifndef ALEPHIUM_CONSTANTS_H
#define ALEPHIUM_CONSTANTS_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

const uint32_t group_nums = 4;
const uint32_t chain_nums = group_nums * group_nums;
const uint32_t parallel_mining_works = chain_nums;
const uint32_t mining_steps = 20000;

#endif // ALEPHIUM_CONSTANTS_H
