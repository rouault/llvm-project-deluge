#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdfil.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

static const size_t num_threads = 8;
static const size_t repeats = 20000;
static const size_t num_toplevel_keys = 2000;
static const size_t num_inner_keys = 2000;
static const size_t max_nodes_to_add = 20;
static const size_t max_nodes_to_remove = 20;

static const unsigned selection_range = 100;
static const unsigned add_nodes_probability = 60;
static const unsigned remove_top_level_probability = 10;
static const unsigned remove_inner_probability = 30;

#define HASHTABLE_MIN_SIZE 16
#define HASHTABLE_MAX_LOAD 2
#define HASHTABLE_MIN_LOAD 6

#define HASHTABLE_INITIALIZER { \
        .table = NULL, \
        .table_size = 0, \
        .table_mask = 0, \
        .key_count = 0, \
        .deleted_count = 0 \
    }

#define UNUSED __attribute__((unused))

static inline bool is_power_of_2(uintptr_t value)
{
    return value && !(value & (value - 1));
}

#define CREATE_HASHTABLE(name, entry_type, key_type) \
    struct name; \
    struct name##_add_result; \
    typedef struct name name; \
    typedef struct name##_add_result name##_add_result; \
    \
    struct name { \
        entry_type* table; \
        unsigned table_size; \
        unsigned table_mask; \
        unsigned key_count; \
        unsigned deleted_count; \
    }; \
    \
    struct name##_add_result { \
        entry_type* entry; \
        bool is_new_entry; \
    }; \
    \
    UNUSED static inline void name##_construct(name* table) \
    { \
        memset(table, 0, sizeof(name)); \
    } \
    \
    UNUSED static inline void name##_destruct(name* table) \
    { \
        free(table->table); \
    } \
    \
    UNUSED static inline void name##_rehash(name* table, unsigned new_size) \
    { \
        static const bool verbose = false; \
        \
        size_t new_byte_size; \
        size_t old_size; \
        entry_type* old_table; \
        entry_type* new_table; \
        unsigned index; \
        unsigned old_index; \
        unsigned new_table_mask; \
        \
        /* This is a table of large allocations, so the sizes are not going to be large enough */ \
        /* to induce overflow. */ \
        \
        ZASSERT(is_power_of_2(new_size)); \
        \
        new_table_mask = new_size - 1; \
        new_byte_size = (size_t)new_size * sizeof(entry_type); \
        if (verbose) \
            zprintf("Allocating a new table with new_size = %u, new_byte_size = %zu.\n", new_size, new_byte_size); \
        new_table = (entry_type*)malloc(new_byte_size); \
        \
        for (index = new_size; index--;) \
            new_table[index] = entry_type##_create_empty(); \
        \
        old_table = table->table; \
        old_size = table->table_size; \
        \
        for (old_index = 0; old_index < old_size; ++old_index) { \
            entry_type* old_entry; \
            unsigned hash; \
            \
            old_entry = old_table + old_index; \
            if (entry_type##_is_empty_or_deleted(*old_entry)) \
                continue; \
            \
            for (hash = key_type##_get_hash(entry_type##_get_key(*old_entry)); ; ++hash) { \
                unsigned new_index; \
                entry_type* new_entry; \
                \
                new_index = hash & new_table_mask; \
                new_entry = new_table + new_index; \
                if (entry_type##_is_empty_or_deleted(*new_entry)) { \
                    *new_entry = *old_entry; \
                    break; \
                } \
            } \
        } \
        \
        /* We do not need to ensure the ordering of the following stores since in-flux-stash is effective while running this code. */ \
        table->table = new_table; \
        table->table_size = new_size; \
        table->table_mask = new_table_mask; \
        table->deleted_count = 0; \
        \
        free(old_table); \
    } \
    \
    UNUSED static inline void name##_expand(name* table) \
    { \
        unsigned new_size; \
        if (!table->table_size) \
            new_size = HASHTABLE_MIN_SIZE; \
        else if (table->key_count * HASHTABLE_MIN_LOAD < table->table_size * 2) \
            new_size = table->table_size; \
        else \
            new_size = table->table_size * 2; \
        name##_rehash(table, new_size); \
    } \
    \
    UNUSED static inline void name##_shrink(name* table) \
    { \
        name##_rehash(table, table->table_size / 2); \
    } \
    \
    UNUSED static inline entry_type* name##_find(name* hashtable, key_type key) \
    { \
        entry_type* table = hashtable->table; \
        if (!table) \
            return NULL; \
        \
        for (unsigned hash = key_type##_get_hash(key); ; ++hash) { \
            unsigned index; \
            entry_type* entry; \
            \
            index = hash & hashtable->table_mask; \
            entry = table + index; \
            \
            if (entry_type##_is_empty_or_deleted(*entry)) { \
                if (entry_type##_is_deleted(*entry)) \
                    continue; \
                return NULL; \
            } \
            if (key_type##_is_equal(entry_type##_get_key(*entry), key)) \
                return entry; \
        } \
    } \
    UNUSED static inline entry_type name##_get(name* hashtable, key_type key) \
    { \
        entry_type* result; \
        result = name##_find(hashtable, key); \
        if (!result) \
            return entry_type##_create_empty(); \
        return *result; \
    } \
    \
    UNUSED static inline name##_add_result name##_add(name* table, key_type key) \
    { \
        entry_type* entry; \
        entry_type* deleted_entry; \
        unsigned hash; \
        \
        name##_add_result result; \
        \
        if ((table->key_count + table->deleted_count) * HASHTABLE_MAX_LOAD \
            >= table->table_size) \
            name##_expand(table); \
        \
        deleted_entry = NULL; \
        \
        for (hash = key_type##_get_hash(key); ; ++hash) { \
            unsigned index = hash & table->table_mask; \
            entry = table->table + index; \
            if (entry_type##_is_empty(*entry)) \
                break; \
            if (entry_type##_is_deleted(*entry)) { \
                if (!deleted_entry) \
                    deleted_entry = entry; \
                continue; \
            } \
            if (key_type##_is_equal(entry_type##_get_key(*entry), key)) { \
                result.entry = entry; \
                result.is_new_entry = false; \
                return result; \
            } \
        } \
        \
        if (deleted_entry) { \
            table->deleted_count--; \
            entry = deleted_entry; \
        } \
        table->key_count++; \
        \
        result.entry = entry; \
        result.is_new_entry = true; \
        return result; \
    } \
    \
    UNUSED static inline void name##_add_new(name* table, entry_type new_entry) \
    { \
        name##_add_result result = name##_add(table, entry_type##_get_key(new_entry)); \
        ZASSERT(result.is_new_entry); \
        *result.entry = new_entry; \
    } \
    \
    UNUSED static inline bool name##_set(name* table, entry_type new_entry) \
    { \
        name##_add_result result = name##_add(table, entry_type##_get_key(new_entry)); \
        *result.entry = new_entry; \
        return result.is_new_entry; \
    } \
    \
    UNUSED static inline bool name##_take_and_return_if_taken( \
        name* table, key_type key, entry_type* result) \
    { \
        entry_type* entry_ptr; \
        entry_type entry; \
        \
        entry_ptr = name##_find(table, key); \
        if (!entry_ptr) { \
            if (result) \
                *result = entry_type##_create_empty(); \
            return false; \
        } \
        \
        entry = *entry_ptr; \
        \
        *entry_ptr = entry_type##_create_deleted(); \
        \
        table->deleted_count++; \
        table->key_count--; \
        if (table->key_count * HASHTABLE_MIN_LOAD < table->table_size \
            && table->table_size > HASHTABLE_MIN_SIZE) \
            name##_shrink(table); \
        \
        if (result) \
            *result = entry; \
        return true; \
    } \
    \
    UNUSED static inline entry_type name##_take(name* table, key_type key) \
    { \
        entry_type result; \
        name##_take_and_return_if_taken(table, key, &result); \
        return result; \
    } \
    \
    UNUSED static inline bool name##_remove(name* table, key_type key) \
    { \
        return name##_take_and_return_if_taken(table, key, NULL); \
    } \
    \
    UNUSED static inline void name##_delete(name* table, key_type key) \
    { \
        bool result; \
        result = name##_remove(table, key); \
        ZASSERT(result); \
    } \
    \
    typedef bool (*name##_for_each_entry_callback)(entry_type* entry, void* arg); \
    \
    UNUSED static inline bool name##_for_each_entry(name* table, \
                                                    name##_for_each_entry_callback callback, \
                                                    void* arg) \
    { \
        unsigned index; \
        \
        for (index = 0; index < table->table_size; ++index) { \
            entry_type* entry; \
            \
            entry = table->table + index; \
            if (entry_type##_is_empty_or_deleted(*entry)) \
                continue; \
            \
            if (!callback(entry, arg)) \
                return false; \
        } \
        \
        return true; \
    } \
    \
    UNUSED static inline size_t name##_size(name* table) \
    { \
        return table->key_count; \
    } \
    \
    UNUSED static inline size_t name##_entry_index_end(name* table) \
    { \
        return table->table_size; \
    } \
    \
    UNUSED static inline entry_type* name##_entry_at_index(name* table, \
                                                           size_t index) \
    { \
        return table->table + index; \
    } \
    \
    struct dummy

struct string_hash_map_entry;
typedef struct string_hash_map_entry string_hash_map_entry;

struct string_hash_map_entry {
    char* key;
    void* value;
};

typedef char* string_hash_map_key;

static inline string_hash_map_entry string_hash_map_entry_create_empty(void)
{
    string_hash_map_entry result;
    result.key = (void*)UINTPTR_MAX;
    result.value = NULL;
    return result;
}

static inline string_hash_map_entry string_hash_map_entry_create_deleted(void)
{
    string_hash_map_entry result;
    result.key = (void*)UINTPTR_MAX;
    result.value = (void*)(uintptr_t)1;
    return result;
}

static inline bool string_hash_map_entry_is_empty_or_deleted(string_hash_map_entry entry)
{
    if (entry.key == (void*)UINTPTR_MAX) {
        ZASSERT(entry.value <= (void*)(uintptr_t)1);
        return true;
    }
    return false;
}

static inline bool string_hash_map_entry_is_empty(string_hash_map_entry entry)
{
    return entry.key == (void*)UINTPTR_MAX
        && !entry.value;
}

static inline bool string_hash_map_entry_is_deleted(string_hash_map_entry entry)
{
    return entry.key == (void*)UINTPTR_MAX
        && entry.value == (void*)(uintptr_t)1;
}

static inline char* string_hash_map_entry_get_key(string_hash_map_entry entry)
{
    return entry.key;
}

static inline unsigned string_hash_map_key_get_hash(string_hash_map_key key)
{
    size_t index;
    unsigned result = 666;
    for (index = strlen(key); index--;)
        result += key[index];
    return result;
}

static inline bool string_hash_map_key_is_equal(string_hash_map_key a,
                                                string_hash_map_key b)
{
    return !strcmp(a, b);
}

CREATE_HASHTABLE(string_hash_map,
                 string_hash_map_entry,
                 string_hash_map_key);

unsigned fast_random_state = 42;

static inline uint32_t xorshift32(uint32_t state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static inline unsigned get_fast_random(unsigned upper_bound)
{
    unsigned rand_value;

    if (!upper_bound)
        upper_bound = UINT32_MAX;

    fast_random_state = xorshift32(fast_random_state);
    rand_value = fast_random_state % upper_bound;

    return rand_value;
}

static pthread_mutex_t global_lock;
static string_hash_map hash_map;
static unsigned num_threads_done;

typedef struct {
    pthread_mutex_t lock;
    string_hash_map map;
} inner_map;

static void add_nodes(void)
{
    char* key;
    asprintf(&key, "key %u", get_fast_random(num_toplevel_keys));
    pthread_mutex_lock(&global_lock);
    inner_map* inner;
    string_hash_map_add_result add_result = string_hash_map_add(&hash_map, key);
    if (add_result.is_new_entry) {
        add_result.entry->key = key;
        inner = malloc(sizeof(inner_map));
        add_result.entry->value = inner;
        pthread_mutex_init(&inner->lock, NULL);
        string_hash_map_construct(&inner->map);
    } else
        inner = add_result.entry->value;
    pthread_mutex_unlock(&global_lock);
    size_t count;
    for (count = get_fast_random(max_nodes_to_add); count--;) {
        char* subkey;
        char* value;
        asprintf(&subkey, "subkey %u", get_fast_random(num_inner_keys));
        asprintf(&value, "subvalue %u", get_fast_random(0));
        string_hash_map_entry entry;
        entry.key = subkey;
        entry.value = value;
        pthread_mutex_lock(&inner->lock);
        string_hash_map_set(&inner->map, entry);
        pthread_mutex_unlock(&inner->lock);
    }
}

static void remove_top_level(void)
{
    char* key;
    asprintf(&key, "key %u", get_fast_random(num_toplevel_keys));
    pthread_mutex_lock(&global_lock);
    string_hash_map_remove(&hash_map, key);
    pthread_mutex_unlock(&global_lock);
}

static void remove_inner(void)
{
    char* key;
    asprintf(&key, "key %u", get_fast_random(num_toplevel_keys));
    pthread_mutex_lock(&global_lock);
    inner_map* inner = string_hash_map_get(&hash_map, key).value;
    pthread_mutex_unlock(&global_lock);
    if (!inner)
        return;
    size_t count;
    for (count = get_fast_random(max_nodes_to_remove); count--;) {
        char* subkey;
        asprintf(&subkey, "subkey %u", get_fast_random(num_inner_keys));
        pthread_mutex_lock(&inner->lock);
        string_hash_map_remove(&inner->map, subkey);
        pthread_mutex_unlock(&inner->lock);
    }
}

static void* thread_main(void* arg)
{
    size_t repeat;
    for (repeat = repeats; repeat--;) {
        unsigned selection = get_fast_random(selection_range);
        if (selection < add_nodes_probability) {
            add_nodes();
            continue;
        }
        selection -= add_nodes_probability;
        if (selection < remove_top_level_probability) {
            remove_top_level();
            continue;
        }
        selection -= remove_top_level_probability;
        ZASSERT(selection < remove_inner_probability);
        remove_inner();
    }
    pthread_mutex_lock(&global_lock);
    num_threads_done++;
    pthread_mutex_unlock(&global_lock);
    return NULL;
}

int main()
{
    pthread_mutex_init(&global_lock, NULL);
    string_hash_map_construct(&hash_map);

    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    size_t index;
    for (index = num_threads; index--;)
        pthread_create(threads + index, NULL, thread_main, NULL);

    while (num_threads_done < num_threads) {
        usleep(1000000);
        pthread_mutex_lock(&global_lock);
        size_t num_toplevel_entries = 0;
        size_t num_inner_entries = 0;
        for (index = hash_map.table_size; index--;) {
            string_hash_map_entry entry = hash_map.table[index];
            if (string_hash_map_entry_is_empty_or_deleted(entry))
                continue;
            num_toplevel_entries++;
            num_inner_entries += ((inner_map*)entry.value)->map.key_count;
        }
        pthread_mutex_unlock(&global_lock);
        zprintf("num_toplevel_entries = %zu, num_inner_entries = %zu\n",
                num_toplevel_entries, num_inner_entries);
    }

    printf("Sukces!\n");
    return 0;
}



