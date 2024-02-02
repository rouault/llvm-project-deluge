#ifndef DELUGE_TYPE_TABLE_H
#define DELUGE_TYPE_TABLE_H

#include "deluge_runtime.h"
#include "pas_hashtable.h"

PAS_BEGIN_EXTERN_C;

struct deluge_type_table_entry;
typedef struct deluge_type_table_entry deluge_type_table_entry;
typedef const deluge_type_template* deluge_type_table_key;

struct deluge_type_table_entry {
    const deluge_type_template* type_template;
    const deluge_type* type;
};

static inline deluge_type_table_entry deluge_type_table_entry_create_empty(void)
{
    deluge_type_table_entry result;
    result.type_template = NULL;
    result.type = NULL;
    return result;
}

static inline deluge_type_table_entry deluge_type_table_entry_create_deleted(void)
{
    deluge_type_table_entry result;
    result.type_template = NULL;
    result.type = (const deluge_type*)(uintptr_t)1;
    return result;
}

static inline bool deluge_type_table_entry_is_empty_or_deleted(deluge_type_table_entry entry)
{
    return !entry.type_template;
}

static inline bool deluge_type_table_entry_is_empty(deluge_type_table_entry entry)
{
    return !entry.type_template && !entry.type;
}

static inline bool deluge_type_table_entry_is_deleted(deluge_type_table_entry entry)
{
    return !entry.type_template && entry.type == (const deluge_type*)(uintptr_t)1;
}

static inline const deluge_type_template* deluge_type_table_entry_get_key(deluge_type_table_entry entry)
{
    return entry.type_template;
}

static inline unsigned deluge_type_table_key_get_hash(const deluge_type_template* key)
{
    return deluge_type_template_hash(key);
}

static inline bool deluge_type_table_key_is_equal(const deluge_type_template* a,
                                                  const deluge_type_template* b)
{
    return deluge_type_template_is_equal(a, b);
}

PAS_CREATE_HASHTABLE(deluge_type_table,
                     deluge_type_table_entry,
                     deluge_type_table_key);

PAS_API extern deluge_type_table deluge_type_table_instance;

PAS_END_EXTERN_C;

#endif /* DELUGE_TYPE_TABLE_H */

