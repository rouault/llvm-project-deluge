#ifndef DELUGE_SLICE_TABLE_H
#define DELUGE_SLICE_TABLE_H

#include "deluge_runtime.h"
#include "pas_hashtable.h"
#include "pas_range.h"

PAS_BEGIN_EXTERN_C;

struct deluge_slice_table_entry;
struct deluge_slice_table_key;
typedef struct deluge_slice_table_entry deluge_slice_table_entry;
typedef struct deluge_slice_table_key deluge_slice_table_key;

typedef deluge_slice_table_key deluge_slice_table_deep_key;

struct deluge_slice_table_key {
    const deluge_type* base_type;
    pas_range range;
};

struct deluge_slice_table_entry {
    deluge_slice_table_key key;
    const deluge_type* result_type;
};

static inline deluge_slice_table_entry deluge_slice_table_entry_create_empty(void)
{
    deluge_slice_table_entry result;
    result.key.base_type = NULL;
    result.key.range = pas_range_create_empty();
    result.result_type = NULL;
    return result;
}

static inline deluge_slice_table_entry deluge_slice_table_entry_create_deleted(void)
{
    deluge_slice_table_entry result;
    result.key.base_type = NULL;
    result.key.range = pas_range_create(0, 1);
    result.result_type = NULL;
    return result;
}

static inline bool deluge_slice_table_entry_is_empty_or_deleted(deluge_slice_table_entry entry)
{
    return !entry.key.base_type;
}

static inline bool deluge_slice_table_entry_is_empty(deluge_slice_table_entry entry)
{
    return !entry.key.base_type && !entry.key.range.end;
}

static inline bool deluge_slice_table_entry_is_deleted(deluge_slice_table_entry entry)
{
    return !entry.key.base_type && entry.key.range.end;
}

static inline deluge_slice_table_key deluge_slice_table_entry_get_key(deluge_slice_table_entry entry)
{
    return entry.key;
}

static inline unsigned deluge_slice_table_key_get_hash(deluge_slice_table_key key)
{
    return pas_hash_ptr(key.base_type) + pas_range_hash(key.range);
}

static inline bool deluge_slice_table_key_is_equal(deluge_slice_table_key a, deluge_slice_table_key b)
{
    return a.base_type == b.base_type && pas_range_is_equal(a.range, b.range);
}

static inline unsigned deluge_slice_table_deep_key_get_hash(deluge_slice_table_key key)
{
    return deluge_type_hash(key.base_type) + pas_range_hash(key.range);
}

static inline bool deluge_slice_table_deep_key_is_equal(deluge_slice_table_key a, deluge_slice_table_key b)
{
    return deluge_type_is_equal(a.base_type, b.base_type) && pas_range_is_equal(a.range, b.range);
}

PAS_CREATE_HASHTABLE(deluge_slice_table,
                     deluge_slice_table_entry,
                     deluge_slice_table_key);

PAS_CREATE_HASHTABLE(deluge_deep_slice_table,
                     deluge_slice_table_entry,
                     deluge_slice_table_deep_key);

PAS_API extern deluge_deep_slice_table deluge_global_slice_table;

PAS_END_EXTERN_C;

#endif /* DELUGE_SLICE_TABLE_H */

