#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "xalloc.h"
#include "log.h"

#include "hashtable.h"
#include "hashtable_data.h"
#include "hashtable_support_primenumbers.h"
#include "hashtable_support_index.h"

static const char* TAG = "hashtable/data";

hashtable_data_t* hashtable_data_init(hashtable_bucket_count_t buckets_count, uint16_t cachelines_to_probe) {
    hashtable_bucket_count_t buckets_count_real = 0;

    if (hashtable_support_primenumbers_valid(buckets_count) == false) {
        LOG_E(TAG, "The buckets_count is greater than the maximum allowed value %lu", HASHTABLE_PRIMENUMBERS_MAX);
        return NULL;
    }

    buckets_count = hashtable_support_primenumbers_next(buckets_count);
    buckets_count_real = hashtable_support_index_roundup_to_cacheline_to_probe(buckets_count, cachelines_to_probe);

    size_t hashes_size = sizeof(hashtable_bucket_hash_t) * buckets_count_real;
    size_t keys_values_size = sizeof(hashtable_bucket_key_value_t) * buckets_count_real;

    hashtable_data_t* hashtable_data = (hashtable_data_t*)xalloc(sizeof(hashtable_data_t));

    hashtable_data->buckets_count = buckets_count;
    hashtable_data->buckets_count_real = buckets_count_real;
    hashtable_data->cachelines_to_probe = cachelines_to_probe;
    hashtable_data->hashes = (hashtable_bucket_hash_t*)xalloc_aligned(
            HASHTABLE_CACHELINE_LENGTH, hashes_size);
    hashtable_data->keys_values = (hashtable_bucket_key_value_t*)xalloc_aligned(
            HASHTABLE_CACHELINE_LENGTH, keys_values_size);

    memset((void*)hashtable_data->hashes, 0, hashes_size);
    memset((void*)hashtable_data->keys_values, 0, keys_values_size);

    return hashtable_data;
}

extern inline void hashtable_data_free(volatile hashtable_data_t* hashtable_data) {
    xfree((void*)hashtable_data->hashes);
    xfree((void*)hashtable_data->keys_values);
}
