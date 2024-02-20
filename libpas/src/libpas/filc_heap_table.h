#ifndef FILC_HEAP_TABLE_H
#define FILC_HEAP_TABLE_H

#include "filc_runtime.h"
#include "pas_hashtable.h"

PAS_BEGIN_EXTERN_C;

struct filc_heap_table_entry;
typedef struct filc_heap_table_entry filc_heap_table_entry;
typedef const filc_type* filc_heap_table_key;

struct filc_heap_table_entry {
    const filc_type* type;
    pas_heap_ref* heap;
};

static inline filc_heap_table_entry filc_heap_table_entry_create_empty(void)
{
    filc_heap_table_entry result;
    result.type = NULL;
    result.heap = NULL;
    return result;
}

static inline filc_heap_table_entry filc_heap_table_entry_create_deleted(void)
{
    filc_heap_table_entry result;
    result.type = NULL;
    result.heap = (pas_heap_ref*)(uintptr_t)1;
    return result;
}

static inline bool filc_heap_table_entry_is_empty_or_deleted(filc_heap_table_entry entry)
{
    return !entry.type;
}

static inline bool filc_heap_table_entry_is_empty(filc_heap_table_entry entry)
{
    return !entry.type && !entry.heap;
}

static inline bool filc_heap_table_entry_is_deleted(filc_heap_table_entry entry)
{
    return !entry.type && entry.heap == (pas_heap_ref*)(uintptr_t)1;
}

static inline const filc_type* filc_heap_table_entry_get_key(filc_heap_table_entry entry)
{
    return entry.type;
}

static inline unsigned filc_heap_table_key_get_hash(const filc_type* key)
{
    return filc_type_hash(key);
}

static inline bool filc_heap_table_key_is_equal(const filc_type* a, const filc_type* b)
{
    return filc_type_is_equal(a, b);
}

PAS_CREATE_HASHTABLE(filc_heap_table,
                     filc_heap_table_entry,
                     filc_heap_table_key);

PAS_API extern filc_heap_table filc_normal_heap_table;
PAS_API extern filc_heap_table filc_hard_heap_table;

PAS_END_EXTERN_C;

#endif /* FILC_HEAP_TABLE_H */

