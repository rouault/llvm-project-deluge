#ifndef FILC_SLICE_TABLE_H
#define FILC_SLICE_TABLE_H

#include "filc_runtime.h"
#include "pas_hashtable.h"
#include "pas_range.h"

PAS_BEGIN_EXTERN_C;

struct filc_slice_table_entry;
struct filc_slice_table_key;
typedef struct filc_slice_table_entry filc_slice_table_entry;
typedef struct filc_slice_table_key filc_slice_table_key;

typedef filc_slice_table_key filc_slice_table_deep_key;

struct filc_slice_table_key {
    const filc_type* base_type;
    pas_range range;
};

struct filc_slice_table_entry {
    filc_slice_table_key key;
    const filc_type* result_type;
};

static inline filc_slice_table_entry filc_slice_table_entry_create_empty(void)
{
    filc_slice_table_entry result;
    result.key.base_type = NULL;
    result.key.range = pas_range_create_empty();
    result.result_type = NULL;
    return result;
}

static inline filc_slice_table_entry filc_slice_table_entry_create_deleted(void)
{
    filc_slice_table_entry result;
    result.key.base_type = NULL;
    result.key.range = pas_range_create(0, 1);
    result.result_type = NULL;
    return result;
}

static inline bool filc_slice_table_entry_is_empty_or_deleted(filc_slice_table_entry entry)
{
    return !entry.key.base_type;
}

static inline bool filc_slice_table_entry_is_empty(filc_slice_table_entry entry)
{
    return !entry.key.base_type && !entry.key.range.end;
}

static inline bool filc_slice_table_entry_is_deleted(filc_slice_table_entry entry)
{
    return !entry.key.base_type && entry.key.range.end;
}

static inline filc_slice_table_key filc_slice_table_entry_get_key(filc_slice_table_entry entry)
{
    return entry.key;
}

static inline unsigned filc_slice_table_key_get_hash(filc_slice_table_key key)
{
    return pas_hash_ptr(key.base_type) + pas_range_hash(key.range);
}

static inline bool filc_slice_table_key_is_equal(filc_slice_table_key a, filc_slice_table_key b)
{
    return a.base_type == b.base_type && pas_range_is_equal(a.range, b.range);
}

static inline unsigned filc_slice_table_deep_key_get_hash(filc_slice_table_key key)
{
    return filc_type_hash(key.base_type) + pas_range_hash(key.range);
}

static inline bool filc_slice_table_deep_key_is_equal(filc_slice_table_key a, filc_slice_table_key b)
{
    return filc_type_is_equal(a.base_type, b.base_type) && pas_range_is_equal(a.range, b.range);
}

PAS_CREATE_HASHTABLE(filc_slice_table,
                     filc_slice_table_entry,
                     filc_slice_table_key);

PAS_CREATE_HASHTABLE(filc_deep_slice_table,
                     filc_slice_table_entry,
                     filc_slice_table_deep_key);

PAS_API extern filc_deep_slice_table filc_global_slice_table;

PAS_END_EXTERN_C;

#endif /* FILC_SLICE_TABLE_H */

