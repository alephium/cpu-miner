#ifndef ALEPHIUM_POW_H
#define ALEPHIUM_POW_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

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

#endif // ALEPHIUM_POW_H
