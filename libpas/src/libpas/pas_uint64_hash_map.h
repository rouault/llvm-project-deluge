#ifndef PAS_UINT64_HASH_MAP_H
#define PAS_UINT64_HASH_MAP_H

#include "pas_hashtable.h"

PAS_BEGIN_EXTERN_C;

struct pas_uint64_hash_map_entry;
typedef struct pas_uint64_hash_map_entry pas_uint64_hash_map_entry;

struct pas_uint64_hash_map_entry {
    bool is_valid;
    uint64_t key;
    uint64_t value;
};

typedef uint64_t pas_uint64_hash_map_key;

static inline pas_uint64_hash_map_entry pas_uint64_hash_map_entry_create_empty(void)
{
    pas_uint64_hash_map_entry result;
    result.is_valid = false;
    result.key = 0;
    result.value = 0;
    return result;
}

static inline pas_uint64_hash_map_entry pas_uint64_hash_map_entry_create_deleted(void)
{
    pas_uint64_hash_map_entry result;
    result.is_valid = false;
    result.key = 1;
    result.value = 0;
    return result;
}

static inline bool pas_uint64_hash_map_entry_is_empty_or_deleted(pas_uint64_hash_map_entry entry)
{
    if (!entry.is_valid) {
        PAS_TESTING_ASSERT(entry.key <= (uint64_t)1);
        PAS_TESTING_ASSERT(!entry.value);
        return true;
    }
    return false;
}

static inline bool pas_uint64_hash_map_entry_is_empty(pas_uint64_hash_map_entry entry)
{
    return !entry.is_valid
        && !entry.key;
}

static inline bool pas_uint64_hash_map_entry_is_deleted(pas_uint64_hash_map_entry entry)
{
    return !entry.is_valid
        && entry.key;
}

static inline uint64_t pas_uint64_hash_map_entry_get_key(pas_uint64_hash_map_entry entry)
{
    return entry.key;
}

static inline unsigned pas_uint64_hash_map_key_get_hash(pas_uint64_hash_map_key key)
{
    return pas_hash64(key);
}

static inline bool pas_uint64_hash_map_key_is_equal(pas_uint64_hash_map_key a,
                                                    pas_uint64_hash_map_key b)
{
    return a == b;
}

PAS_CREATE_HASHTABLE(pas_uint64_hash_map,
                     pas_uint64_hash_map_entry,
                     pas_uint64_hash_map_key);

PAS_END_EXTERN_C;

#endif /* PAS_UINT64_HASH_MAP_H */

