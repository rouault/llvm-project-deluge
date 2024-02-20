#ifndef FILC_TYPE_TABLE_H
#define FILC_TYPE_TABLE_H

#include "filc_runtime.h"
#include "pas_hashtable.h"

PAS_BEGIN_EXTERN_C;

struct filc_type_table_entry;
typedef struct filc_type_table_entry filc_type_table_entry;
typedef const filc_type_template* filc_type_table_key;

struct filc_type_table_entry {
    const filc_type_template* type_template;
    const filc_type* type;
};

static inline filc_type_table_entry filc_type_table_entry_create_empty(void)
{
    filc_type_table_entry result;
    result.type_template = NULL;
    result.type = NULL;
    return result;
}

static inline filc_type_table_entry filc_type_table_entry_create_deleted(void)
{
    filc_type_table_entry result;
    result.type_template = NULL;
    result.type = (const filc_type*)(uintptr_t)1;
    return result;
}

static inline bool filc_type_table_entry_is_empty_or_deleted(filc_type_table_entry entry)
{
    return !entry.type_template;
}

static inline bool filc_type_table_entry_is_empty(filc_type_table_entry entry)
{
    return !entry.type_template && !entry.type;
}

static inline bool filc_type_table_entry_is_deleted(filc_type_table_entry entry)
{
    return !entry.type_template && entry.type == (const filc_type*)(uintptr_t)1;
}

static inline const filc_type_template* filc_type_table_entry_get_key(filc_type_table_entry entry)
{
    return entry.type_template;
}

static inline unsigned filc_type_table_key_get_hash(const filc_type_template* key)
{
    return filc_type_template_hash(key);
}

static inline bool filc_type_table_key_is_equal(const filc_type_template* a,
                                                  const filc_type_template* b)
{
    return filc_type_template_is_equal(a, b);
}

PAS_CREATE_HASHTABLE(filc_type_table,
                     filc_type_table_entry,
                     filc_type_table_key);

PAS_API extern filc_type_table filc_type_table_instance;

PAS_END_EXTERN_C;

#endif /* FILC_TYPE_TABLE_H */

