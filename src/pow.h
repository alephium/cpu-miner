#ifndef ALEPHIUM_POW_H
#define ALEPHIUM_POW_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "constants.h"
#include "messages.h"

bool check_target(uint8_t *hash, blob_t *target)
{
    assert(target->len <= 32);

    ssize_t zero_len = 32 - target->len;
    for (ssize_t i = 0; i < zero_len; i++) {
        if (hash[i] != 0) {
            return false;
        }
    }
    uint8_t *non_zero_hash = hash + zero_len;
    uint8_t *target_bytes = target->blob;
    for (ssize_t i = 0; i < target->len; i++) {
        if (non_zero_hash[i] > target_bytes[i]) {
            return false;
        } else if (non_zero_hash[i] < target_bytes[i]) {
            return true;
        }
    }
    return true;
}

bool check_index(uint8_t *hash, uint32_t from_group, uint32_t to_group)
{
	uint8_t big_index = hash[31] % chain_nums;
	return (big_index / group_nums == from_group) && (big_index % group_nums == to_group);
}

bool check_hash(uint8_t *hash, blob_t *target, uint32_t from_group, uint32_t to_group)
{
	return check_target(hash, target) && check_index(hash, from_group, to_group);
}

#endif // ALEPHIUM_POW_H
