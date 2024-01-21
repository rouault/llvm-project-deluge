#ifndef DELUGE_CAT_TABLE_H
#define DELUGE_CAT_TABLE_H

#include "deluge_runtime.h"
#include "pas_hashtable.h"
#include "pas_range.h"

PAS_BEGIN_EXTERN_C;

struct deluge_cat_table_entry;
struct deluge_cat_table_key;
typedef struct deluge_cat_table_entry deluge_cat_table_entry;
typedef struct deluge_cat_table_key deluge_cat_table_key;

typedef deluge_cat_table_key deluge_cat_table_deep_key;

struct deluge_cat_table_key {
    const deluge_type* a;
    size_t a_size;
    const deluge_type* b;
    size_t b_size;
};

struct deluge_cat_table_entry {
    deluge_cat_table_key key;
    const deluge_type* result_type;
};

static inline deluge_cat_table_entry deluge_cat_table_entry_create_empty(void)
{
    deluge_cat_table_entry result;
    result.key.a = NULL;
    result.key.a_size = 0;
    result.key.b = NULL;
    result.key.b_size = 0;
    result.result_type = NULL;
    return result;
}

static inline deluge_cat_table_entry deluge_cat_table_entry_create_deleted(void)
{
    deluge_cat_table_entry result;
    result.key.a = NULL;
    result.key.a_size = 1;
    result.key.b = NULL;
    result.key.b_size = 0;
    result.result_type = NULL;
    return result;
}

static inline bool deluge_cat_table_entry_is_empty_or_deleted(deluge_cat_table_entry entry)
{
    return !entry.key.a;
}

static inline bool deluge_cat_table_entry_is_empty(deluge_cat_table_entry entry)
{
    return !entry.key.a && !entry.key.a_size;
}

static inline bool deluge_cat_table_entry_is_deleted(deluge_cat_table_entry entry)
{
    return !entry.key.a && entry.key.a_size;
}

static inline deluge_cat_table_key deluge_cat_table_entry_get_key(deluge_cat_table_entry entry)
{
    return entry.key;
}

static inline unsigned deluge_cat_table_key_get_hash(deluge_cat_table_key key)
{
    return pas_hash_ptr(key.a) + pas_hash_ptr(key.b) * 666 + key.a_size + key.b_size * 42;
}

static inline bool deluge_cat_table_key_is_equal(deluge_cat_table_key a, deluge_cat_table_key b)
{
    return a.a == b.a
        && a.a_size == b.a_size
        && a.b == b.b
        && a.b_size == b.b_size;
}

static inline unsigned deluge_cat_table_deep_key_get_hash(deluge_cat_table_key key)
{
    return deluge_type_hash(key.a) + deluge_type_hash(key.b) * 42 + key.a_size + key.b_size * 666;
}

static inline bool deluge_cat_table_deep_key_is_equal(deluge_cat_table_key a, deluge_cat_table_key b)
{
    return deluge_type_is_equal(a.a, b.a)
        && a.a_size == b.a_size
        && deluge_type_is_equal(a.b, b.b)
        && a.b_size == b.b_size;
}

PAS_CREATE_HASHTABLE(deluge_cat_table,
                     deluge_cat_table_entry,
                     deluge_cat_table_key);

PAS_CREATE_HASHTABLE(deluge_deep_cat_table,
                     deluge_cat_table_entry,
                     deluge_cat_table_deep_key);

PAS_API extern deluge_deep_cat_table deluge_global_cat_table;

PAS_END_EXTERN_C;

#endif /* DELUGE_CAT_TABLE_H */

