#ifndef FILC_CAT_TABLE_H
#define FILC_CAT_TABLE_H

#include "filc_runtime.h"
#include "pas_hashtable.h"
#include "pas_range.h"

PAS_BEGIN_EXTERN_C;

struct filc_cat_table_entry;
struct filc_cat_table_key;
typedef struct filc_cat_table_entry filc_cat_table_entry;
typedef struct filc_cat_table_key filc_cat_table_key;

typedef filc_cat_table_key filc_cat_table_deep_key;

struct filc_cat_table_key {
    const filc_type* a;
    size_t a_size;
    const filc_type* b;
    size_t b_size;
};

struct filc_cat_table_entry {
    filc_cat_table_key key;
    const filc_type* result_type;
};

static inline filc_cat_table_entry filc_cat_table_entry_create_empty(void)
{
    filc_cat_table_entry result;
    result.key.a = NULL;
    result.key.a_size = 0;
    result.key.b = NULL;
    result.key.b_size = 0;
    result.result_type = NULL;
    return result;
}

static inline filc_cat_table_entry filc_cat_table_entry_create_deleted(void)
{
    filc_cat_table_entry result;
    result.key.a = NULL;
    result.key.a_size = 1;
    result.key.b = NULL;
    result.key.b_size = 0;
    result.result_type = NULL;
    return result;
}

static inline bool filc_cat_table_entry_is_empty_or_deleted(filc_cat_table_entry entry)
{
    return !entry.key.a;
}

static inline bool filc_cat_table_entry_is_empty(filc_cat_table_entry entry)
{
    return !entry.key.a && !entry.key.a_size;
}

static inline bool filc_cat_table_entry_is_deleted(filc_cat_table_entry entry)
{
    return !entry.key.a && entry.key.a_size;
}

static inline filc_cat_table_key filc_cat_table_entry_get_key(filc_cat_table_entry entry)
{
    return entry.key;
}

static inline unsigned filc_cat_table_key_get_hash(filc_cat_table_key key)
{
    return pas_hash_ptr(key.a) + pas_hash_ptr(key.b) * 666 + key.a_size + key.b_size * 42;
}

static inline bool filc_cat_table_key_is_equal(filc_cat_table_key a, filc_cat_table_key b)
{
    return a.a == b.a
        && a.a_size == b.a_size
        && a.b == b.b
        && a.b_size == b.b_size;
}

static inline unsigned filc_cat_table_deep_key_get_hash(filc_cat_table_key key)
{
    return filc_type_hash(key.a) + filc_type_hash(key.b) * 42 + key.a_size + key.b_size * 666;
}

static inline bool filc_cat_table_deep_key_is_equal(filc_cat_table_key a, filc_cat_table_key b)
{
    return filc_type_is_equal(a.a, b.a)
        && a.a_size == b.a_size
        && filc_type_is_equal(a.b, b.b)
        && a.b_size == b.b_size;
}

PAS_CREATE_HASHTABLE(filc_cat_table,
                     filc_cat_table_entry,
                     filc_cat_table_key);

PAS_CREATE_HASHTABLE(filc_deep_cat_table,
                     filc_cat_table_entry,
                     filc_cat_table_deep_key);

PAS_API extern filc_deep_cat_table filc_global_cat_table;

PAS_END_EXTERN_C;

#endif /* FILC_CAT_TABLE_H */

