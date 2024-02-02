#ifndef DELUGE_HEAP_TABLE_H
#define DELUGE_HEAP_TABLE_H

#include "deluge_runtime.h"
#include "pas_hashtable.h"

PAS_BEGIN_EXTERN_C;

struct deluge_heap_table_entry;
typedef struct deluge_heap_table_entry deluge_heap_table_entry;
typedef const deluge_type* deluge_heap_table_key;

struct deluge_heap_table_entry {
    const deluge_type* type;
    pas_heap_ref* heap;
};

static inline deluge_heap_table_entry deluge_heap_table_entry_create_empty(void)
{
    deluge_heap_table_entry result;
    result.type = NULL;
    result.heap = NULL;
    return result;
}

static inline deluge_heap_table_entry deluge_heap_table_entry_create_deleted(void)
{
    deluge_heap_table_entry result;
    result.type = NULL;
    result.heap = (pas_heap_ref*)(uintptr_t)1;
    return result;
}

static inline bool deluge_heap_table_entry_is_empty_or_deleted(deluge_heap_table_entry entry)
{
    return !entry.type;
}

static inline bool deluge_heap_table_entry_is_empty(deluge_heap_table_entry entry)
{
    return !entry.type && !entry.heap;
}

static inline bool deluge_heap_table_entry_is_deleted(deluge_heap_table_entry entry)
{
    return !entry.type && entry.heap == (pas_heap_ref*)(uintptr_t)1;
}

static inline const deluge_type* deluge_heap_table_entry_get_key(deluge_heap_table_entry entry)
{
    return entry.type;
}

static inline unsigned deluge_heap_table_key_get_hash(const deluge_type* key)
{
    return deluge_type_hash(key);
}

static inline bool deluge_heap_table_key_is_equal(const deluge_type* a, const deluge_type* b)
{
    return deluge_type_is_equal(a, b);
}

PAS_CREATE_HASHTABLE(deluge_heap_table,
                     deluge_heap_table_entry,
                     deluge_heap_table_key);

PAS_API extern deluge_heap_table deluge_normal_heap_table;
PAS_API extern deluge_heap_table deluge_hard_heap_table;

PAS_END_EXTERN_C;

#endif /* DELUGE_HEAP_TABLE_H */

