#include "pas_config.h"

#if LIBPAS_ENABLED

#include "deluge_runtime.h"

#if PAS_ENABLE_DELUGE

#include "deluge_cat_table.h"
#include "deluge_hard_heap_config.h"
#include "deluge_heap_config.h"
#include "deluge_heap_innards.h"
#include "deluge_heap_table.h"
#include "deluge_slice_table.h"
#include "deluge_type_table.h"
#include "pas_deallocate.h"
#include "pas_get_allocation_size.h"
#include "pas_hashtable.h"
#include "pas_string_stream.h"
#include "pas_try_allocate.h"
#include "pas_try_allocate_array.h"
#include "pas_try_allocate_intrinsic.h"
#include "pas_try_reallocate.h"
#include "pas_utils.h"
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <sys/select.h>

const deluge_type_template deluge_int_type_template = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { DELUGE_WORD_TYPE_INT }
};

const deluge_type deluge_int_type = {
    .index = DELUGE_INT_TYPE_INDEX,
    .size = 1,
    .alignment = 1,
    .num_words = 1,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = { DELUGE_WORD_TYPE_INT }
};

const deluge_type deluge_one_ptr_type = {
    .index = DELUGE_ONE_PTR_TYPE_INDEX,
    .size = sizeof(deluge_ptr),
    .alignment = alignof(deluge_ptr),
    .num_words = sizeof(deluge_ptr) / DELUGE_WORD_SIZE,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = { DELUGE_WORD_TYPES_PTR }
};

const deluge_type deluge_function_type = {
    .index = DELUGE_FUNCTION_TYPE_INDEX,
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

const deluge_type deluge_type_type = {
    .index = DELUGE_TYPE_TYPE_INDEX,
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

const deluge_type** deluge_type_array = NULL;
unsigned deluge_type_array_size = 0;
unsigned deluge_type_array_capacity = 0;

deluge_type_table deluge_type_table_instance = PAS_HASHTABLE_INITIALIZER;
deluge_heap_table deluge_normal_heap_table = PAS_HASHTABLE_INITIALIZER;
deluge_heap_table deluge_hard_heap_table = PAS_HASHTABLE_INITIALIZER;
deluge_deep_slice_table deluge_global_slice_table = PAS_HASHTABLE_INITIALIZER;
deluge_deep_cat_table deluge_global_cat_table = PAS_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable deluge_fast_type_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable deluge_fast_heap_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable deluge_fast_hard_heap_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;

PAS_DEFINE_LOCK(deluge_type);
PAS_DEFINE_LOCK(deluge_type_ops);
PAS_DEFINE_LOCK(deluge_global_initialization);

static void* allocate_utility_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return deluge_allocate_utility(size);
}

static void* allocate_int_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return deluge_allocate_int(size, 1);
}

static void deallocate_for_allocation_config(
    void* ptr, size_t size, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(size);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    deluge_deallocate(ptr);
}

static void initialize_utility_allocation_config(pas_allocation_config* allocation_config)
{
    allocation_config->allocate = allocate_utility_for_allocation_config;
    allocation_config->deallocate = deallocate_for_allocation_config;
    allocation_config->arg = NULL;
}

static void initialize_int_allocation_config(pas_allocation_config* allocation_config)
{
    allocation_config->allocate = allocate_int_for_allocation_config;
    allocation_config->deallocate = deallocate_for_allocation_config;
    allocation_config->arg = NULL;
}

typedef struct {
    deluge_slice_table slice_table;
    deluge_cat_table cat_table;
} type_tables;

static pthread_key_t type_tables_key;

static void type_tables_destroy(void* value)
{
    type_tables* tables;
    pas_allocation_config allocation_config;

    tables = (type_tables*)value;

    if (!tables)
        return;

    initialize_utility_allocation_config(&allocation_config);
    deluge_slice_table_destruct(&tables->slice_table, &allocation_config);
    deluge_cat_table_destruct(&tables->cat_table, &allocation_config);
    deluge_deallocate(tables);
}

struct musl_passwd {
    deluge_ptr pw_name;
    deluge_ptr pw_passwd;
    unsigned pw_uid;
    unsigned pw_gid;
    deluge_ptr pw_gecos;
    deluge_ptr pw_dir;
    deluge_ptr pw_shell;
};

static const deluge_type musl_passwd_type = {
    .index = DELUGE_MUSL_PASSWD_TYPE_INDEX,
    .size = sizeof(struct musl_passwd),
    .alignment = alignof(struct musl_passwd),
    .num_words = sizeof(struct musl_passwd) / DELUGE_WORD_SIZE,
    .u = {
        .trailing_array = NULL
    },
    .word_types = {
        DELUGE_WORD_TYPES_PTR, /* pw_name */
        DELUGE_WORD_TYPES_PTR, /* pw_passwd */
        DELUGE_WORD_TYPE_INT, /* pw_uid, pw_gid */
        DELUGE_WORD_TYPES_PTR, /* pw_gecos */
        DELUGE_WORD_TYPES_PTR, /* pw_dir */
        DELUGE_WORD_TYPES_PTR /* pw_shell */
    }
};

static pas_heap_ref musl_passwd_heap = {
    .type = (const pas_heap_type*)&musl_passwd_type,
    .heap = NULL,
    .allocator_index = 0
};

static void musl_passwd_free_guts(struct musl_passwd* musl_passwd)
{
    deluge_deallocate_safe(deluge_ptr_load(&musl_passwd->pw_name));
    deluge_deallocate_safe(deluge_ptr_load(&musl_passwd->pw_passwd));
    deluge_deallocate_safe(deluge_ptr_load(&musl_passwd->pw_gecos));
    deluge_deallocate_safe(deluge_ptr_load(&musl_passwd->pw_dir));
    deluge_deallocate_safe(deluge_ptr_load(&musl_passwd->pw_shell));
}

static void musl_passwd_threadlocal_destructor(void* ptr)
{
    struct musl_passwd* musl_passwd = (struct musl_passwd*)ptr;
    musl_passwd_free_guts(musl_passwd);
    deluge_deallocate(musl_passwd);
}

static pthread_key_t musl_passwd_threadlocal_key;

struct musl_sigset {
    unsigned long bits[128 / sizeof(long)];
};

struct musl_sigaction {
    deluge_ptr sa_handler_ish;
    struct musl_sigset sa_mask;
    int sa_flags;
    deluge_ptr sa_restorer; /* ignored */
};

static const deluge_type musl_sigaction_type = {
    .index = DELUGE_MUSL_SIGACTION_TYPE_INDEX,
    .size = sizeof(struct musl_sigaction),
    .alignment = alignof(struct musl_sigaction),
    .num_words = sizeof(struct musl_sigaction) / DELUGE_WORD_SIZE,
    .u = {
        .trailing_array = NULL
    },
    .word_types = {
        DELUGE_WORD_TYPES_PTR, /* sa_handler_ish */
        DELUGE_WORD_TYPE_INT, /* sa_mask */
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_INT, /* sa_flags */
        DELUGE_WORD_TYPES_PTR /* sa_restorer */
    }
};

#define DEFINE_RUNTIME_CONFIG(name, type, fresh_memory_constructor)     \
    static void name ## _initialize_fresh_memory(void* begin, void* end) \
    { \
        PAS_TESTING_ASSERT(((char*)end - (char*)begin) >= (ptrdiff_t)sizeof(type)); \
        fresh_memory_constructor(begin); \
    } \
    \
    static pas_basic_heap_runtime_config name = { \
        .base = { \
            .sharing_mode = pas_do_not_share_pages, \
            .statically_allocated = false, \
            .is_part_of_heap = true, \
            .is_flex = false, \
            .directory_size_bound_for_partial_views = 0, \
            .directory_size_bound_for_baseline_allocators = PAS_TYPED_BOUND_FOR_BASELINE_ALLOCATORS, \
            .directory_size_bound_for_no_view_cache = PAS_TYPED_BOUND_FOR_NO_VIEW_CACHE, \
            .max_segregated_object_size = PAS_TYPED_MAX_SEGREGATED_OBJECT_SIZE, \
            .max_bitfit_object_size = 0, \
            .view_cache_capacity_for_object_size = pas_heap_runtime_config_zero_view_cache_capacity, \
            .initialize_fresh_memory = name ## _initialize_fresh_memory \
        }, \
        .page_caches = &deluge_page_caches \
    }

typedef struct {
    pas_lock lock;
    void (*destructor)(DELUDED_SIGNATURE);
    pthread_key_t key;
    uint64_t version;
} thread_specific;

static void thread_specific_construct(thread_specific* specific)
{
    pas_lock_construct(&specific->lock);
}

DEFINE_RUNTIME_CONFIG(thread_specific_runtime_config, thread_specific, thread_specific_construct);

static const deluge_type thread_specific_type = {
    .index = DELUGE_THREAD_SPECIFIC_TYPE_INDEX,
    .size = sizeof(thread_specific),
    .alignment = alignof(thread_specific),
    .num_words = 0,
    .u = {
        .runtime_config = &thread_specific_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref thread_specific_heap = {
    .type = (const pas_heap_type*)&thread_specific_type,
    .heap = NULL,
    .allocator_index = 0
};

static void rwlock_construct(pthread_rwlock_t* rwlock)
{
    int result = pthread_rwlock_init(rwlock, NULL);
    PAS_ASSERT(!result);
}

DEFINE_RUNTIME_CONFIG(rwlock_runtime_config, pthread_rwlock_t, rwlock_construct);

static const deluge_type rwlock_type = {
    .index = DELUGE_RWLOCK_TYPE_INDEX,
    .size = sizeof(pthread_rwlock_t),
    .alignment = alignof(pthread_rwlock_t),
    .num_words = 0,
    .u = {
        .runtime_config = &rwlock_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref rwlock_heap = {
    .type = (const pas_heap_type*)&rwlock_type,
    .heap = NULL,
    .allocator_index = 0
};

static void mutex_construct(pthread_mutex_t* mutex)
{
    int result = pthread_mutex_init(mutex, NULL);
    PAS_ASSERT(!result);
}

DEFINE_RUNTIME_CONFIG(mutex_runtime_config, pthread_mutex_t, mutex_construct);

static const deluge_type mutex_type = {
    .index = DELUGE_MUTEX_TYPE_INDEX,
    .size = sizeof(pthread_mutex_t),
    .alignment = alignof(pthread_mutex_t),
    .num_words = 0,
    .u = {
        .runtime_config = &mutex_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref mutex_heap = {
    .type = (const pas_heap_type*)&mutex_type,
    .heap = NULL,
    .allocator_index = 0
};

typedef struct {
    pas_lock lock;
    pthread_t thread; /* this becomes NULL the moment that we join or detach. */
    bool is_running;
    bool is_joining;
    void (*callback)(DELUDED_SIGNATURE);
    deluge_ptr arg_ptr;
    deluge_ptr result_ptr;
} zthread;

static void zthread_construct(zthread* thread)
{
    pas_lock_construct(&thread->lock);
    thread->thread = NULL;
}

DEFINE_RUNTIME_CONFIG(zthread_runtime_config, zthread, zthread_construct);

static const deluge_type zthread_type = {
    .index = DELUGE_THREAD_TYPE_INDEX,
    .size = sizeof(zthread),
    .alignment = alignof(zthread),
    .num_words = 0,
    .u = {
        .runtime_config = &zthread_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref zthread_heap = {
    .type = (const pas_heap_type*)&zthread_type,
    .heap = NULL,
    .allocator_index = 0
};

static bool should_destroy_thread(zthread* thread)
{
    return !thread->is_running && !thread->thread;
}

static void destroy_thread_if_appropriate(zthread* thread)
{
    if (should_destroy_thread(thread))
        deluge_deallocate(thread);
}

static void zthread_destruct(void* zthread_arg)
{
    zthread* thread;

    thread = (zthread*)zthread_arg;

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(thread->is_running);
    thread->is_running = false;

    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock); /* We can do this safely because it's a no-page-sharing isoheap. */
}

static pthread_key_t zthread_key;

static void initialize_impl(void)
{
    deluge_type_array = (const deluge_type**)deluge_allocate_utility(
        sizeof(const deluge_type*) * DELUGE_TYPE_ARRAY_INITIAL_CAPACITY);
    deluge_type_array_size = DELUGE_TYPE_ARRAY_INITIAL_SIZE;
    deluge_type_array_capacity = DELUGE_TYPE_ARRAY_INITIAL_CAPACITY;

    deluge_type_array[DELUGE_INVALID_TYPE_INDEX] = 0;
    deluge_type_array[DELUGE_INT_TYPE_INDEX] = &deluge_int_type;
    deluge_type_array[DELUGE_ONE_PTR_TYPE_INDEX] = &deluge_one_ptr_type;
    deluge_type_array[DELUGE_FUNCTION_TYPE_INDEX] = &deluge_function_type;
    deluge_type_array[DELUGE_TYPE_TYPE_INDEX] = &deluge_type_type;
    deluge_type_array[DELUGE_MUSL_PASSWD_TYPE_INDEX] = &musl_passwd_type;
    deluge_type_array[DELUGE_MUSL_SIGACTION_TYPE_INDEX] = &musl_sigaction_type;
    deluge_type_array[DELUGE_THREAD_SPECIFIC_TYPE_INDEX] = &thread_specific_type;
    deluge_type_array[DELUGE_RWLOCK_TYPE_INDEX] = &rwlock_type;
    deluge_type_array[DELUGE_MUTEX_TYPE_INDEX] = &mutex_type;
    deluge_type_array[DELUGE_THREAD_TYPE_INDEX] = &zthread_type;
    
    pthread_key_create(&type_tables_key, type_tables_destroy);
    pthread_key_create(&musl_passwd_threadlocal_key, musl_passwd_threadlocal_destructor);
    
    zthread* result = (zthread*)deluge_allocate_one(&zthread_heap);
    PAS_ASSERT(!result->thread);
    result->thread = pthread_self();
    result->is_running = true;
    PAS_ASSERT(!should_destroy_thread(result));
    PAS_ASSERT(!pthread_key_create(&zthread_key, zthread_destruct));
    PAS_ASSERT(!pthread_setspecific(zthread_key, result));
}

static pas_system_once global_once = PAS_SYSTEM_ONCE_INIT;

void deluge_initialize(void)
{
    pas_system_once_run(&global_once, initialize_impl);
}

static void type_template_dump_impl(const deluge_type_template* type, pas_stream* stream)
{
    static const bool dump_ptr = false;

    size_t index;

    if (dump_ptr)
        pas_stream_printf(stream, "@%p", type);

    if (!type) {
        pas_stream_printf(stream, "{null}");
        return;
    }

    if (type == &deluge_int_type_template) {
        pas_stream_printf(stream, "{int}");
        return;
    }

    pas_stream_printf(stream, "{%zu,%zu,", type->size, type->alignment);
    for (index = 0; index < deluge_type_template_num_words(type); ++index)
        deluge_word_type_dump(type->word_types[index], stream);
    if (type->trailing_array) {
        pas_stream_printf(stream, ",trail");
        type_template_dump_impl(type->trailing_array, stream);
    }
    pas_stream_printf(stream, "}");
}

void deluge_type_template_dump(const deluge_type_template* type, pas_stream* stream)
{
    static const bool dump_only_ptr = false;
    
    if (dump_only_ptr) {
        pas_stream_printf(stream, "%p", type);
        return;
    }

    pas_stream_printf(stream, "type_template");
    type_template_dump_impl(type, stream);
}

static const deluge_type* get_type_impl(const deluge_type_template* type_template,
                                        pas_lock_hold_mode type_lock_hold_mode);

static const deluge_type* get_type_slow_impl(const deluge_type_template* type_template)
{
    static const bool verbose = false;
    
    pas_allocation_config allocation_config;
    deluge_type_table_add_result add_result;

    PAS_ASSERT(type_template->size);
    PAS_ASSERT(type_template->alignment);
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = deluge_type_table_add(
        &deluge_type_table_instance, type_template, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        deluge_type* result_type;
        size_t index;
        size_t total_size;

        PAS_ASSERT(!pas_mul_uintptr_overflow(
                       sizeof(deluge_word_type), deluge_type_template_num_words(type_template),
                       &total_size));
        PAS_ASSERT(!pas_add_uintptr_overflow(
                       total_size, PAS_OFFSETOF(deluge_type, word_types),
                       &total_size));

        if (deluge_type_array_size >= deluge_type_array_capacity) {
            const deluge_type** new_type_array;
            unsigned new_capacity;
            
            PAS_ASSERT(deluge_type_array_size == deluge_type_array_capacity);
            PAS_ASSERT(deluge_type_array_size <= DELUGE_TYPE_ARRAY_MAX_SIZE);
            if (deluge_type_array_size == DELUGE_TYPE_ARRAY_MAX_SIZE)
                pas_panic("too many deluge types.\n");

            if (deluge_type_array_capacity >= DELUGE_TYPE_ARRAY_MAX_SIZE / 2)
                new_capacity = DELUGE_TYPE_ARRAY_MAX_SIZE;
            else
                new_capacity = deluge_type_array_capacity * 2;
            new_type_array = (const deluge_type**)deluge_allocate_utility(
                sizeof(deluge_type*) * new_capacity);

            memcpy(new_type_array, deluge_type_array, deluge_type_array_size * sizeof(deluge_type*));

            pas_store_store_fence();
            
            deluge_type_array = new_type_array;
            deluge_type_array_capacity = new_capacity;

            /* Intentionally leak the old array so that there's no race. */
        }

        result_type = deluge_allocate_utility(total_size);
        result_type->index = deluge_type_array_size++;
        result_type->size = type_template->size;
        result_type->alignment = type_template->alignment;
        result_type->num_words = deluge_type_template_num_words(type_template);
        result_type->u.trailing_array = NULL;
        memcpy(result_type->word_types, type_template->word_types,
               deluge_type_template_num_words(type_template) * sizeof(deluge_word_type));
        
        add_result.entry->type_template = type_template;
        add_result.entry->type = result_type;

        if (type_template->trailing_array) {
            /* Fill in the trailing_array after we've put the hashtable in an OK state (this entry is
               filled in with something). */
            result_type->u.trailing_array = get_type_impl(type_template->trailing_array, pas_lock_is_held);
        }

        deluge_validate_type(add_result.entry->type, NULL);

        pas_store_store_fence();

        deluge_type_array[result_type->index] = result_type;

        if (verbose)
            pas_log("Created new type %p with index %u\n", result_type, result_type->index);
    }

    return add_result.entry->type;
}

static unsigned fast_type_template_hash(const void* type, void* arg)
{
    PAS_ASSERT(!arg);
    return pas_hash_ptr(type);
}

static const deluge_type* get_type_impl(const deluge_type_template* type_template,
                                        pas_lock_hold_mode type_lock_hold_mode)
{
    static const bool verbose = false;
    const deluge_type* result;
    if (verbose) {
        pas_log("Getting type for ");
        deluge_type_template_dump(type_template, &pas_log_stream.base);
        pas_log("\n");
    }
    PAS_ASSERT(type_template);
    if (type_template == &deluge_int_type_template)
        return &deluge_int_type;
    result = (const deluge_type*)pas_lock_free_read_ptr_ptr_hashtable_find(
        &deluge_fast_type_table, fast_type_template_hash, NULL, type_template);
    if (result)
        return result;
    deluge_type_lock_lock_conditionally(type_lock_hold_mode);
    result = get_type_slow_impl(type_template);
    deluge_type_lock_unlock_conditionally(type_lock_hold_mode);
    pas_heap_lock_lock();
    pas_lock_free_read_ptr_ptr_hashtable_set(
        &deluge_fast_type_table, fast_type_template_hash, NULL, type_template, result,
        pas_lock_free_read_ptr_ptr_hashtable_set_maybe_existing);
    pas_heap_lock_unlock();
    return result;
}

const deluge_type* deluge_get_type(const deluge_type_template* type_template)
{
    return get_type_impl(type_template, pas_lock_is_not_held);
}

void deluge_ptr_dump(deluge_ptr ptr, pas_stream* stream)
{
    pas_stream_printf(
        stream, "%p,%p,%p,", deluge_ptr_ptr(ptr), deluge_ptr_lower(ptr), deluge_ptr_upper(ptr));
    deluge_type_dump(deluge_ptr_type(ptr), stream);
}

static char* ptr_to_new_string_impl(deluge_ptr ptr, pas_allocation_config* allocation_config)
{
    pas_string_stream stream;
    pas_string_stream_construct(&stream, allocation_config);
    deluge_ptr_dump(ptr, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* deluge_ptr_to_new_string(deluge_ptr ptr)
{
    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    return ptr_to_new_string_impl(ptr, &allocation_config);
}

void deluge_validate_type(const deluge_type* type, const deluge_origin* origin)
{
    if (deluge_type_is_equal(type, &deluge_int_type))
        DELUGE_ASSERT(type == &deluge_int_type, origin);
    if (deluge_type_is_equal(type, &deluge_function_type))
        DELUGE_ASSERT(type == &deluge_function_type, origin);
    if (deluge_type_is_equal(type, &deluge_type_type))
        DELUGE_ASSERT(type == &deluge_type_type, origin);
    if (type->size) {
        DELUGE_ASSERT(type->alignment, origin);
        DELUGE_ASSERT(pas_is_power_of_2(type->alignment), origin);
        if (!deluge_type_get_trailing_array(type))
            DELUGE_ASSERT(!(type->size % type->alignment), origin);
    } else
        DELUGE_ASSERT(!type->alignment, origin);
    if (type != &deluge_int_type) {
        if (type->num_words) {
            size_t index;
            if (type->u.trailing_array == &deluge_int_type) {
                /* You could have an int trailing array starting at an offset that is not a multiple of 16.
                   That's fine so long as the word in which it starts is itself an int. The math inside of
                   deluge_type_get_word_type() ends up being wrong in a benign way in that case; for any
                   byte in the trailing array it might return the int word from the tail end of the base
                   type or the int word from the trailing type, and it's arbitrary which you get, and it
                   doesn't matter. These assertions are all about making sure you get int either way,
                   which is right. */
                DELUGE_ASSERT(pas_round_up_to_power_of_2(type->size, DELUGE_WORD_SIZE) / DELUGE_WORD_SIZE
                              == type->num_words, origin);
                DELUGE_ASSERT(
                    type->size == type->num_words * DELUGE_WORD_SIZE ||
                    type->word_types[type->num_words - 1] == DELUGE_WORD_TYPE_INT,
                    origin);
            } else
                DELUGE_ASSERT(type->size == type->num_words * DELUGE_WORD_SIZE, origin);
            if (type->u.trailing_array) {
                DELUGE_ASSERT(type->u.trailing_array->num_words, origin);
                DELUGE_ASSERT(!type->u.trailing_array->u.trailing_array, origin);
                DELUGE_ASSERT(type->u.trailing_array->size, origin);
                DELUGE_ASSERT(type->u.trailing_array->alignment <= type->alignment, origin);
                deluge_validate_type(type->u.trailing_array, origin);
            }
            for (index = type->num_words; index--;) {
                DELUGE_ASSERT(
                    (type->word_types[index] >= DELUGE_WORD_TYPE_OFF_LIMITS) &&
                    (type->word_types[index] <= DELUGE_WORD_TYPE_PTR_CAPABILITY),
                    origin);
            }
        }
    }
}

bool deluge_type_template_is_equal(const deluge_type_template* a, const deluge_type_template* b)
{
    size_t index;
    if (a == b)
        return true;
    if (a->size != b->size)
        return false;
    if (a->alignment != b->alignment)
        return false;
    if (a->trailing_array) {
        if (!b->trailing_array)
            return false;
        return deluge_type_template_is_equal(a->trailing_array, b->trailing_array);
    } else if (b->trailing_array)
        return false;
    for (index = deluge_type_template_num_words(a); index--;) {
        if (a->word_types[index] != b->word_types[index])
            return false;
    }
    return true;
}

unsigned deluge_type_template_hash(const deluge_type_template* type)
{
    unsigned result;
    size_t index;
    result = type->size + 3 * type->alignment;
    if (type->trailing_array)
        result += 7 * deluge_type_template_hash(type->trailing_array);
    for (index = deluge_type_template_num_words(type); index--;) {
        result *= 11;
        result += type->word_types[index];
    }
    return result;
}

void deluge_word_type_dump(deluge_word_type type, pas_stream* stream)
{
    switch (type) {
    case DELUGE_WORD_TYPE_OFF_LIMITS:
        pas_stream_printf(stream, "/");
        return;
    case DELUGE_WORD_TYPE_INT:
        pas_stream_printf(stream, "i");
        return;
    case DELUGE_WORD_TYPE_PTR_SIDECAR:
        pas_stream_printf(stream, "S");
        return;
    case DELUGE_WORD_TYPE_PTR_CAPABILITY:
        pas_stream_printf(stream, "C");
        return;
    default:
        pas_stream_printf(stream, "?%u", type);
        return;
    }
}

static void type_dump_impl(const deluge_type* type, pas_stream* stream)
{
    static const bool dump_ptr = false;

    size_t index;

    if (dump_ptr)
        pas_stream_printf(stream, "@%p", type);

    if (!type) {
        pas_stream_printf(stream, "{invalid}");
        return;
    }

    if (type == &deluge_function_type) {
        pas_stream_printf(stream, "{function}");
        return;
    }

    if (type == &deluge_type_type) {
        pas_stream_printf(stream, "{type}");
        return;
    }

    if (type == &deluge_int_type) {
        pas_stream_printf(stream, "{int}");
        return;
    }

    if (!type->num_words) {
        pas_stream_printf(stream, "{unique:%u,%p", type->index, type);
        if (type->size)
            pas_stream_printf(stream, ",%zu,%zu", type->size, type->alignment);
        if (type->u.runtime_config)
            pas_stream_printf(stream, ",%p", type->u.runtime_config);
        pas_stream_printf(stream, "}");
        return;
    }
    
    pas_stream_printf(stream, "{%zu,%zu,", type->size, type->alignment);
    for (index = 0; index < type->num_words; ++index)
        deluge_word_type_dump(type->word_types[index], stream);
    if (type->u.trailing_array) {
        pas_stream_printf(stream, ",trail");
        type_dump_impl(type->u.trailing_array, stream);
    }
    pas_stream_printf(stream, "}");
}

void deluge_type_dump(const deluge_type* type, pas_stream* stream)
{
    static const bool dump_only_ptr = false;
    
    if (dump_only_ptr) {
        pas_stream_printf(stream, "%p", type);
        return;
    }

    pas_stream_printf(stream, "type");
    type_dump_impl(type, stream);
}

pas_heap_runtime_config* deluge_type_as_heap_type_get_runtime_config(
    const pas_heap_type* heap_type, pas_heap_runtime_config* config)
{
    const deluge_type* type = (const deluge_type*)heap_type;
    if (!type->num_words && type->u.runtime_config)
        return &type->u.runtime_config->base;
    return config;
}

pas_heap_runtime_config* deluge_type_as_heap_type_assert_default_runtime_config(
    const pas_heap_type* heap_type, pas_heap_runtime_config* config)
{
    const deluge_type* type = (const deluge_type*)heap_type;
    PAS_ASSERT(!(!type->num_words && type->u.runtime_config));
    return config;
}

void deluge_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream)
{
    deluge_type_dump((const deluge_type*)type, stream);
}

static char* type_to_new_string_impl(const deluge_type* type, pas_allocation_config* allocation_config)
{
    pas_string_stream stream;
    pas_string_stream_construct(&stream, allocation_config);
    deluge_type_dump(type, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* deluge_type_to_new_string(const deluge_type* type)
{
    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    return type_to_new_string_impl(type, &allocation_config);
}

static type_tables* get_type_tables(void)
{
    type_tables* result;
    result = pthread_getspecific(type_tables_key);
    if (!result) {
        result = deluge_allocate_utility(sizeof(type_tables));
        deluge_slice_table_construct(&result->slice_table);
        deluge_cat_table_construct(&result->cat_table);
        pthread_setspecific(type_tables_key, result);
    }
    return result;
}

static void check_int_slice_range(const deluge_type* type, pas_range range,
                                  const deluge_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;

    if (type == &deluge_int_type)
        return;

    offset = range.begin;
    first_word_type_index = offset / DELUGE_WORD_SIZE;
    last_word_type_index = (offset + pas_range_size(range) - 1) / DELUGE_WORD_SIZE;

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        DELUGE_CHECK(
            deluge_type_get_word_type(type, word_type_index) == DELUGE_WORD_TYPE_INT,
            origin,
            "cannot slice %zu...%zu as int, span contains non-ints (type = %s).",
            range.begin, range.end, deluge_type_to_new_string(type));
    }
}

const deluge_type* deluge_type_slice(const deluge_type* type, pas_range range, const deluge_origin* origin)
{
    deluge_slice_table* table;
    deluge_slice_table_key key;
    deluge_deep_slice_table_add_result add_result;
    pas_allocation_config allocation_config;
    deluge_slice_table_entry new_entry;
    uintptr_t offset_in_type;
    deluge_slice_table_entry* entry;
    
    if (type == &deluge_int_type)
        return &deluge_int_type;

    DELUGE_CHECK(
        type->num_words,
        origin,
        "cannot slice opaque types like %s.",
        deluge_type_to_new_string(type));

    if (type->u.trailing_array && range.begin >= type->size) {
        range = pas_range_sub(range, type->size);
        type = type->u.trailing_array;
        if (type == &deluge_int_type)
            return &deluge_int_type;
    }
    offset_in_type = range.begin % type->size;
    PAS_ASSERT(offset_in_type == range.begin || !type->u.trailing_array);
    PAS_ASSERT(range.begin >= offset_in_type);
    range = pas_range_sub(range, range.begin - offset_in_type);

    if (!range.begin && range.end == type->size && !type->u.trailing_array)
        return type;
    
    if ((range.begin % DELUGE_WORD_SIZE) || (range.end % DELUGE_WORD_SIZE)) {
        check_int_slice_range(type, range, origin);
        return &deluge_int_type;
    }

    PAS_TESTING_ASSERT(!(pas_range_size(range) % DELUGE_WORD_SIZE));

    table = &get_type_tables()->slice_table;
    key.base_type = type;
    key.range = range;
    entry = deluge_slice_table_find(table, key);
    if (entry)
        return entry->result_type;

    initialize_utility_allocation_config(&allocation_config);
    
    deluge_type_ops_lock_lock();
    add_result = deluge_deep_slice_table_add(&deluge_global_slice_table, key, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        deluge_type_template* type_template;
        size_t index;

        size_t total_size;
        DELUGE_CHECK(
            !pas_mul_uintptr_overflow(
                sizeof(deluge_word_type), pas_range_size(range) / DELUGE_WORD_SIZE, &total_size),
            origin,
            "range too big (integer overflow).");
        DELUGE_CHECK(
            !pas_add_uintptr_overflow(
                total_size, PAS_OFFSETOF(deluge_type_template, word_types), &total_size),
            origin,
            "range too big (integer overflow).");

        type_template = (deluge_type_template*)deluge_allocate_utility(total_size);
        type_template->size = pas_range_size(range);
        type_template->alignment = type->alignment;
        while (!pas_is_aligned(range.begin, type_template->alignment) ||
               !pas_is_aligned(range.end, type_template->alignment) ||
               !pas_is_aligned(pas_range_size(range), type_template->alignment))
            type_template->alignment /= 2;
        PAS_ASSERT(type_template->alignment >= DELUGE_WORD_SIZE);
        for (index = pas_range_size(range) / DELUGE_WORD_SIZE; index--;) {
            type_template->word_types[index] =
                deluge_type_get_word_type(type, index + range.begin / DELUGE_WORD_SIZE);
        }
        
        add_result.entry->key = key;
        add_result.entry->result_type = deluge_get_type(type_template);
    }
    deluge_type_ops_lock_unlock();

    new_entry.key = key;
    new_entry.result_type = add_result.entry->result_type;
    deluge_slice_table_add_new(table, new_entry, NULL, &allocation_config);

    return new_entry.result_type;
}

const deluge_type* deluge_type_cat(const deluge_type* a, size_t a_size,
                                   const deluge_type* b, size_t b_size,
                                   const deluge_origin* origin)
{
    deluge_cat_table* table;
    deluge_cat_table_key key;
    deluge_deep_cat_table_add_result add_result;
    pas_allocation_config allocation_config;
    deluge_cat_table_entry new_entry;
    deluge_cat_table_entry* entry;

    if (a == &deluge_int_type && b == &deluge_int_type)
        return &deluge_int_type;

    DELUGE_CHECK(
        a->num_words,
        origin,
        "cannot cat opaque types like %s.",
        deluge_type_to_new_string(a));
    DELUGE_CHECK(
        b->num_words,
        origin,
        "cannot cat opaque types like %s.",
        deluge_type_to_new_string(b));
    DELUGE_CHECK(
        pas_is_aligned(a_size, b->alignment),
        origin,
        "a_size %zu is not aligned to b type %s; refusing to add alignment padding for you.",
        a_size, deluge_type_to_new_string(b));

    if ((a_size % DELUGE_WORD_SIZE)) {
        check_int_slice_range(a, pas_range_create(0, a_size), origin);
        check_int_slice_range(b, pas_range_create(0, b_size), origin);
        return &deluge_int_type;
    }

    if ((b_size % DELUGE_WORD_SIZE)) {
        check_int_slice_range(b, pas_range_create(0, b_size), origin);
        b_size = pas_round_up_to_power_of_2(b_size, 8);
    }

    table = &get_type_tables()->cat_table;
    key.a = a;
    key.a_size = a_size;
    key.b = b;
    key.b_size = b_size;
    entry = deluge_cat_table_find(table, key);
    if (entry)
        return entry->result_type;

    initialize_utility_allocation_config(&allocation_config);

    deluge_type_ops_lock_lock();
    add_result = deluge_deep_cat_table_add(&deluge_global_cat_table, key, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        deluge_type_template* result_type;
        size_t index;
        size_t new_type_size;
        size_t total_size;
        size_t alignment;
        size_t aligned_size;

        DELUGE_CHECK(
            !pas_add_uintptr_overflow(a_size, b_size, &new_type_size),
            origin,
            "sizes too big (integer overflow).");
        DELUGE_CHECK(
            !pas_mul_uintptr_overflow(
                sizeof(deluge_word_type), new_type_size / DELUGE_WORD_SIZE, &total_size),
            origin,
            "sizes too big (integer overflow).");
        DELUGE_CHECK(
            !pas_add_uintptr_overflow(
                total_size, PAS_OFFSETOF(deluge_type_template, word_types), &total_size),
            origin,
            "sizes too big (integer overflow).");
        alignment = pas_max_uintptr(a->alignment, b->alignment);
        aligned_size = pas_round_up_to_power_of_2(new_type_size, alignment);
        DELUGE_CHECK(
            aligned_size >= new_type_size,
            origin,
            "sizes too big (hella strange alignment-related integer overflow).");

        result_type = (deluge_type_template*)deluge_allocate_utility(total_size);

        result_type->size = aligned_size;
        result_type->alignment = alignment;
        PAS_ASSERT(pas_is_aligned(result_type->size, result_type->alignment));
        for (index = a_size / DELUGE_WORD_SIZE; index--;)
            result_type->word_types[index] = deluge_type_get_word_type(a, index);
        for (index = b_size / DELUGE_WORD_SIZE; index--;) {
            result_type->word_types[index + a_size / DELUGE_WORD_SIZE] =
                deluge_type_get_word_type(b, index);
        }
        for (index = (aligned_size - new_type_size) / DELUGE_WORD_SIZE; index--;)
            result_type->word_types[index + new_type_size / DELUGE_WORD_SIZE] = DELUGE_WORD_TYPE_OFF_LIMITS;

        add_result.entry->key = key;
        add_result.entry->result_type = deluge_get_type(result_type);
    }

    deluge_type_ops_lock_unlock();

    new_entry.key = key;
    new_entry.result_type = add_result.entry->result_type;
    deluge_cat_table_add_new(table, new_entry, NULL, &allocation_config);

    return new_entry.result_type;
}

static pas_heap_ref* get_heap_slow_impl(deluge_heap_table* table, const deluge_type* type)
{
    pas_allocation_config allocation_config;
    deluge_heap_table_add_result add_result;

    PAS_ASSERT(type != &deluge_int_type);
    PAS_ASSERT(type->size);
    PAS_ASSERT(type->alignment);
    deluge_validate_type(type, NULL);
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = deluge_heap_table_add(table, type, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        add_result.entry->type = type;
        add_result.entry->heap = (pas_heap_ref*)deluge_allocate_utility(sizeof(pas_heap_ref));
        add_result.entry->heap->type = (const pas_heap_type*)type;
        add_result.entry->heap->heap = NULL;
        add_result.entry->heap->allocator_index = 0;
    }

    return add_result.entry->heap;
}

static unsigned fast_type_hash(const void* type, void* arg)
{
    PAS_ASSERT(!arg);
    return (uintptr_t)type / sizeof(deluge_type);
}

static pas_heap_ref* get_heap_impl(pas_lock_free_read_ptr_ptr_hashtable* fast_table,
                                   deluge_heap_table* slow_table,
                                   const deluge_type* type)
{
    static const bool verbose = false;
    pas_heap_ref* result;
    if (verbose) {
        pas_log("Getting heap for ");
        deluge_type_dump(type, &pas_log_stream.base);
        pas_log("\n");
    }
    /* FIXME: This only needs the lock-free table now. And we should make the lock-free table capable
       of using allocators that don't need the heap_lock. */
    PAS_ASSERT(type);
    result = (pas_heap_ref*)pas_lock_free_read_ptr_ptr_hashtable_find(
        fast_table, fast_type_hash, NULL, type);
    if (result)
        return result;
    deluge_type_lock_lock();
    result = get_heap_slow_impl(slow_table, type);
    deluge_type_lock_unlock();
    pas_heap_lock_lock();
    pas_lock_free_read_ptr_ptr_hashtable_set(
        fast_table, fast_type_hash, NULL, type, result,
        pas_lock_free_read_ptr_ptr_hashtable_set_maybe_existing);
    pas_heap_lock_unlock();
    return result;
}

pas_heap_ref* deluge_get_heap(const deluge_type* type)
{
    return get_heap_impl(&deluge_fast_heap_table, &deluge_normal_heap_table, type);
}

static void set_errno(int errno_value);

static PAS_ALWAYS_INLINE pas_allocation_result allocation_result_set_errno(pas_allocation_result result)
{
    if (!result.did_succeed)
        set_errno(ENOMEM);
    return result;
}

pas_allocator_counts deluge_allocator_counts;

pas_intrinsic_heap_support deluge_int_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap deluge_int_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &deluge_int_heap,
        &deluge_int_type,
        deluge_int_heap_support,
        DELUGE_HEAP_CONFIG,
        &deluge_intrinsic_runtime_config.base);

pas_intrinsic_heap_support deluge_utility_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap deluge_utility_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &deluge_utility_heap,
        &deluge_int_type,
        deluge_utility_heap_support,
        DELUGE_HEAP_CONFIG,
        &deluge_intrinsic_runtime_config.base);

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_try_allocate_int_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    allocation_result_set_errno,
    &deluge_int_heap,
    &deluge_int_heap_support,
    pas_intrinsic_heap_is_designated);

void* deluge_try_allocate_int(size_t size, size_t count)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_try_allocate_int_impl_ptr(size, 1);
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_try_allocate_int_with_alignment_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    allocation_result_set_errno,
    &deluge_int_heap,
    &deluge_int_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* deluge_try_allocate_int_with_alignment(size_t size, size_t count, size_t alignment)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_try_allocate_int_with_alignment_impl_ptr(size, alignment);
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_allocate_int_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_crash_on_error,
    &deluge_int_heap,
    &deluge_int_heap_support,
    pas_intrinsic_heap_is_designated);

void* deluge_allocate_int(size_t size, size_t count)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_allocate_int_impl_ptr(size, 1);
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_allocate_int_with_alignment_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_crash_on_error,
    &deluge_int_heap,
    &deluge_int_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* deluge_allocate_int_with_alignment(size_t size, size_t count, size_t alignment)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_allocate_int_with_alignment_impl_ptr(size, alignment);
}

PAS_CREATE_TRY_ALLOCATE(
    deluge_try_allocate_one_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_typed_runtime_config.base,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_allocate_one(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(!deluge_type_get_trailing_array((const deluge_type*)ref->type));
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    return deluge_try_allocate_one_impl_ptr(ref);
}

PAS_CREATE_TRY_ALLOCATE(
    deluge_allocate_one_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_typed_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_crash_on_error);

void* deluge_allocate_one(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(!deluge_type_get_trailing_array((const deluge_type*)ref->type));
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    return deluge_allocate_one_impl_ptr(ref);
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_try_allocate_many_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_typed_runtime_config.base,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_allocate_many(pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!((const deluge_type*)ref->type)->u.trailing_array);
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    if (count == 1)
        return deluge_try_allocate_one(ref);
    return (void*)deluge_try_allocate_many_impl_by_count(ref, count, 1).begin;
}

void* deluge_try_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment)
{
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!((const deluge_type*)ref->type)->u.trailing_array);
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    if (count == 1 && alignment <= PAS_MIN_ALIGN)
        return deluge_try_allocate_one(ref);
    return (void*)deluge_try_allocate_many_impl_by_count(ref, count, alignment).begin;
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_allocate_many_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_typed_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_crash_on_error);

void* deluge_allocate_many(pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!((const deluge_type*)ref->type)->u.trailing_array);
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    if (count == 1)
        return deluge_allocate_one(ref);
    return (void*)deluge_allocate_many_impl_by_count(ref, count, 1).begin;
}

static bool get_flex_size(size_t base_size, size_t element_size, size_t count, size_t* total_size)
{
    size_t extra_size;

    if (pas_mul_uintptr_overflow(element_size, count, &extra_size)) {
        set_errno(ENOMEM);
        return false;
    }

    if (pas_add_uintptr_overflow(base_size, extra_size, total_size)) {
        set_errno(ENOMEM);
        return false;
    }

    return true;
}

void* deluge_try_allocate_int_flex(size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return deluge_try_allocate_int_impl_ptr(total_size, 1);
}

void* deluge_try_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                                  size_t alignment)
{
    size_t total_size;
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return deluge_try_allocate_int_with_alignment_impl_ptr(total_size, alignment);
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_try_allocate_flex_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_flex_runtime_config.base,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->u.trailing_array);
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return (void*)deluge_try_allocate_flex_impl_by_size(ref, total_size, 1).begin;
}

void* deluge_try_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                              size_t count, size_t alignment)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->u.trailing_array);
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return (void*)deluge_try_allocate_flex_impl_by_size(ref, total_size, alignment).begin;
}

void* deluge_try_allocate_with_type(const deluge_type* type, size_t size)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "deluge_try_allocate_with_type",
        .line = 0,
        .column = 0
    };
    if (type == &deluge_int_type)
        return deluge_try_allocate_int(size, 1);
    DELUGE_CHECK(
        type->num_words,
        &origin,
        "cannot allocate instance of opaque type (type = %s).",
        deluge_type_to_new_string(type));
    DELUGE_CHECK(
        !type->u.trailing_array,
        &origin,
        "cannot reflectively allocate instance of type with trailing array (type = %s).",
        deluge_type_to_new_string(type));
    DELUGE_CHECK(
        !(size % type->size),
        &origin,
        "cannot allocate %zu bytes of type %s (have %zu remainder).",
        size, deluge_type_to_new_string(type), size % type->size);
    return deluge_try_allocate_many(deluge_get_heap(type), size / type->size);
}

void* deluge_allocate_with_type(const deluge_type* type, size_t size)
{
    void* result = deluge_try_allocate_with_type(type, size);
    if (!result)
        pas_panic_on_out_of_memory_error();
    return result;
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_allocate_utility_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_crash_on_error,
    &deluge_utility_heap,
    &deluge_utility_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* deluge_allocate_utility(size_t size)
{
    return deluge_allocate_utility_impl_ptr(size, 1);
}

void* deluge_try_reallocate_int(void* ptr, size_t size, size_t count)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return pas_try_reallocate_intrinsic(
        ptr,
        &deluge_int_heap,
        size,
        DELUGE_HEAP_CONFIG,
        deluge_try_allocate_int_impl_for_realloc,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void* deluge_try_reallocate_int_with_alignment(void* ptr, size_t size, size_t count, size_t alignment)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return pas_try_reallocate_intrinsic_with_alignment(
        ptr,
        &deluge_int_heap,
        size,
        alignment,
        DELUGE_HEAP_CONFIG,
        deluge_try_allocate_int_with_alignment_impl_for_realloc_with_alignment,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void* deluge_try_reallocate(void* ptr, pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge);
    return pas_try_reallocate_array_by_count(
        ptr,
        ref,
        count,
        DELUGE_HEAP_CONFIG,
        deluge_try_allocate_many_impl_for_realloc,
        &deluge_typed_runtime_config.base,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void deluge_deallocate(void* ptr)
{
    pas_deallocate(ptr, DELUGE_HEAP_CONFIG);
}

void deluge_deallocate_safe(deluge_ptr ptr)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "deluge_deallocate_safe",
        .line = 0,
        .column = 0
    };

    /* These checks aren't necessary to protect the allocator, which will already rule out
       pointers that don't belong to it. But, they are necessary to prevent the following:
       
       - Attempts to free deluge_types.
       
       - Attempts to free other opaque types, which may be allocated in our heap, and which
         may or may not take kindly to being freed without first doing other things or
         performing other checks.
       
       - Attempts to free someone else's object by doing pointer arithmetic or dreaming up
         a pointer with an inttoptr cast. */
    DELUGE_CHECK(
        deluge_ptr_ptr(ptr) == deluge_ptr_lower(ptr),
        &origin,
        "attempt to free a pointer with ptr != lower (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    
    if (!deluge_ptr_ptr(ptr))
        return;
    
    DELUGE_CHECK(
        deluge_ptr_type(ptr),
        &origin,
        "attempt to free nonnull pointer with invalid type (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    DELUGE_CHECK(
        deluge_ptr_type(ptr)->num_words,
        &origin,
        "attempt to free nonnull pointer to opaque type (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    
    deluge_deallocate(deluge_ptr_ptr(ptr));
}

void deluded_f_zfree(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zfree",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_deallocate_safe(ptr);
}

void deluded_f_zgetallocsize(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zgetallocsize",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(size_t), &origin);
    *(size_t*)deluge_ptr_ptr(rets) = pas_get_allocation_size(deluge_ptr_ptr(ptr), DELUGE_HEAP_CONFIG);
}

void deluded_f_zcalloc_multiply(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zcalloc_multiply",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    size_t left = deluge_ptr_get_next_size_t(&args, &origin);
    size_t right = deluge_ptr_get_next_size_t(&args, &origin);
    deluge_ptr result = deluge_ptr_get_next_ptr(&args, &origin);
    bool return_value;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(result, sizeof(size_t), &origin);
    if (__builtin_mul_overflow(left, right, (size_t*)deluge_ptr_ptr(result))) {
        return_value = false;
        set_errno(ENOMEM);
    } else
        return_value = true;
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)deluge_ptr_ptr(rets) = return_value;
}

pas_heap_ref* deluge_get_hard_heap(const deluge_type* type)
{
    return get_heap_impl(&deluge_fast_hard_heap_table, &deluge_hard_heap_table, type);
}

pas_intrinsic_heap_support deluge_hard_int_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap deluge_hard_int_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &deluge_hard_int_heap,
        &deluge_int_type,
        deluge_hard_int_heap_support,
        DELUGE_HARD_HEAP_CONFIG,
        &deluge_hard_int_runtime_config);

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_try_hard_allocate_int_impl,
    DELUGE_HARD_HEAP_CONFIG,
    &deluge_hard_int_runtime_config,
    &deluge_allocator_counts,
    allocation_result_set_errno,
    &deluge_hard_int_heap,
    &deluge_hard_int_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* deluge_try_hard_allocate_int(size_t size, size_t count)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_try_hard_allocate_int_impl_ptr(size, 1);
}

void* deluge_try_hard_allocate_int_with_alignment(size_t size, size_t count, size_t alignment)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return deluge_try_hard_allocate_int_impl_ptr(size, alignment);
}

PAS_CREATE_TRY_ALLOCATE(
    deluge_try_hard_allocate_one_impl,
    DELUGE_HARD_HEAP_CONFIG,
    &deluge_hard_typed_runtime_config,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_hard_allocate_one(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    PAS_TESTING_ASSERT(!deluge_type_get_trailing_array((const deluge_type*)ref->type));
    return deluge_try_hard_allocate_one_impl_ptr(ref);
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_try_hard_allocate_many_impl,
    DELUGE_HARD_HEAP_CONFIG,
    &deluge_hard_typed_runtime_config,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_hard_allocate_many(pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!((const deluge_type*)ref->type)->u.trailing_array);
    if (count == 1)
        return deluge_try_hard_allocate_one(ref);
    return (void*)deluge_try_hard_allocate_many_impl_by_count(ref, count, 1).begin;
}

void* deluge_try_hard_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!((const deluge_type*)ref->type)->u.trailing_array);
    if (count == 1 && alignment <= PAS_MIN_ALIGN)
        return deluge_try_hard_allocate_one(ref);
    return (void*)deluge_try_hard_allocate_many_impl_by_count(ref, count, alignment).begin;
}

void* deluge_try_hard_allocate_int_flex(size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return deluge_try_hard_allocate_int_impl_ptr(total_size, 1);
}

void* deluge_try_hard_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                                       size_t alignment)
{
    size_t total_size;
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return deluge_try_hard_allocate_int_impl_ptr(total_size, alignment);
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_try_hard_allocate_flex_impl,
    DELUGE_HARD_HEAP_CONFIG,
    &deluge_hard_flex_runtime_config,
    &deluge_allocator_counts,
    allocation_result_set_errno);

void* deluge_try_hard_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->u.trailing_array);
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return (void*)deluge_try_hard_allocate_flex_impl_by_size(ref, total_size, 1).begin;
}

void* deluge_try_hard_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                                   size_t count, size_t alignment)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(((const deluge_type*)ref->type)->u.trailing_array);
    if (!get_flex_size(base_size, element_size, count, &total_size))
        return NULL;
    return (void*)deluge_try_hard_allocate_flex_impl_by_size(ref, total_size, alignment).begin;
}

void* deluge_try_hard_reallocate_int(void* ptr, size_t size, size_t count)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return pas_try_reallocate_intrinsic(
        ptr,
        &deluge_hard_int_heap,
        size,
        DELUGE_HARD_HEAP_CONFIG,
        deluge_try_hard_allocate_int_impl_for_realloc,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void* deluge_try_hard_reallocate_int_with_alignment(void* ptr, size_t size, size_t count, size_t alignment)
{
    if (pas_mul_uintptr_overflow(size, count, &size)) {
        set_errno(ENOMEM);
        return NULL;
    }
    return pas_try_reallocate_intrinsic_with_alignment(
        ptr,
        &deluge_hard_int_heap,
        size,
        alignment,
        DELUGE_HARD_HEAP_CONFIG,
        deluge_try_hard_allocate_int_impl_for_realloc_with_alignment,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void* deluge_try_hard_reallocate(void* ptr, pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_deluge_hard);
    return pas_try_reallocate_array_by_count(
        ptr,
        ref,
        count,
        DELUGE_HARD_HEAP_CONFIG,
        deluge_try_hard_allocate_many_impl_for_realloc,
        &deluge_hard_typed_runtime_config,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void deluge_hard_deallocate(void* ptr)
{
    if (!ptr)
        return;
    
    size_t size = pas_get_allocation_size(ptr, DELUGE_HARD_HEAP_CONFIG);
    if (!size) {
        pas_deallocation_did_fail(
            "attempt to hard deallocate object not allocated by hard heap", (uintptr_t)ptr);
    }

    pas_zero_memory(ptr, size);

    pas_deallocate(ptr, DELUGE_HARD_HEAP_CONFIG);
}

static deluge_ptr new_ptr(void* ptr, size_t size, const deluge_type* type)
{
    if (!ptr)
        return deluge_ptr_forge_invalid(NULL);
    return deluge_ptr_forge(ptr, ptr, (char*)ptr + size, type);
}

pas_uint128 deluge_new_capability(void* ptr, size_t size, const deluge_type* type)
{
    static const bool verbose = false;
    if (verbose) {
        pas_log("Creating capability with ptr = %p, size = %zu, type = %s\n", ptr, size,
                deluge_type_to_new_string(type));
    }
    return new_ptr(ptr, size, type).capability;
}

pas_uint128 deluge_new_sidecar(void* ptr, size_t size, const deluge_type* type)
{
    static const bool verbose = false;
    if (verbose) {
        pas_log("Creating sidecar with ptr = %p, size = %zu, type = %s\n", ptr, size,
                deluge_type_to_new_string(type));
    }
    return new_ptr(ptr, size, type).sidecar;
}

void deluge_check_forge(
    void* ptr, size_t size, size_t count, const deluge_type* type, const deluge_origin* origin)
{
    size_t total_size;
    DELUGE_CHECK(
        !pas_mul_uintptr_overflow(size, count, &total_size),
        origin,
        "bad zunsafe_forge: size * count overflows (size = %zu, count = %zu).",
        size, count);
    DELUGE_CHECK(
        pas_is_aligned((uintptr_t)ptr, type->alignment),
        origin,
        "bad zunsafe_forge: pointer is not aligned to the type's alignment (ptr = %p, type = %s).",
        ptr, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        !((uintptr_t)ptr & ~PAS_ADDRESS_MASK),
        origin,
        "bad zunsafe_forge: pointer does not fit in capability (ptr = %p).",
        ptr);
    DELUGE_CHECK(
        !(((uintptr_t)ptr + total_size) & ~PAS_ADDRESS_MASK),
        origin,
        "bad zunsafe_forge: upper bound does not fit in capability "
        "(ptr = %p, total size = %zu, upper = %p).",
        ptr, total_size, (char*)ptr + total_size);

    /* These things should just be true if the compiler did its job. */
    PAS_TESTING_ASSERT(type->num_words);
    PAS_TESTING_ASSERT(!type->u.trailing_array);
    PAS_TESTING_ASSERT(!(total_size % type->size));
}

void deluded_f_zhard_free(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zhard_free",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    DELUGE_CHECK(
        deluge_ptr_ptr(ptr) == deluge_ptr_lower(ptr),
        &origin,
        "attempt to hard free a pointer with ptr != lower (ptr = %s).",
        deluge_ptr_to_new_string(ptr));

    if (!deluge_ptr_ptr(ptr))
        return;

    DELUGE_CHECK(
        deluge_ptr_type(ptr),
        &origin,
        "attempt to hard free nonnull pointer with invalid type (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    DELUGE_CHECK(
        deluge_ptr_type(ptr)->num_words,
        &origin,
        "attempt to hard free nonnull pointer to opaque type (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    
    deluge_hard_deallocate(deluge_ptr_ptr(ptr));
}

void deluded_f_zhard_getallocsize(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zhard_getallocsize",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(size_t), &origin);
    *(size_t*)deluge_ptr_ptr(rets) = pas_get_allocation_size(deluge_ptr_ptr(ptr), DELUGE_HARD_HEAP_CONFIG);
}

void deluded_f_zgetlower(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zgetlower",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    /* It's totally fine to store deluge_ptrs into the rets without any atomic stuff, because the
       rets buffer is guaranteed to only be read by the caller and then immediately discarded. */
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_with_ptr(ptr, deluge_ptr_lower(ptr));
}

void deluded_f_zgetupper(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zgetupper",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_with_ptr(ptr, deluge_ptr_upper(ptr));
}

void deluded_f_zgettype(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zgettype",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) =
        deluge_ptr_forge_byte((void*)deluge_ptr_type(ptr), &deluge_type_type);
}

void deluded_f_zslicetype(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zslicetype",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr type_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    size_t begin = deluge_ptr_get_next_size_t(&args, &origin);
    size_t end = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    deluge_check_access_opaque(type_ptr, &deluge_type_type, &origin);
    const deluge_type* result = deluge_type_slice(
        (const deluge_type*)deluge_ptr_ptr(type_ptr), pas_range_create(begin, end), &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte((void*)result, &deluge_type_type);
}

void deluge_validate_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                              const deluge_origin* origin)
{
    static const bool verbose = false;

    deluge_ptr ptr;
    deluge_ptr borked_ptr;
    const deluge_type* expected_borked_type;
    
    /* Have to create this manually because deluge_ptr_create calls validate! */
    ptr.sidecar = sidecar;
    ptr.capability = capability;
    borked_ptr.sidecar = 0;
    borked_ptr.capability = capability;

    DELUGE_ASSERT(deluge_ptr_capability_type_index(ptr) < deluge_type_array_size, origin);
    DELUGE_ASSERT(deluge_ptr_sidecar_type_index(ptr) < deluge_type_array_size, origin);

    if (verbose) {
        pas_log("validating ptr: ");
        deluge_ptr_dump(ptr, &pas_log_stream.base);
        pas_log("\n");
    }

    void* ptr_ptr = deluge_ptr_ptr(ptr);
    void* lower = deluge_ptr_lower(ptr);
    void* borked_lower = deluge_ptr_lower(borked_ptr);
    void* upper = deluge_ptr_upper(ptr);
    void* borked_upper = deluge_ptr_upper(borked_ptr);
    const deluge_type* type = deluge_ptr_type(ptr);

    if (!lower)
        DELUGE_ASSERT(!borked_lower, origin);
    DELUGE_ASSERT(borked_lower >= lower, origin);
    DELUGE_ASSERT(borked_upper <= upper, origin);
    if (deluge_ptr_capability_kind(ptr) == deluge_capability_flex_base)
        DELUGE_ASSERT(borked_lower == lower, origin);
    else
        DELUGE_ASSERT(borked_upper == upper, origin);
    
    if (!type) {
        DELUGE_ASSERT(!lower, origin);
        DELUGE_ASSERT(!upper, origin);
        return;
    }

    DELUGE_ASSERT(lower, origin);
    DELUGE_ASSERT(upper >= lower, origin);
    DELUGE_ASSERT(upper <= (void*)PAS_MAX_ADDRESS, origin);
    expected_borked_type = type;
    if (type->size) {
        const deluge_type* trailing_type;
        DELUGE_ASSERT(pas_is_aligned((uintptr_t)lower, type->alignment), origin);
        if (type->num_words) {
            trailing_type = type->u.trailing_array;
            if (trailing_type) {
                DELUGE_ASSERT(pas_is_aligned((uintptr_t)upper, trailing_type->alignment), origin);
                if (lower != upper) {
                    DELUGE_ASSERT((size_t)((char*)upper - (char*)lower) >= type->size, origin);
                    DELUGE_ASSERT(
                        !(((char*)upper - (char*)lower - type->size) % trailing_type->size), origin);
                }
                if (ptr_ptr < lower || (size_t)((char*)ptr_ptr - (char*)lower) >= type->size)
                    expected_borked_type = trailing_type;
            } else {
                DELUGE_ASSERT(pas_is_aligned((uintptr_t)upper, type->alignment), origin);
                DELUGE_ASSERT(!(((char*)upper - (char*)lower) % type->size), origin);
            }
        }
    }
    DELUGE_ASSERT(deluge_ptr_type(borked_ptr) == expected_borked_type, origin);
    if (!type->num_words && lower != upper) {
        DELUGE_ASSERT((char*)upper == (char*)lower + 1, origin);
        DELUGE_ASSERT((char*)upper == (char*)borked_lower + 1, origin);
    }

    deluge_validate_type(type, origin);
}

void* deluge_ptr_ptr_impl(pas_uint128 sidecar, pas_uint128 capability)
{
    return deluge_ptr_ptr(deluge_ptr_create(sidecar, capability));
}

static void check_access_common_maybe_opaque(deluge_ptr ptr, uintptr_t bytes, const deluge_origin* origin)
{
    if (PAS_ENABLE_TESTING)
        deluge_validate_ptr(ptr, origin);

    /* This check is not strictly necessary, but I like the error message. */
    DELUGE_CHECK(
        deluge_ptr_lower(ptr),
        origin,
        "cannot access pointer with null lower (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    
    DELUGE_CHECK(
        deluge_ptr_ptr(ptr) >= deluge_ptr_lower(ptr),
        origin,
        "cannot access pointer with ptr < lower (ptr = %s).", 
        deluge_ptr_to_new_string(ptr));

    DELUGE_CHECK(
        deluge_ptr_ptr(ptr) < deluge_ptr_upper(ptr),
        origin,
        "cannot access pointer with ptr >= upper (ptr = %s).",
        deluge_ptr_to_new_string(ptr));

    DELUGE_CHECK(
        bytes <= (uintptr_t)((char*)deluge_ptr_upper(ptr) - (char*)deluge_ptr_ptr(ptr)),
        origin,
        "cannot access %zu bytes when upper - ptr = %zu (ptr = %s).",
        bytes, (size_t)((char*)deluge_ptr_upper(ptr) - (char*)deluge_ptr_ptr(ptr)),
        deluge_ptr_to_new_string(ptr));
        
    DELUGE_CHECK(
        deluge_ptr_type(ptr),
        origin,
        "cannot access ptr with invalid type (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
}

static void check_access_common(deluge_ptr ptr, uintptr_t bytes, const deluge_origin* origin)
{
    check_access_common_maybe_opaque(ptr, bytes, origin);
    
    DELUGE_CHECK(
        deluge_ptr_type(ptr)->num_words,
        origin,
        "cannot access %zu bytes, span has opaque type (ptr = %s).",
        bytes, deluge_ptr_to_new_string(ptr));
}

void deluded_f_zgettypeslice(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zgettypeslice",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    size_t bytes = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    check_access_common(ptr, bytes, &origin);
    uintptr_t offset = deluge_ptr_offset(ptr);
    const deluge_type* result = deluge_type_slice(
        (const deluge_type*)deluge_ptr_type(ptr), pas_range_create(offset, offset + bytes), &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte((void*)result, &deluge_type_type);
}

void deluded_f_zcattype(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zcattype",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr a_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    size_t a_size = deluge_ptr_get_next_size_t(&args, &origin);
    deluge_ptr b_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    size_t b_size = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    deluge_check_access_opaque(a_ptr, &deluge_type_type, &origin);
    deluge_check_access_opaque(b_ptr, &deluge_type_type, &origin);
    const deluge_type* result = deluge_type_cat(
        (const deluge_type*)deluge_ptr_ptr(a_ptr), a_size,
        (const deluge_type*)deluge_ptr_ptr(b_ptr), b_size, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte((void*)result, &deluge_type_type);
}

void deluded_f_zalloc_with_type(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zalloc_with_type",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr type_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    size_t size = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    deluge_check_access_opaque(type_ptr, &deluge_type_type, &origin);
    const deluge_type* type = (const deluge_type*)deluge_ptr_ptr(type_ptr);
    if (type != &deluge_int_type) {
        /* This is never wrong, since type sizes are a multiple of 16. It just gives folks some forgiveness
           when using this API, which makes it a bit more practical to use it with zcattype. */
        size = pas_round_up_to_power_of_2(size, DELUGE_WORD_SIZE);
    }
    void* result = deluge_try_allocate_with_type(type, size);
    if (result) {
        *(deluge_ptr*)deluge_ptr_ptr(rets) =
            deluge_ptr_forge(result, result, (char*)result + size, type);
    }
}

void deluded_f_ztype_to_new_string(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "ztype_to_new_string",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr type_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    deluge_check_access_opaque(type_ptr, &deluge_type_type, &origin);
    const deluge_type* type = (const deluge_type*)deluge_ptr_ptr(type_ptr);
    pas_allocation_config allocation_config;
    initialize_int_allocation_config(&allocation_config);
    char* result = type_to_new_string_impl(type, &allocation_config);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = 
        deluge_ptr_forge(result, result, result + strlen(result) + 1, &deluge_int_type);
}

void deluded_f_zptr_to_new_string(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zptr_to_new_string",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    initialize_int_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    deluge_ptr_dump(ptr, &stream.base);
    char* result = pas_string_stream_take_string(&stream);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = 
        deluge_ptr_forge(result, result, result + strlen(result) + 1, &deluge_int_type);
}

static void check_int(deluge_ptr ptr, uintptr_t bytes, const deluge_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;
    const deluge_type* type;

    type = deluge_ptr_type(ptr);
    if (type == &deluge_int_type)
        return;

    offset = deluge_ptr_offset(ptr);
    first_word_type_index = offset / DELUGE_WORD_SIZE;
    last_word_type_index = (offset + bytes - 1) / DELUGE_WORD_SIZE;

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        DELUGE_CHECK(
            deluge_type_get_word_type(type, word_type_index) == DELUGE_WORD_TYPE_INT,
            origin,
            "cannot access %zu bytes as int, span contains non-ints (ptr = %s).",
            bytes, deluge_ptr_to_new_string(ptr));
    }
}

pas_uint128 deluge_update_sidecar(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr)
{
    return deluge_ptr_with_ptr(deluge_ptr_create(sidecar, capability), new_ptr).sidecar;
}

pas_uint128 deluge_update_capability(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr)
{
    return deluge_ptr_with_ptr(deluge_ptr_create(sidecar, capability), new_ptr).capability;
}

void deluge_check_access_int_impl(pas_uint128 sidecar, pas_uint128 capability, uintptr_t bytes,
                                  const deluge_origin* origin)
{
    deluge_ptr ptr;
    /* NOTE: the compiler will never generate a zero-byte check, but the runtime may do it. There
       are times when we are passed a primitive vector and a length and we run this check. If the
       length is zero, we want to permit any pointer (even a null or non-int ptr), since we're
       saying we aren't actually going to access it.
       
       If we ever want to optimize this check out, we just need to move it onto the paths where the
       runtime calls into this function, and allow the compiler to emit code that bypasses it. */
    if (!bytes)
        return;
    ptr = deluge_ptr_create(sidecar, capability);
    check_access_common(ptr, bytes, origin);
    check_int(ptr, bytes, origin);
}

void deluge_check_access_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const deluge_origin* origin)
{
    deluge_ptr ptr;
    uintptr_t offset;
    uintptr_t word_type_index;
    const deluge_type* type;

    ptr = deluge_ptr_create(sidecar, capability);

    check_access_common(ptr, sizeof(deluge_ptr), origin);

    offset = deluge_ptr_offset(ptr);
    DELUGE_CHECK(
        pas_is_aligned(offset, DELUGE_WORD_SIZE),
        origin,
        "cannot access memory as ptr without 16-byte alignment; in this case ptr %% 16 = %zu (ptr = %s).",
        (size_t)(offset % DELUGE_WORD_SIZE), deluge_ptr_to_new_string(ptr));
    word_type_index = offset / DELUGE_WORD_SIZE;
    type = deluge_ptr_type(ptr);
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 0) == DELUGE_WORD_TYPE_PTR_SIDECAR,
        origin,
        "memory type error accessing ptr sidecar (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
    /* It's interesting that this next check is almost certainly unnecessary, since no type will ever
       have a sidecar followed by anything other than a capability. */
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 1) == DELUGE_WORD_TYPE_PTR_CAPABILITY,
        origin,
        "memory type error accessing ptr capability (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
}

void deluge_check_function_call_impl(pas_uint128 sidecar, pas_uint128 capability,
                                     const deluge_origin* origin)
{
    deluge_ptr ptr;
    ptr = deluge_ptr_create(sidecar, capability);
    check_access_common_maybe_opaque(ptr, 1, origin);
    DELUGE_CHECK(
        deluge_ptr_type(ptr) == &deluge_function_type,
        origin,
        "attempt to call pointer that is not a function (ptr = %s).",
        deluge_ptr_to_new_string(ptr));
}

void deluge_check_access_opaque(
    deluge_ptr ptr, const deluge_type* expected_type, const deluge_origin* origin)
{
    PAS_TESTING_ASSERT(!expected_type->num_words);
    check_access_common_maybe_opaque(ptr, 1, origin);
    DELUGE_CHECK(
        deluge_ptr_type(ptr) == expected_type,
        origin,
        "expected ptr to %s during internal opaque access (ptr = %s).",
        deluge_type_to_new_string(expected_type),
        deluge_ptr_to_new_string(ptr));
}

void deluge_low_level_ptr_safe_bzero(void* raw_ptr, size_t bytes)
{
    static const bool verbose = false;
    size_t words;
    pas_uint128* ptr;
    if (verbose)
        pas_log("bytes = %zu\n", bytes);
    ptr = (pas_uint128*)raw_ptr;
    PAS_ASSERT(pas_is_aligned(bytes, DELUGE_WORD_SIZE));
    words = bytes / DELUGE_WORD_SIZE;
    while (words--)
        __c11_atomic_store((_Atomic pas_uint128*)ptr++, 0, __ATOMIC_RELAXED);
}

static void ptr_safe_memcpy_up(void* raw_dst, void* raw_src, size_t words)
{
    pas_uint128* dst;
    pas_uint128* src;
    dst = (pas_uint128*)raw_dst;
    src = (pas_uint128*)raw_src;
    while (words--) {
        __c11_atomic_store(
            (_Atomic pas_uint128*)dst++,
            __c11_atomic_load((_Atomic pas_uint128*)src++, __ATOMIC_RELAXED),
            __ATOMIC_RELAXED);
    }
}

static void ptr_safe_memcpy_down(void* raw_dst, void* raw_src, size_t words)
{
    pas_uint128* dst;
    pas_uint128* src;
    dst = (pas_uint128*)raw_dst;
    src = (pas_uint128*)raw_src;
    while (words--) {
        __c11_atomic_store(
            (_Atomic pas_uint128*)dst + words,
            __c11_atomic_load((_Atomic pas_uint128*)src + words, __ATOMIC_RELAXED),
            __ATOMIC_RELAXED);
    }
}

void deluge_low_level_ptr_safe_memcpy(void* dst, void* src, size_t bytes)
{
    PAS_ASSERT(pas_is_aligned(bytes, DELUGE_WORD_SIZE));
    ptr_safe_memcpy_up(dst, src, bytes / DELUGE_WORD_SIZE);
}

void deluge_low_level_ptr_safe_memmove(void* dst, void* src, size_t bytes)
{
    PAS_ASSERT(pas_is_aligned(bytes, DELUGE_WORD_SIZE));
    if (dst < src)
        ptr_safe_memcpy_up(dst, src, bytes / DELUGE_WORD_SIZE);
    else
        ptr_safe_memcpy_down(dst, src, bytes / DELUGE_WORD_SIZE);
}

void deluge_memset_impl(pas_uint128 sidecar, pas_uint128 capability,
                        unsigned value, size_t count, const deluge_origin* origin)
{
    static const bool verbose = false;
    deluge_ptr ptr;
    char* raw_ptr;
    
    if (!count)
        return;

    ptr = deluge_ptr_create(sidecar, capability);
    raw_ptr = deluge_ptr_ptr(ptr);

    if (verbose)
        pas_log("count = %zu\n", count);
    check_access_common(ptr, count, origin);
    
    if (!value) {
        const deluge_type* type;
        type = deluge_ptr_type(ptr);
        if (type != &deluge_int_type) {
            uintptr_t offset;
            deluge_word_type word_type;
            
            offset = deluge_ptr_offset(ptr);
            word_type = deluge_type_get_word_type(type, offset / DELUGE_WORD_SIZE);
            if (offset % DELUGE_WORD_SIZE) {
                DELUGE_ASSERT(
                    word_type != DELUGE_WORD_TYPE_PTR_SIDECAR
                    && word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            } else {
                DELUGE_ASSERT(
                    word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            }
            word_type = deluge_type_get_word_type(type, (offset + count - 1) / DELUGE_WORD_SIZE);
            if ((offset + count) % DELUGE_WORD_SIZE) {
                DELUGE_ASSERT(
                    word_type != DELUGE_WORD_TYPE_PTR_SIDECAR
                    && word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            } else {
                DELUGE_ASSERT(
                    word_type != DELUGE_WORD_TYPE_PTR_SIDECAR,
                    origin);
            }
        }

        if ((uintptr_t)raw_ptr % DELUGE_WORD_SIZE) {
            size_t sliver_size;
            char* new_raw_ptr;
            new_raw_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_ptr, DELUGE_WORD_SIZE);
            sliver_size = new_raw_ptr - raw_ptr;
            if (sliver_size > count) {
                memset(raw_ptr, 0, count);
                return;
            }
            memset(raw_ptr, 0, sliver_size);
            count -= sliver_size;
            raw_ptr = new_raw_ptr;
        }
        deluge_low_level_ptr_safe_bzero(raw_ptr, pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE));
        raw_ptr += pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        memset(raw_ptr, 0, count);
        return;
    }

    check_int(ptr, count, origin);
    memset(raw_ptr, value, count);
}

enum type_overlap_result {
    int_type_overlap,
    possible_ptr_type_overlap
};

typedef enum type_overlap_result type_overlap_result;

static type_overlap_result check_type_overlap(deluge_ptr dst, deluge_ptr src,
                                              size_t count, const deluge_origin* origin)
{
    const deluge_type* dst_type;
    const deluge_type* src_type;
    uintptr_t dst_offset;
    uintptr_t src_offset;
    deluge_word_type word_type;
    uintptr_t num_word_types;
    uintptr_t word_type_index_offset;
    uintptr_t first_dst_word_type_index;
    uintptr_t first_src_word_type_index;

    dst_type = deluge_ptr_type(dst);
    src_type = deluge_ptr_type(src);

    if (dst_type == &deluge_int_type && src_type == &deluge_int_type)
        return int_type_overlap;

    dst_offset = deluge_ptr_offset(dst);
    src_offset = deluge_ptr_offset(src);
    if ((dst_offset % DELUGE_WORD_SIZE) != (src_offset % DELUGE_WORD_SIZE)) {
        /* No chance we could copy pointers if the offsets are skewed within a word.
           
           It would be harder to write a generalized checking algorithm for this case (the
           non-offset-skew version lower in this function can't handle it) and we don't need
           to, since that path could only succeed if there were only integers on both sides of
           the copy. */
        check_int(dst, count, origin);
        check_int(src, count, origin);
        return int_type_overlap;
    }

    num_word_types = (dst_offset + count - 1) / DELUGE_WORD_SIZE - dst_offset / DELUGE_WORD_SIZE + 1;
    first_dst_word_type_index = dst_offset / DELUGE_WORD_SIZE;
    first_src_word_type_index = src_offset / DELUGE_WORD_SIZE;

    for (word_type_index_offset = num_word_types; word_type_index_offset--;) {
        DELUGE_ASSERT(
            deluge_type_get_word_type(dst_type, first_dst_word_type_index + word_type_index_offset)
            ==
            deluge_type_get_word_type(src_type, first_src_word_type_index + word_type_index_offset),
            origin);
    }

    /* We cannot copy parts of pointers. */
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index);
    if (dst_offset % DELUGE_WORD_SIZE) {
        DELUGE_ASSERT(
            word_type != DELUGE_WORD_TYPE_PTR_SIDECAR
            && word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
            origin);
    } else {
        DELUGE_ASSERT(
            word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
            origin);
    }
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index + num_word_types - 1);
    if ((dst_offset + count) % DELUGE_WORD_SIZE) {
        DELUGE_ASSERT(
            word_type != DELUGE_WORD_TYPE_PTR_SIDECAR
            && word_type != DELUGE_WORD_TYPE_PTR_CAPABILITY,
            origin);
    } else {
        DELUGE_ASSERT(
            word_type != DELUGE_WORD_TYPE_PTR_SIDECAR,
            origin);
    }

    /* We could do better here if we really wanted to. */
    return possible_ptr_type_overlap;
}

static type_overlap_result check_copy(deluge_ptr dst, deluge_ptr src,
                                      size_t count, const deluge_origin* origin)
{
    check_access_common(dst, count, origin);
    check_access_common(src, count, origin);
    return check_type_overlap(dst, src, count, origin);
}

/* NOTE: There's an argument to be made that memcpy in Deluge should always memmove, because:
   
   - The overhead of Deluge is likely to make the small perf difference between memcpy and memmove
     irrelevant.

   - If you're copying a range that includes any pointers then it's unlikely that there is any
     difference in performance between memcpy and memmove at all. */
void deluge_memcpy_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                        pas_uint128 src_sidecar, pas_uint128 src_capability,
                        size_t count, const deluge_origin* origin)
{
    deluge_ptr dst;
    deluge_ptr src;
    char* raw_dst_ptr;
    char* raw_src_ptr;
    
    if (!count)
        return;

    dst = deluge_ptr_create(dst_sidecar, dst_capability);
    src = deluge_ptr_create(src_sidecar, src_capability);

    raw_dst_ptr = deluge_ptr_ptr(dst);
    raw_src_ptr = deluge_ptr_ptr(src);
    
    switch (check_copy(dst, src, count, origin)) {
    case int_type_overlap:
        memcpy(raw_dst_ptr, raw_src_ptr, count);
        return;
    case possible_ptr_type_overlap: {
        PAS_TESTING_ASSERT(
            ((uintptr_t)raw_dst_ptr % DELUGE_WORD_SIZE) == ((uintptr_t)raw_src_ptr % DELUGE_WORD_SIZE));
        if ((uintptr_t)raw_dst_ptr % DELUGE_WORD_SIZE) {
            size_t sliver_size;
            char* new_raw_dst_ptr;
            char* new_raw_src_ptr;
            new_raw_dst_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_dst_ptr, DELUGE_WORD_SIZE);
            new_raw_src_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_src_ptr, DELUGE_WORD_SIZE);
            sliver_size = new_raw_dst_ptr - raw_dst_ptr;
            if (sliver_size > count) {
                memcpy(raw_dst_ptr, raw_src_ptr, count);
                return;
            }
            memcpy(raw_dst_ptr, raw_src_ptr, sliver_size);
            count -= sliver_size;
            raw_dst_ptr = new_raw_dst_ptr;
            raw_src_ptr = new_raw_src_ptr;
        }
        deluge_low_level_ptr_safe_memcpy(
            raw_dst_ptr, raw_src_ptr, pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE));
        raw_dst_ptr += pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        raw_src_ptr += pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        memcpy(raw_dst_ptr, raw_src_ptr, count);
        return;
    } }
    PAS_ASSERT(!"Should not be reached");
}

void deluge_memmove_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                         pas_uint128 src_sidecar, pas_uint128 src_capability,
                         size_t count, const deluge_origin* origin)
{
    deluge_ptr dst;
    deluge_ptr src;
    char* raw_dst_ptr;
    char* raw_src_ptr;
    
    if (!count)
        return;
    
    dst = deluge_ptr_create(dst_sidecar, dst_capability);
    src = deluge_ptr_create(src_sidecar, src_capability);

    raw_dst_ptr = deluge_ptr_ptr(dst);
    raw_src_ptr = deluge_ptr_ptr(src);
    
    switch (check_copy(dst, src, count, origin)) {
    case int_type_overlap:
        memmove(raw_dst_ptr, raw_src_ptr, count);
        return;
    case possible_ptr_type_overlap: {
        void* low_dst;
        void* low_src;
        size_t low_count;
        void* mid_dst;
        void* mid_src;
        size_t mid_count;
        void* high_dst;
        void* high_src;
        size_t high_count;
        size_t sliver_size;
        char* new_raw_dst_ptr;
        char* new_raw_src_ptr;
        PAS_TESTING_ASSERT(
            ((uintptr_t)raw_dst_ptr % DELUGE_WORD_SIZE) == ((uintptr_t)raw_src_ptr % DELUGE_WORD_SIZE));
        new_raw_dst_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_dst_ptr, DELUGE_WORD_SIZE);
        new_raw_src_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_src_ptr, DELUGE_WORD_SIZE);
        sliver_size = new_raw_dst_ptr - raw_dst_ptr;
        if (sliver_size > count) {
            memmove(raw_dst_ptr, raw_src_ptr, count);
            return;
        }
        low_dst = raw_dst_ptr;
        low_src = raw_src_ptr;
        low_count = sliver_size;
        count -= sliver_size;
        raw_dst_ptr = new_raw_dst_ptr;
        raw_src_ptr = new_raw_src_ptr;
        mid_dst = raw_dst_ptr;
        mid_src = raw_src_ptr;
        mid_count = pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        raw_dst_ptr += pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        raw_src_ptr += pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, DELUGE_WORD_SIZE);
        high_dst = raw_dst_ptr;
        high_src = raw_src_ptr;
        high_count = count;
        if (low_dst < low_src) {
            memmove(low_dst, low_src, low_count);
            deluge_low_level_ptr_safe_memmove(mid_dst, mid_src, mid_count);
            memmove(high_dst, high_src, high_count);
        } else {
            memmove(high_dst, high_src, high_count);
            deluge_low_level_ptr_safe_memmove(mid_dst, mid_src, mid_count);
            memmove(low_dst, low_src, low_count);
        }
        return;
    } }
    PAS_ASSERT(!"Should not be reached");
}

void deluge_check_restrict(pas_uint128 sidecar, pas_uint128 capability,
                           void* new_upper, const deluge_type* new_type, const deluge_origin* origin)
{
    deluge_ptr ptr;
    ptr = deluge_ptr_create(sidecar, capability);
    check_access_common(ptr, (char*)new_upper - (char*)deluge_ptr_ptr(ptr), origin);
    DELUGE_CHECK(new_type, origin, "cannot restrict to NULL type");
    check_type_overlap(deluge_ptr_forge(deluge_ptr_ptr(ptr), deluge_ptr_ptr(ptr), new_upper, new_type),
                       ptr, (char*)new_upper - (char*)deluge_ptr_ptr(ptr), origin);
}

const char* deluge_check_and_get_str(deluge_ptr str, const deluge_origin* origin)
{
    size_t available;
    size_t length;
    check_access_common(str, 1, origin);
    available = deluge_ptr_available(str);
    length = strnlen((char*)deluge_ptr_ptr(str), available);
    PAS_ASSERT(length < available);
    PAS_ASSERT(length + 1 <= available);
    check_int(str, length + 1, origin);
    return (char*)deluge_ptr_ptr(str);
}

deluge_ptr deluge_strdup(const char* str)
{
    if (!str)
        return deluge_ptr_forge_invalid(NULL);
    size_t size = strlen(str) + 1;
    char* result = (char*)deluge_allocate_int(size, 1);
    memcpy(result, str, size);
    return deluge_ptr_forge(result, result, result + size, &deluge_int_type);
}

void* deluge_va_arg_impl(
    pas_uint128 va_list_sidecar, pas_uint128 va_list_capability,
    size_t count, size_t alignment, const deluge_type* type, const deluge_origin* origin)
{
    deluge_ptr va_list;
    deluge_ptr* va_list_impl;
    void* result;
    va_list = deluge_ptr_create(va_list_sidecar, va_list_capability);
    deluge_check_access_ptr(va_list, origin);
    va_list_impl = (deluge_ptr*)deluge_ptr_ptr(va_list);
    return deluge_ptr_ptr(deluge_ptr_get_next(va_list_impl, count, alignment, type, origin));
}

deluge_global_initialization_context* deluge_global_initialization_context_create(
    deluge_global_initialization_context* parent)
{
    static const bool verbose = false;
    
    static const size_t initial_capacity = 1;
    
    deluge_global_initialization_context* result;

    if (verbose)
        pas_log("creating context with parent = %p\n", parent);
    
    if (parent) {
        parent->ref_count++;
        return parent;
    }

    result = (deluge_global_initialization_context*)
        deluge_allocate_utility(sizeof(deluge_global_initialization_context));
    result->ref_count = 1;
    pas_ptr_hash_set_construct(&result->seen);
    result->entries = (deluge_initialization_entry*)
        deluge_allocate_utility(sizeof(deluge_initialization_entry) * initial_capacity);
    result->num_entries = 0;
    result->entries_capacity = initial_capacity;

    return result;
}

bool deluge_global_initialization_context_add(
    deluge_global_initialization_context* context, pas_uint128* deluded_gptr, pas_uint128 ptr_capability)
{
    static const bool verbose = false;
    
    pas_allocation_config allocation_config;

    PAS_ASSERT(!*deluded_gptr);

    initialize_utility_allocation_config(&allocation_config);

    if (verbose)
        pas_log("capability = %s\n", deluge_ptr_to_new_string(deluge_ptr_create(0, ptr_capability)));

    if (!pas_ptr_hash_set_set(&context->seen, deluded_gptr, NULL, &allocation_config)) {
        if (verbose)
            pas_log("was already seen\n");
        return false;
    }

    if (context->num_entries >= context->entries_capacity) {
        size_t new_capacity;
        deluge_initialization_entry* new_entries;
        
        PAS_ASSERT(context->num_entries = context->entries_capacity);

        new_capacity = context->entries_capacity * 2;
        PAS_ASSERT(new_capacity > context->entries_capacity);
        new_entries = (deluge_initialization_entry*)
            deluge_allocate_utility(sizeof(deluge_initialization_entry) * new_capacity);

        memcpy(new_entries, context->entries, sizeof(deluge_initialization_entry) * context->num_entries);

        deluge_deallocate(context->entries);

        context->entries = new_entries;
        context->entries_capacity = new_capacity;
        PAS_ASSERT(context->num_entries < context->entries_capacity);
    }

    context->entries[context->num_entries].deluded_gptr = deluded_gptr;
    context->entries[context->num_entries].ptr_capability = ptr_capability;
    context->num_entries++;
    return true;
}

void deluge_global_initialization_context_destroy(deluge_global_initialization_context* context)
{
    size_t index;
    pas_allocation_config allocation_config;
    
    if (--context->ref_count)
        return;

    pas_store_store_fence();

    for (index = context->num_entries; index--;) {
        PAS_ASSERT(!*context->entries[index].deluded_gptr);
        __c11_atomic_store((_Atomic pas_uint128*)context->entries[index].deluded_gptr,
                           context->entries[index].ptr_capability, __ATOMIC_RELAXED);
    }

    initialize_utility_allocation_config(&allocation_config);

    deluge_deallocate(context->entries);
    pas_ptr_hash_set_destruct(&context->seen, &allocation_config);
    deluge_deallocate(context);
}

void deluge_alloca_stack_push(deluge_alloca_stack* stack, void* alloca)
{
    if (stack->size >= stack->capacity) {
        void** new_array;
        size_t new_capacity;
        PAS_ASSERT(stack->size == stack->capacity);

        new_capacity = (stack->capacity + 1) * 2;
        new_array = (void**)deluge_allocate_utility(new_capacity * sizeof(void*));

        memcpy(new_array, stack->array, stack->size * sizeof(void*));

        deluge_deallocate(stack->array);

        stack->array = new_array;
        stack->capacity = new_capacity;

        PAS_ASSERT(stack->size < stack->capacity);
    }
    PAS_ASSERT(stack->size < stack->capacity);

    stack->array[stack->size++] = alloca;
}

void deluge_alloca_stack_restore(deluge_alloca_stack* stack, size_t size)
{
    // We use our own origin because the compiler would have had to mess up real bad for an error to
    // happen here. Not sure it's possible for an error to happen here other than via a miscompile. But,
    // even in case of miscompile, we'll always do the type-safe thing here. Worst case, we free allocas
    // too soon, but then they are freed into the right heap.
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "deluge_alloca_stack_restore",
        .line = 0,
        .column = 0
    };
    DELUGE_CHECK(
        size <= stack->size,
        &origin,
        "cannot restore alloca stack to %zu, size is %zu.",
        size, stack->size);
    for (size_t index = size; index < stack->size; ++index)
        deluge_deallocate(stack->array[index]);
    stack->size = size;
}

void deluge_alloca_stack_destroy(deluge_alloca_stack* stack)
{
    size_t index;

    PAS_ASSERT(stack->size <= stack->capacity);

    for (index = stack->size; index--;)
        deluge_deallocate(stack->array[index]);
    deluge_deallocate(stack->array);
}

static deluge_ptr get_constant_value(deluge_constant_kind kind, void* target,
                                     deluge_global_initialization_context* context)
{
    switch (kind) {
    case deluge_function_constant:
        return deluge_ptr_forge_byte(target, &deluge_function_type);
    case deluge_global_constant:
        return deluge_ptr_create(
            0, ((pas_uint128 (*)(deluge_global_initialization_context*))target)(context));
    case deluge_expr_constant: {
        deluge_constexpr_node* node = (deluge_constexpr_node*)target;
        switch (node->opcode) {
        case deluge_constexpr_add_ptr_immediate:
            return deluge_ptr_with_offset(get_constant_value(node->left_kind, node->left_target, context),
                                          node->right_value);
        }
        PAS_ASSERT(!"Bad constexpr opcode");
    } }
    PAS_ASSERT(!"Bad relocation kind");
    return deluge_ptr_create(0, 0);
}

void deluge_execute_constant_relocations(
    void* constant, deluge_constant_relocation* relocations, size_t num_relocations,
    deluge_global_initialization_context* context)
{
    size_t index;
    PAS_ASSERT(context);
    /* Nothing here needs to be atomic, since the constant doesn't become visible to the universe
       until the initialization context is destroyed. */
    for (index = num_relocations; index--;) {
        deluge_constant_relocation* relocation;
        deluge_ptr* ptr_ptr;
        relocation = relocations + index;
        PAS_ASSERT(pas_is_aligned(relocation->offset, DELUGE_WORD_SIZE));
        ptr_ptr = (deluge_ptr*)((char*)constant + relocation->offset);
        PAS_ASSERT(!ptr_ptr->sidecar);
        PAS_ASSERT(!ptr_ptr->capability);
        PAS_ASSERT(pas_is_aligned((uintptr_t)ptr_ptr, DELUGE_WORD_SIZE));
        *ptr_ptr = get_constant_value(relocation->kind, relocation->target, context);
    }
}

static bool did_run_deferred_global_ctors = false;
static void (**deferred_global_ctors)(DELUDED_SIGNATURE) = NULL; 
static size_t num_deferred_global_ctors = 0;
static size_t deferred_global_ctors_capacity = 0;

void deluge_defer_or_run_global_ctor(void (*global_ctor)(DELUDED_SIGNATURE))
{
    if (did_run_deferred_global_ctors) {
        uintptr_t return_buffer[2];
        global_ctor(NULL, NULL, NULL, return_buffer, return_buffer + 2, &deluge_int_type);
        return;
    }

    if (num_deferred_global_ctors >= deferred_global_ctors_capacity) {
        void (**new_deferred_global_ctors)(DELUDED_SIGNATURE);
        size_t new_deferred_global_ctors_capacity;

        PAS_ASSERT(num_deferred_global_ctors == deferred_global_ctors_capacity);

        new_deferred_global_ctors_capacity = (deferred_global_ctors_capacity + 1) * 2;
        new_deferred_global_ctors = (void (**)(DELUDED_SIGNATURE))deluge_allocate_utility(
            new_deferred_global_ctors_capacity * sizeof(void (*)(DELUDED_SIGNATURE)));

        memcpy(new_deferred_global_ctors, deferred_global_ctors,
               num_deferred_global_ctors * sizeof(void (*)(DELUDED_SIGNATURE)));

        deluge_deallocate(deferred_global_ctors);

        deferred_global_ctors = new_deferred_global_ctors;
        deferred_global_ctors_capacity = new_deferred_global_ctors_capacity;
    }

    deferred_global_ctors[num_deferred_global_ctors++] = global_ctor;
}

void deluge_run_deferred_global_ctors(void)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "deluge_run_deferred_global_ctors",
        .line = 0,
        .column = 0
    };
    uintptr_t return_buffer[2];
    DELUGE_CHECK(
        !did_run_deferred_global_ctors,
        &origin,
        "cannot run deferred global constructors twice.");
    did_run_deferred_global_ctors = true;
    /* It's important to run the destructors in exactly the order in which they were deferred, since
       this allows us to match the priority semantics despite not having the priority. */
    for (size_t index = 0; index < num_deferred_global_ctors; ++index)
        deferred_global_ctors[index](NULL, NULL, NULL, return_buffer, return_buffer + 2, &deluge_int_type);
    deluge_deallocate(deferred_global_ctors);
    num_deferred_global_ctors = 0;
    deferred_global_ctors_capacity = 0;
}

void deluded_f_zrun_deferred_global_ctors(DELUDED_SIGNATURE)
{
    DELUDED_DELETE_ARGS();
    deluge_run_deferred_global_ctors();
}

void deluge_panic(const deluge_origin* origin, const char* format, ...)
{
    va_list args;
    if (origin) {
        pas_log("%s:", origin->filename);
        if (origin->line) {
            pas_log("%u:", origin->line);
            if (origin->column)
                pas_log("%u:", origin->column);
        }
        pas_log(" ");
        if (origin->function)
            pas_log("%s: ", origin->function);
    } else
        pas_log("<somewhere>: ");
    va_start(args, format);
    pas_vlog(format, args);
    va_end(args);
    pas_log("\n");
    pas_panic("thwarted a futile attempt to violate memory safety.\n");
}

void deluge_error(const deluge_origin* origin)
{
    deluge_panic(origin, "deluge_error called");
}

static void print_str(const char* str)
{
    size_t length;
    length = strlen(str);
    while (length) {
        ssize_t result = write(2, str, length);
        PAS_ASSERT(result);
        if (result < 0 && errno == EINTR)
            continue;
        PAS_ASSERT(result > 0);
        PAS_ASSERT((size_t)result <= length);
        str += result;
        length -= result;
    }
}

void deluded_f_zprint(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zprint",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr str;
    str = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    print_str(deluge_check_and_get_str(str, &origin));
}

void deluded_f_zprint_long(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zprint_long",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    long x;
    char buf[100];
    x = deluge_ptr_get_next_long(&args, &origin);
    DELUDED_DELETE_ARGS();
    snprintf(buf, sizeof(buf), "%ld", x);
    print_str(buf);
}

void deluded_f_zprint_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zprint_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr ptr;
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    initialize_utility_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    deluge_ptr_dump(ptr, &stream.base);
    print_str(pas_string_stream_get_string(&stream));
    pas_string_stream_destruct(&stream);
}

void deluded_f_zerror(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zerror",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    const char* str = deluge_check_and_get_str(deluge_ptr_get_next_ptr(&args, &origin), &origin);
    DELUDED_DELETE_ARGS();
    pas_panic("zerror: %s\n", str);
}

void deluded_f_zstrlen(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zstrlen",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    const char* str = deluge_check_and_get_str(deluge_ptr_get_next_ptr(&args, &origin), &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = strlen(str);
}

void deluded_f_zstrchr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zstrlen",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr str_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    const char* str = deluge_check_and_get_str(str_ptr, &origin);
    int chr = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_with_ptr(str_ptr, strchr(str, chr));
}

void deluded_f_zisdigit(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zisdigit",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int chr = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = isdigit(chr);
}

void deluded_f_zfence(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    DELUDED_DELETE_ARGS();
    pas_fence();
}

void deluded_f_zunfenced_weak_cas_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_weak_cas_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr expected = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)deluge_ptr_ptr(rets) = deluge_ptr_unfenced_weak_cas(deluge_ptr_ptr(ptr), expected, new_value);
}

void deluded_f_zweak_cas_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zweak_cas_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr expected = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)deluge_ptr_ptr(rets) = deluge_ptr_weak_cas(deluge_ptr_ptr(ptr), expected, new_value);
}

void deluded_f_zunfenced_strong_cas_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_strong_cas_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr expected = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) =
        deluge_ptr_unfenced_strong_cas(deluge_ptr_ptr(ptr), expected, new_value);
}

void deluded_f_zstrong_cas_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zstrong_cas_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr expected = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_strong_cas(deluge_ptr_ptr(ptr), expected, new_value);
}

void deluded_f_zunfenced_xchg_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_xchg_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_unfenced_xchg(deluge_ptr_ptr(ptr), new_value);
}

void deluded_f_zxchg_ptr(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zxchg_ptr",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr new_value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(ptr, &origin);
    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_xchg(deluge_ptr_ptr(ptr), new_value);
}

void deluded_f_zis_runtime_testing_enabled(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zis_runtime_testing_enabled",
        .line = 0,
        .column = 0
    };
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)deluge_ptr_ptr(rets) = !!PAS_ENABLE_TESTING;
}

static void (*deluded_errno_handler)(DELUDED_SIGNATURE);

void deluded_f_zregister_sys_errno_handler(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zregister_sys_errno_handler",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr errno_handler = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    DELUGE_CHECK(
        !deluded_errno_handler,
        &origin,
        "errno handler already registered.");
    deluge_check_function_call(errno_handler, &origin);
    deluded_errno_handler = (void(*)(DELUDED_SIGNATURE))deluge_ptr_ptr(errno_handler);
}

static void set_musl_errno(int errno_value)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "set_musl_errno",
        .line = 0,
        .column = 0
    };
    uintptr_t return_buffer[2];
    int* args;
    DELUGE_CHECK(
        deluded_errno_handler,
        &origin,
        "errno handler not registered when trying to set errno = %d.", errno_value);
    args = (int*)deluge_allocate_int(sizeof(int), 1);
    *args = errno_value;
    memset(return_buffer, 0, sizeof(return_buffer));
    deluded_errno_handler(args, args + 1, &deluge_int_type,
                          return_buffer, return_buffer + 2, &deluge_int_type);
}

static int to_musl_errno(int errno_value)
{
    // FIXME: This must be in sync with musl's bits/errno.h, which feels wrong.
    // FIXME: Wouldn't it be infinitely better if we just gave deluded code the real errno?
    switch (errno_value) {
    case EPERM          : return  1;
    case ENOENT         : return  2;
    case ESRCH          : return  3;
    case EINTR          : return  4;
    case EIO            : return  5;
    case ENXIO          : return  6;
    case E2BIG          : return  7;
    case ENOEXEC        : return  8;
    case EBADF          : return  9;
    case ECHILD         : return 10;
    case EAGAIN         : return 11;
    case ENOMEM         : return 12;
    case EACCES         : return 13;
    case EFAULT         : return 14;
    case ENOTBLK        : return 15;
    case EBUSY          : return 16;
    case EEXIST         : return 17;
    case EXDEV          : return 18;
    case ENODEV         : return 19;
    case ENOTDIR        : return 20;
    case EISDIR         : return 21;
    case EINVAL         : return 22;
    case ENFILE         : return 23;
    case EMFILE         : return 24;
    case ENOTTY         : return 25;
    case ETXTBSY        : return 26;
    case EFBIG          : return 27;
    case ENOSPC         : return 28;
    case ESPIPE         : return 29;
    case EROFS          : return 30;
    case EMLINK         : return 31;
    case EPIPE          : return 32;
    case EDOM           : return 33;
    case ERANGE         : return 34;
    case EDEADLK        : return 35;
    case ENAMETOOLONG   : return 36;
    case ENOLCK         : return 37;
    case ENOSYS         : return 38;
    case ENOTEMPTY      : return 39;
    case ELOOP          : return 40;
    case ENOMSG         : return 42;
    case EIDRM          : return 43;
    case ENOSTR         : return 60;
    case ENODATA        : return 61;
    case ETIME          : return 62;
    case ENOSR          : return 63;
    case EREMOTE        : return 66;
    case ENOLINK        : return 67;
    case EPROTO         : return 71;
    case EMULTIHOP      : return 72;
    case EBADMSG        : return 74;
    case EOVERFLOW      : return 75;
    case EILSEQ         : return 84;
    case EUSERS         : return 87;
    case ENOTSOCK       : return 88;
    case EDESTADDRREQ   : return 89;
    case EMSGSIZE       : return 90;
    case EPROTOTYPE     : return 91;
    case ENOPROTOOPT    : return 92;
    case EPROTONOSUPPORT: return 93;
    case ESOCKTNOSUPPORT: return 94;
    case EOPNOTSUPP     : return 95;
    case ENOTSUP        : return 95;
    case EPFNOSUPPORT   : return 96;
    case EAFNOSUPPORT   : return 97;
    case EADDRINUSE     : return 98;
    case EADDRNOTAVAIL  : return 99;
    case ENETDOWN       : return 100;
    case ENETUNREACH    : return 101;
    case ENETRESET      : return 102;
    case ECONNABORTED   : return 103;
    case ECONNRESET     : return 104;
    case ENOBUFS        : return 105;
    case EISCONN        : return 106;
    case ENOTCONN       : return 107;
    case ESHUTDOWN      : return 108;
    case ETOOMANYREFS   : return 109;
    case ETIMEDOUT      : return 110;
    case ECONNREFUSED   : return 111;
    case EHOSTDOWN      : return 112;
    case EHOSTUNREACH   : return 113;
    case EALREADY       : return 114;
    case EINPROGRESS    : return 115;
    case ESTALE         : return 116;
    case EDQUOT         : return 122;
    case ECANCELED      : return 125;
    case EOWNERDEAD     : return 130;
    case ENOTRECOVERABLE: return 131;
    default:
        // FIXME: Eventually, we'll probably have to map weird host errnos. Or, just get rid of this
        // madness and have musl use system errno's.
        PAS_ASSERT(!"Bad errno value");
        return 0;
    }
}

static PAS_NEVER_INLINE void set_errno(int errno_value)
{
    set_musl_errno(to_musl_errno(errno_value));
}

struct musl_winsize { unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel; };

static int zsys_ioctl_impl(int fd, unsigned long request, deluge_ptr args)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_ioctl_impl",
        .line = 0,
        .column = 0
    };
    // NOTE: This must use musl's ioctl numbers, and must treat the passed-in struct as having the
    // deluded musl layout.
    switch (request) {
    case 0x5413: { // TIOCGWINSZ
        deluge_ptr musl_winsize_ptr;
        struct musl_winsize* musl_winsize;
        struct winsize winsize;
        musl_winsize_ptr = deluge_ptr_get_next_ptr(&args, &origin);
        deluge_check_access_int(musl_winsize_ptr, sizeof(struct musl_winsize), &origin);
        musl_winsize = (struct musl_winsize*)deluge_ptr_ptr(musl_winsize_ptr);
        if (ioctl(fd, TIOCGWINSZ, &winsize) < 0) {
            set_errno(errno);
            return -1;
        }
        musl_winsize->ws_row = winsize.ws_row;
        musl_winsize->ws_col = winsize.ws_col;
        musl_winsize->ws_xpixel = winsize.ws_xpixel;
        musl_winsize->ws_ypixel = winsize.ws_ypixel;
        return 0;
    }
    default:
        set_errno(EINVAL);
        return -1;
    }
}

void deluded_f_zsys_ioctl(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_ioctl",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    unsigned long request = deluge_ptr_get_next_unsigned_long(&args, &origin);
    int result = zsys_ioctl_impl(fd, request, args);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = result;
}

struct musl_iovec { deluge_ptr iov_base; size_t iov_len; };

static struct iovec* prepare_iovec(deluge_ptr musl_iov, int iovcnt)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "prepare_iovec",
        .line = 0,
        .column = 0
    };
    struct iovec* iov;
    size_t iov_size;
    size_t index;
    DELUGE_CHECK(
        iovcnt >= 0,
        &origin,
        "iovcnt cannot be negative; iovcnt = %d.\n", iovcnt);
    DELUGE_CHECK(
        !pas_mul_uintptr_overflow(sizeof(struct iovec), iovcnt, &iov_size),
        &origin,
        "iovcnt too large, leading to overflow; iovcnt = %d.\n", iovcnt);
    iov = deluge_allocate_utility(iov_size);
    for (index = 0; index < (size_t)iovcnt; ++index) {
        deluge_ptr musl_iov_entry;
        deluge_ptr musl_iov_base;
        size_t iov_len;
        musl_iov_entry = deluge_ptr_with_offset(musl_iov, sizeof(struct musl_iovec) * index);
        deluge_check_access_ptr(
            deluge_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_base)),
            &origin);
        deluge_check_access_int(
            deluge_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_len)),
            sizeof(size_t), &origin);
        musl_iov_base = ((struct musl_iovec*)deluge_ptr_ptr(musl_iov_entry))->iov_base;
        iov_len = ((struct musl_iovec*)deluge_ptr_ptr(musl_iov_entry))->iov_len;
        deluge_check_access_int(musl_iov_base, iov_len, &origin);
        iov[index].iov_base = deluge_ptr_ptr(musl_iov_base);
        iov[index].iov_len = iov_len;
    }
    return iov;
}

void deluded_f_zsys_writev(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_writev",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr musl_iov = deluge_ptr_get_next_ptr(&args, &origin);
    int iovcnt = deluge_ptr_get_next_int(&args, &origin);
    ssize_t result;
    DELUDED_DELETE_ARGS();
    struct iovec* iov = prepare_iovec(musl_iov, iovcnt);
    result = writev(fd, iov, iovcnt);
    if (result < 0)
        set_errno(errno);
    deluge_deallocate(iov);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_read(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_read",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr buf = deluge_ptr_get_next_ptr(&args, &origin);
    ssize_t size = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(buf, size, &origin);
    int result = read(fd, deluge_ptr_ptr(buf), size);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_readv(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_readv",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr musl_iov = deluge_ptr_get_next_ptr(&args, &origin);
    int iovcnt = deluge_ptr_get_next_int(&args, &origin);
    ssize_t result;
    DELUDED_DELETE_ARGS();
    struct iovec* iov = prepare_iovec(musl_iov, iovcnt);
    result = readv(fd, iov, iovcnt);
    if (result < 0)
        set_errno(errno);
    deluge_deallocate(iov);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_write(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_write",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr buf = deluge_ptr_get_next_ptr(&args, &origin);
    ssize_t size = deluge_ptr_get_next_size_t(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(buf, size, &origin);
    int result = write(fd, deluge_ptr_ptr(buf), size);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_close(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_close",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    int result = close(fd);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_lseek(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_lseek",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    long offset = deluge_ptr_get_next_long(&args, &origin);
    int whence = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    long result = lseek(fd, offset, whence);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(long), &origin);
    *(long*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_exit(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_exit",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    int return_code = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    _exit(return_code);
    PAS_ASSERT(!"Should not be reached");
}

static int from_musl_signum(int signum)
{
    switch (signum) {
    case 1: return SIGHUP;
    case 2: return SIGINT;
    case 3: return SIGQUIT;
    case 4: return SIGILL;
    case 5: return SIGTRAP;
    case 6: return SIGABRT;
    case 7: return SIGBUS;
    case 8: return SIGFPE;
    case 9: return SIGKILL;
    case 10: return SIGUSR1;
    case 11: return SIGSEGV;
    case 12: return SIGUSR2;
    case 13: return SIGPIPE;
    case 14: return SIGALRM;
    case 15: return SIGTERM;
    case 17: return SIGCHLD;
    case 18: return SIGCONT;
    case 19: return SIGSTOP;
    case 20: return SIGTSTP;
    case 21: return SIGTTIN;
    case 22: return SIGTTOU;
    case 23: return SIGURG;
    case 24: return SIGXCPU;
    case 25: return SIGXFSZ;
    case 26: return SIGVTALRM;
    case 27: return SIGPROF;
    case 28: return SIGWINCH;
    case 29: return SIGIO;
    case 31: return SIGSYS;
    default: return -1;
    }
}

static void check_signal(int signum, const deluge_origin* origin)
{
    DELUGE_CHECK(signum != SIGILL, origin, "cannot override SIGILL.");
    DELUGE_CHECK(signum != SIGTRAP, origin, "cannot override SIGTRAP.");
    DELUGE_CHECK(signum != SIGBUS, origin, "cannot override SIGBUS.");
    DELUGE_CHECK(signum != SIGSEGV, origin, "cannot override SIGSEGV.");
}

static void* from_musl_signal_handler(void* handler, const deluge_origin* origin)
{
    DELUGE_CHECK(
        handler == NULL || handler == (void*)(uintptr_t)1,
        origin,
        "signal handler dan only be SIG_DFL or SIG_IGN.");
    return handler == NULL ? SIG_DFL : SIG_IGN;
}

static deluge_ptr to_musl_signal_handler(void* handler)
{
    if (handler == SIG_DFL)
        return deluge_ptr_forge_invalid(NULL);
    if (handler == SIG_IGN)
        return deluge_ptr_forge_invalid((void*)(uintptr_t)1);
    return deluge_ptr_forge_invalid(handler);
}

void deluded_f_zsys_signal(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_signal",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int musl_signum = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr sighandler = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    int signum = from_musl_signum(musl_signum);
    if (signum < 0) {
        set_errno(EINVAL);
        *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_invalid((void*)(intptr_t)-1);
        return;
    }
    check_signal(signum, &origin);
    void* result = signal(signum, from_musl_signal_handler(deluge_ptr_ptr(sighandler), &origin));
    if (result == SIG_ERR) {
        *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_invalid((void*)(intptr_t)-1);
        set_errno(errno);
        return;
    }
    *(deluge_ptr*)deluge_ptr_ptr(rets) = to_musl_signal_handler(result);
}

void deluded_f_zsys_getuid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getuid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(unsigned), &origin);
    *(unsigned*)deluge_ptr_ptr(rets) = getuid();
}

void deluded_f_zsys_geteuid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_geteuid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(unsigned), &origin);
    *(unsigned*)deluge_ptr_ptr(rets) = geteuid();
}

void deluded_f_zsys_getgid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getgid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(unsigned), &origin);
    *(unsigned*)deluge_ptr_ptr(rets) = getgid();
}

void deluded_f_zsys_getegid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getegid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(unsigned), &origin);
    *(unsigned*)deluge_ptr_ptr(rets) = getegid();
}

static bool check_and_clear(int* flags, int expected)
{
    if ((*flags & expected) == expected) {
        *flags &= ~expected;
        return true;
    }
    return false;
}

static int from_musl_open_flags(int musl_flags)
{
    int result = 0;

    if (check_and_clear(&musl_flags, 01))
        result |= O_WRONLY;
    if (check_and_clear(&musl_flags, 02))
        result |= O_RDWR;
    if (check_and_clear(&musl_flags, 0100))
        result |= O_CREAT;
    if (check_and_clear(&musl_flags, 0200))
        result |= O_EXCL;
    if (check_and_clear(&musl_flags, 0400))
        result |= O_NOCTTY;
    if (check_and_clear(&musl_flags, 01000))
        result |= O_TRUNC;
    if (check_and_clear(&musl_flags, 02000))
        result |= O_APPEND;
    if (check_and_clear(&musl_flags, 04000))
        result |= O_NONBLOCK;
    if (check_and_clear(&musl_flags, 0200000))
        result |= O_DIRECTORY;
    if (check_and_clear(&musl_flags, 0400000))
        result |= O_NOFOLLOW;
    if (check_and_clear(&musl_flags, 02000000))
        result |= O_CLOEXEC;
    if (check_and_clear(&musl_flags, 020000))
        result |= O_ASYNC;

    if (musl_flags)
        return -1;
    return result;
}

static int to_musl_open_flags(int flags)
{
    int result = 0;

    if (check_and_clear(&flags, O_WRONLY))
        result |= 01;
    if (check_and_clear(&flags, O_RDWR))
        result |= 02;
    if (check_and_clear(&flags, O_CREAT))
        result |= 0100;
    if (check_and_clear(&flags, O_EXCL))
        result |= 0200;
    if (check_and_clear(&flags, O_NOCTTY))
        result |= 0400;
    if (check_and_clear(&flags, O_TRUNC))
        result |= 01000;
    if (check_and_clear(&flags, O_APPEND))
        result |= 02000;
    if (check_and_clear(&flags, O_NONBLOCK))
        result |= 04000;
    if (check_and_clear(&flags, O_DIRECTORY))
        result |= 0200000;
    if (check_and_clear(&flags, O_NOFOLLOW))
        result |= 0400000;
    if (check_and_clear(&flags, O_CLOEXEC))
        result |= 02000000;
    if (check_and_clear(&flags, O_ASYNC))
        result |= 020000;

    if (flags)
        return -1;
    return result;
}

void deluded_f_zsys_open(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_open",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr path_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    int musl_flags = deluge_ptr_get_next_int(&args, &origin);
    int flags = from_musl_open_flags(musl_flags);
    int mode = 0;
    if (flags >= 0 && (flags & O_CREAT))
        mode = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    if (flags < 0) {
        set_errno(EINVAL);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    int result = open(deluge_check_and_get_str(path_ptr, &origin), flags, mode);
    if (result < 0)
        set_errno(errno);
    *(int*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_getpid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = getpid();
}

static bool from_musl_clock_id(int musl_clock_id, clockid_t* result)
{
    switch (musl_clock_id) {
    case 0:
        *result = CLOCK_REALTIME;
        return true;
    case 1:
        *result = CLOCK_MONOTONIC;
        return true;
    case 2:
        *result = CLOCK_PROCESS_CPUTIME_ID;
        return true;
    case 3:
        *result = CLOCK_THREAD_CPUTIME_ID;
        return true;
    case 4:
        *result = CLOCK_MONOTONIC_RAW;
        return true;
    default:
        *result = 0;
        return false;
    }
}

struct musl_timespec {
    uint64_t tv_sec;
    uint64_t tv_nsec;
};

void deluded_f_zsys_clock_gettime(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_clock_gettime",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int musl_clock_id = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr timespec_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    deluge_check_access_int(timespec_ptr, sizeof(struct musl_timespec), &origin);
    clockid_t clock_id;
    if (!from_musl_clock_id(musl_clock_id, &clock_id)) {
        *(int*)deluge_ptr_ptr(rets) = -1;
        set_errno(EINVAL);
        return;
    }
    struct timespec ts;
    int result = clock_gettime(clock_id, &ts);
    if (result < 0) {
        *(int*)deluge_ptr_ptr(rets) = -1;
        set_errno(errno);
        return;
    }
    struct musl_timespec* musl_timespec = (struct musl_timespec*)deluge_ptr_ptr(timespec_ptr);
    musl_timespec->tv_sec = ts.tv_sec;
    musl_timespec->tv_nsec = ts.tv_nsec;
}

struct musl_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    unsigned st_mode;
    uint64_t st_nlink;
    unsigned st_uid;
    unsigned st_gid;
    uint64_t st_rdev;
    int64_t st_size;
    long st_blksize;
    int64_t st_blocks;
    uint64_t st_atim[2];
    uint64_t st_mtim[2];
    uint64_t st_ctim[2];
};

static bool from_musl_fstatat_flag(int musl_flag, int* result)
{
    *result = 0;
    if (check_and_clear(&musl_flag, 0x100))
        *result |= AT_SYMLINK_NOFOLLOW;
    if (check_and_clear(&musl_flag, 0x200))
        *result |= AT_REMOVEDIR | AT_EACCESS;
    if (check_and_clear(&musl_flag, 0x400))
        *result |= AT_SYMLINK_FOLLOW;
    return !musl_flag;
}

static void handle_fstat_result(deluge_ptr rets, deluge_ptr musl_stat_ptr, struct stat *st, int result)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "handle_fstat_result",
        .line = 0,
        .column = 0
    };
    deluge_check_access_int(rets, sizeof(int), &origin);
    deluge_check_access_int(musl_stat_ptr, sizeof(struct musl_stat), &origin);
    if (result < 0) {
        set_errno(errno);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    struct musl_stat* musl_stat = (struct musl_stat*)deluge_ptr_ptr(musl_stat_ptr);
    musl_stat->st_dev = st->st_dev;
    musl_stat->st_ino = st->st_ino;
    musl_stat->st_mode = st->st_mode;
    musl_stat->st_nlink = st->st_nlink;
    musl_stat->st_uid = st->st_uid;
    musl_stat->st_gid = st->st_gid;
    musl_stat->st_rdev = st->st_rdev;
    musl_stat->st_size = st->st_size;
    musl_stat->st_blksize = st->st_blksize;
    musl_stat->st_blocks = st->st_blocks;
    musl_stat->st_atim[0] = st->st_atimespec.tv_sec;
    musl_stat->st_atim[1] = st->st_atimespec.tv_nsec;
    musl_stat->st_mtim[0] = st->st_mtimespec.tv_sec;
    musl_stat->st_mtim[1] = st->st_mtimespec.tv_nsec;
    musl_stat->st_ctim[0] = st->st_ctimespec.tv_sec;
    musl_stat->st_ctim[1] = st->st_ctimespec.tv_nsec;
}

void deluded_f_zsys_fstatat(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fstatat",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr path_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr musl_stat_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    int musl_flag = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    const char* path = deluge_check_and_get_str(path_ptr, &origin);
    int flag;
    if (!from_musl_fstatat_flag(musl_flag, &flag)) {
        set_errno(EINVAL);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    if (fd == -100)
        fd = AT_FDCWD;
    struct stat st;
    int result = fstatat(fd, path, &st, flag);
    handle_fstat_result(rets, musl_stat_ptr, &st, result);
}

void deluded_f_zsys_fstat(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fstat",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr musl_stat_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    struct stat st;
    int result = fstat(fd, &st);
    handle_fstat_result(rets, musl_stat_ptr, &st, result);
}

struct musl_flock {
    short l_type;
    short l_whence;
    int64_t l_start;
    int64_t l_len;
    int l_pid;
};

void deluded_f_zsys_fcntl(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fcntl",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    int musl_cmd = deluge_ptr_get_next_int(&args, &origin);
    deluge_check_access_int(rets, sizeof(int), &origin);
    int cmd = 0;
    bool have_cmd = true;
    bool have_arg_int = false;
    bool have_arg_flock = false;
    switch (musl_cmd) {
    case 0:
        cmd = F_DUPFD;
        have_arg_int = true;
        break;
    case 1:
        cmd = F_GETFD;
        break;
    case 2:
        cmd = F_SETFD;
        have_arg_int = true;
        break;
    case 3:
        cmd = F_GETFL;
        break;
    case 4:
        cmd = F_SETFL;
        have_arg_int = true;
        break;
    case 8:
        cmd = F_SETOWN;
        have_arg_int = true;
        break;
    case 9:
        cmd = F_GETOWN;
        break;
    case 5:
        cmd = F_GETLK;
        have_arg_flock = true;
        break;
    case 6:
        cmd = F_SETLK;
        have_arg_flock = true;
        break;
    case 7:
        cmd = F_SETLKW;
        have_arg_flock = true;
        break;
    case 1030:
        cmd = F_DUPFD_CLOEXEC;
        have_arg_int = true;
        break;
    default:
        have_cmd = false;
        break;
    }
    int arg_int = 0;
    struct flock arg_flock;
    deluge_ptr musl_flock_ptr;
    struct musl_flock* musl_flock;
    pas_zero_memory(&arg_flock, sizeof(arg_flock));
    if (have_arg_int)
        arg_int = deluge_ptr_get_next_int(&args, &origin);
    if (have_arg_flock) {
        musl_flock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
        deluge_check_access_int(musl_flock_ptr, sizeof(struct musl_flock), &origin);
        musl_flock = (struct musl_flock*)deluge_ptr_ptr(musl_flock_ptr);
        switch (musl_flock->l_type) {
        case 0:
            arg_flock.l_type = F_RDLCK;
            break;
        case 1:
            arg_flock.l_type = F_WRLCK;
            break;
        case 2:
            arg_flock.l_type = F_UNLCK;
            break;
        default:
            set_errno(EINVAL);
            *(int*)deluge_ptr_ptr(rets) = -1;
            return;
        }
        arg_flock.l_whence = musl_flock->l_whence;
        arg_flock.l_start = musl_flock->l_start;
        arg_flock.l_len = musl_flock->l_len;
        arg_flock.l_pid = musl_flock->l_pid;
    }
    DELUDED_DELETE_ARGS();
    if (!have_cmd) {
        set_errno(EINVAL);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    switch (cmd) {
    case F_SETFD:
        switch (arg_int) {
        case 0:
            break;
        case 1:
            arg_int = FD_CLOEXEC;
            break;
        default:
            set_errno(EINVAL);
            *(int*)deluge_ptr_ptr(rets) = -1;
            return;
        }
        break;
    case F_SETFL:
        arg_int = from_musl_open_flags(arg_int);
        break;
    default:
        break;
    }
    int result;
    if (have_arg_int) {
        PAS_ASSERT(!have_arg_flock);
        result = fcntl(fd, cmd, arg_int);
    } else if (have_arg_flock) {
        struct flock saved_flock = arg_flock;
        result = fcntl(fd, cmd, &arg_flock);
        if (arg_flock.l_type != saved_flock.l_type) {
            switch (arg_flock.l_type) {
            case F_RDLCK:
                musl_flock->l_type = 0;
                break;
            case F_WRLCK:
                musl_flock->l_type = 1;
                break;
            case F_UNLCK:
                musl_flock->l_type = 2;
                break;
            default:
                /* WTF? */
                musl_flock->l_type = arg_flock.l_type;
                break;
            }
        }
#define SET_IF_DIFFERENT(field) do { \
        if (arg_flock.field != saved_flock.field) \
            musl_flock->field = arg_flock.field; \
    } while (false)
        SET_IF_DIFFERENT(l_whence);
        SET_IF_DIFFERENT(l_start);
        SET_IF_DIFFERENT(l_len);
        SET_IF_DIFFERENT(l_pid);
#undef SET_IF_DIFFERENT
    } else
        result = fcntl(fd, cmd);
    if (result == -1)
        set_errno(errno);
    switch (cmd) {
    case F_GETFD:
        switch (result) {
        case 0:
            break;
        case FD_CLOEXEC:
            result = 1;
            break;
        default:
            /* WTF? */
            break;
        }
        break;
    case F_GETFL:
        result = to_musl_open_flags(result);
        break;
    default:
        break;
    }
    *(int*)deluge_ptr_ptr(rets) = result;
}

static struct musl_passwd* to_musl_passwd_threadlocal(struct passwd* passwd)
{
    struct musl_passwd* result = (struct musl_passwd*)pthread_getspecific(musl_passwd_threadlocal_key);
    if (result)
        musl_passwd_free_guts(result);
    else {
        result = deluge_allocate_one(&musl_passwd_heap);
        pthread_setspecific(musl_passwd_threadlocal_key, result);
    }
    deluge_low_level_ptr_safe_bzero(result, sizeof(struct musl_passwd));

    deluge_ptr_store(&result->pw_name, deluge_strdup(passwd->pw_name));
    deluge_ptr_store(&result->pw_passwd, deluge_strdup(passwd->pw_passwd));
    result->pw_uid = passwd->pw_uid;
    result->pw_gid = passwd->pw_gid;
    deluge_ptr_store(&result->pw_gecos, deluge_strdup(passwd->pw_gecos));
    deluge_ptr_store(&result->pw_dir, deluge_strdup(passwd->pw_dir));
    deluge_ptr_store(&result->pw_shell, deluge_strdup(passwd->pw_shell));
    return result;
}

void deluded_f_zsys_getpwuid(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpwuid",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    unsigned uid = deluge_ptr_get_next_unsigned(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    struct passwd* passwd = getpwuid(uid);
    if (!passwd) {
        set_errno(errno);
        return;
    }
    struct musl_passwd* musl_passwd = to_musl_passwd_threadlocal(passwd);
    *(deluge_ptr*)deluge_ptr_ptr(rets) =
        deluge_ptr_forge(musl_passwd, musl_passwd, musl_passwd + 1, &musl_passwd_type);
}

static void from_musl_sigset(struct musl_sigset* musl_sigset,
                             sigset_t* sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    sigfillset(sigset);
    
    unsigned musl_signum;
    for (musl_signum = num_active_bits; musl_signum--;) {
        bool bit_value = !!(musl_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_signum)]
                            & PAS_BITVECTOR_BIT_MASK64(musl_signum));
        if (bit_value)
            continue;
        int signum = from_musl_signum(musl_signum);
        if (signum < 0)
            continue;
        sigdelset(sigset, signum);
    }
}

static bool from_musl_sa_flags(int musl_flags, int* flags)
{
    *flags = 0;
    if (check_and_clear(&musl_flags, 1))
        *flags |= SA_NOCLDSTOP;
    if (check_and_clear(&musl_flags, 2))
        *flags |= SA_NOCLDWAIT;
    if (check_and_clear(&musl_flags, 4))
        *flags |= SA_SIGINFO;
    if (check_and_clear(&musl_flags, 0x08000000))
        *flags |= SA_ONSTACK;
    if (check_and_clear(&musl_flags, 0x10000000))
        *flags |= SA_RESTART;
    if (check_and_clear(&musl_flags, 0x40000000))
        *flags |= SA_NODEFER;
    if (check_and_clear(&musl_flags, 0x80000000))
        *flags |= SA_RESETHAND;
    return !musl_flags;
}

static void to_musl_sigset(sigset_t* sigset, struct musl_sigset* musl_sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    memset(musl_sigset, 255, sizeof(struct musl_sigset));
    
    unsigned musl_signum;
    for (musl_signum = num_active_bits; musl_signum--;) {
        int signum = from_musl_signum(musl_signum);
        if (signum < 0)
            continue;
        if (!sigismember(sigset, signum)) {
            musl_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_signum)] &=
                ~PAS_BITVECTOR_BIT_MASK64(musl_signum);
        }
    }
}

static int to_musl_sa_flags(int sa_flags)
{
    int result = 0;
    if (check_and_clear(&sa_flags, SA_NOCLDSTOP))
        result |= 1;
    if (check_and_clear(&sa_flags, SA_NOCLDWAIT))
        result |= 2;
    if (check_and_clear(&sa_flags, SA_SIGINFO))
        result |= 4;
    if (check_and_clear(&sa_flags, SA_ONSTACK))
        result |= 0x08000000;
    if (check_and_clear(&sa_flags, SA_RESTART))
        result |= 0x10000000;
    if (check_and_clear(&sa_flags, SA_NODEFER))
        result |= 0x40000000;
    if (check_and_clear(&sa_flags, SA_RESETHAND))
        result |= 0x80000000;
    PAS_ASSERT(!sa_flags);
    return result;
}

void deluded_f_zsys_sigaction(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_sigaction",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int musl_signum = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr act_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr oact_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    int signum = from_musl_signum(musl_signum);
    if (signum < 0) {
        pas_log("bad signum\n");
        set_errno(EINVAL);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    check_signal(signum, &origin);
    if (deluge_ptr_ptr(act_ptr))
        deluge_restrict(act_ptr, 1, &musl_sigaction_type, &origin);
    if (deluge_ptr_ptr(oact_ptr))
        deluge_restrict(oact_ptr, 1, &musl_sigaction_type, &origin);
    struct musl_sigaction* musl_act = (struct musl_sigaction*)deluge_ptr_ptr(act_ptr);
    struct musl_sigaction* musl_oact = (struct musl_sigaction*)deluge_ptr_ptr(oact_ptr);
    struct sigaction act;
    struct sigaction oact;
    if (musl_act) {
        act.sa_handler = from_musl_signal_handler(
            deluge_ptr_ptr(deluge_ptr_load(&musl_act->sa_handler_ish)), &origin);
        from_musl_sigset(&musl_act->sa_mask, &act.sa_mask);
        if (!from_musl_sa_flags(musl_act->sa_flags, &act.sa_flags)) {
            set_errno(EINVAL);
            *(int*)deluge_ptr_ptr(rets) = -1;
            return;
        }
    }
    if (musl_oact)
        pas_zero_memory(&oact, sizeof(struct sigaction));
    int result = sigaction(signum, musl_act ? &act : NULL, musl_oact ? &oact : NULL);
    if (result < 0) {
        set_errno(errno);
        *(int*)deluge_ptr_ptr(rets) = 1;
        return;
    }
    if (musl_oact) {
        deluge_ptr_store(&musl_oact->sa_handler_ish, to_musl_signal_handler(oact.sa_handler));
        to_musl_sigset(&oact.sa_mask, &musl_oact->sa_mask);
        musl_oact->sa_flags = to_musl_sa_flags(oact.sa_flags);
    }
}

void deluded_f_zsys_isatty(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_isatty",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int fd = deluge_ptr_get_next_int(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    errno = 0;
    int result = isatty(fd);
    if (!result && errno)
        set_errno(errno);
    *(int*)deluge_ptr_ptr(rets) = result;
}

void deluded_f_zsys_pipe(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_pipe",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr fds_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int), &origin);
    deluge_check_access_int(fds_ptr, sizeof(int) * 2, &origin);
    int fds[2];
    int result = pipe(fds);
    if (result < 0) {
        /* Make sure not to modify what fds_ptr points to on error, even if the system modified
           our fds, since that would be nonconforming behavior. Probably doesn't matter since of
           course we would never run on a nonconforming system. */
        set_errno(errno);
        *(int*)deluge_ptr_ptr(rets) = -1;
        return;
    }
    ((int*)deluge_ptr_ptr(fds_ptr))[0] = fds[0];
    ((int*)deluge_ptr_ptr(fds_ptr))[1] = fds[1];
}

struct musl_timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
};

void deluded_f_zsys_select(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zsys_select",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int nfds = deluge_ptr_get_next_int(&args, &origin);
    deluge_ptr readfds_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr writefds_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr exceptfds_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr timeout_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    PAS_ASSERT(FD_SETSIZE == 1024);
    DELUGE_CHECK(
        nfds <= 1024,
        &origin,
        "attempt to select with nfds = %d (should be 1024 or less).",
        nfds);
    if (deluge_ptr_ptr(readfds_ptr))
        deluge_check_access_int(readfds_ptr, sizeof(fd_set), &origin);
    if (deluge_ptr_ptr(writefds_ptr))
        deluge_check_access_int(writefds_ptr, sizeof(fd_set), &origin);
    if (deluge_ptr_ptr(exceptfds_ptr))
        deluge_check_access_int(exceptfds_ptr, sizeof(fd_set), &origin);
    if (deluge_ptr_ptr(timeout_ptr))
        deluge_check_access_int(timeout_ptr, sizeof(struct musl_timeval), &origin);
    fd_set* readfds = (fd_set*)deluge_ptr_ptr(readfds_ptr);
    fd_set* writefds = (fd_set*)deluge_ptr_ptr(writefds_ptr);
    fd_set* exceptfds = (fd_set*)deluge_ptr_ptr(exceptfds_ptr);
    struct musl_timeval* musl_timeout = (struct musl_timeval*)deluge_ptr_ptr(timeout_ptr);
    struct timeval timeout;
    if (musl_timeout) {
        timeout.tv_sec = musl_timeout->tv_sec;
        timeout.tv_usec = musl_timeout->tv_usec;
    }
    int result = select(nfds, readfds, writefds, exceptfds, musl_timeout ? &timeout : NULL);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(int), &origin);
    *(int*)deluge_ptr_ptr(rets) = result;
    if (musl_timeout) {
        musl_timeout->tv_sec = timeout.tv_sec;
        musl_timeout->tv_usec = timeout.tv_usec;
    }
}

typedef struct {
    thread_specific* parent;
    uint64_t version;
    deluge_ptr value;
} thread_specific_value;

static void my_destructor(void* untyped_value)
{
    thread_specific_value* value;
    void (*destructor)(DELUDED_SIGNATURE);

    value = (thread_specific_value*)untyped_value;
    if (!value)
        return;
    
    destructor = value->parent->destructor;
    if (destructor && value->version == value->parent->version) {
        deluge_ptr* args;
        uintptr_t return_buffer[2];
        
        args = deluge_allocate_one(deluge_get_heap(&deluge_one_ptr_type));
        deluge_ptr_store(args, deluge_ptr_load(&value->value));
        destructor(args, args + 1, &deluge_one_ptr_type,
                   return_buffer, return_buffer + 2, &deluge_int_type);
    }
    
    deluge_deallocate(value);
}

void deluded_f_zthread_key_create(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_key_create",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr destructor_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    if (deluge_ptr_ptr(destructor_ptr))
        deluge_check_function_call(destructor_ptr, &origin);

    thread_specific* result = deluge_try_allocate_one(&thread_specific_heap);
    if (!result)
        return;

    uint64_t version = result->version;
    pas_fence();
    if (!version) {
        pas_lock_lock(&result->lock);
        version = result->version;
        pas_fence();
        if (!version) {
            int my_errno;
            my_errno = pthread_key_create(&result->key, my_destructor);
            if (my_errno) {
                set_errno(my_errno);
                pas_lock_unlock(&result->lock);
                deluge_deallocate(result);
                return;
            }
            pas_fence();
            result->version = 1;
        }
        pas_lock_unlock(&result->lock);
    }

    result->destructor = (void(*)(DELUDED_SIGNATURE))deluge_ptr_ptr(destructor_ptr);

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte(result, &thread_specific_type);
}

void deluded_f_zthread_key_delete(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_key_delete",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr thread_specific_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_opaque(thread_specific_ptr, &thread_specific_type, &origin);

    thread_specific* specific = (thread_specific*)deluge_ptr_ptr(thread_specific_ptr);
    specific->version++; /* This can be racy, since:
                            
                            - Users aren't supposed to use it in a racy way, so any memory safe
                              outcome is acceptable if they do.

                            - Version has no memory safety implications. */

    deluge_deallocate(specific);
}

void deluded_f_zthread_setspecific(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_setspecific",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr thread_specific_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr value = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(thread_specific_ptr, &thread_specific_type, &origin);

    thread_specific* specific = (thread_specific*)deluge_ptr_ptr(thread_specific_ptr);
    thread_specific_value* specific_value = pthread_getspecific(specific->key);
    if (!specific_value) {
        int my_errno;
        specific_value = deluge_allocate_utility(sizeof(thread_specific_value));
        specific_value->parent = specific;
        specific_value->version = specific->version;
        my_errno = pthread_setspecific(specific->key, specific_value);
        if (my_errno) {
            set_errno(my_errno);
            return;
        }
    }

    deluge_ptr_store(&specific_value->value, value);
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_getspecific(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_getspecific",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr thread_specific_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(thread_specific_ptr, &thread_specific_type, &origin);

    thread_specific* specific = (thread_specific*)deluge_ptr_ptr(thread_specific_ptr);
    thread_specific_value* value = pthread_getspecific(specific->key);
    if (!value)
        return;

    if (value->version != value->parent->version)
        return;

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_load(&value->value);
}

void deluded_f_zthread_rwlock_create(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_create",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();

    pthread_rwlock_t* result = (pthread_rwlock_t*)deluge_try_allocate_one(&rwlock_heap);
    if (!result)
        return;

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte(result, &rwlock_type);
}

void deluded_f_zthread_rwlock_delete(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_delete",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    deluge_deallocate(rwlock);
}

void deluded_f_zthread_rwlock_rdlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_rdlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    int my_errno = pthread_rwlock_rdlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_rwlock_tryrdlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_tryrdlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    int my_errno = pthread_rwlock_tryrdlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_rwlock_wrlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_wrlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    int my_errno = pthread_rwlock_wrlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_rwlock_trywrlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_trywrlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    int my_errno = pthread_rwlock_trywrlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_rwlock_unlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_rwlock_unlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr rwlock_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(rwlock_ptr, &rwlock_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)deluge_ptr_ptr(rwlock_ptr);
    int my_errno = pthread_rwlock_unlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_mutex_create(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_mutex_create",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();

    pthread_mutex_t* result = (pthread_mutex_t*)deluge_try_allocate_one(&mutex_heap);
    if (!result)
        return;

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte(result, &mutex_type);
}

void deluded_f_zthread_mutex_delete(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_mutex_delete",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr mutex_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(mutex_ptr, &mutex_type, &origin);

    pthread_mutex_t* mutex = (pthread_mutex_t*)deluge_ptr_ptr(mutex_ptr);
    deluge_deallocate(mutex);
}

void deluded_f_zthread_mutex_lock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_mutex_lock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr mutex_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(mutex_ptr, &mutex_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_mutex_t* mutex = (pthread_mutex_t*)deluge_ptr_ptr(mutex_ptr);
    int my_errno = pthread_mutex_lock(mutex);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_mutex_trylock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_mutex_trylock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr mutex_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(mutex_ptr, &mutex_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_mutex_t* mutex = (pthread_mutex_t*)deluge_ptr_ptr(mutex_ptr);
    int my_errno = pthread_mutex_trylock(mutex);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_mutex_unlock(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_mutex_unlock",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr mutex_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();

    deluge_check_access_opaque(mutex_ptr, &mutex_type, &origin);
    deluge_check_access_int(rets, sizeof(bool), &origin);

    pthread_mutex_t* mutex = (pthread_mutex_t*)deluge_ptr_ptr(mutex_ptr);
    int my_errno = pthread_mutex_unlock(mutex);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)deluge_ptr_ptr(rets) = true;
}

void deluded_f_zthread_self(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_self",
        .line = 0,
        .column = 0
    };
    deluge_ptr rets = DELUDED_RETS;
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    zthread* thread = pthread_getspecific(zthread_key);
    PAS_ASSERT(thread);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte(thread, &zthread_type);
}

static void* start_thread(void* arg)
{
    zthread* thread;

    thread = (zthread*)arg;

    PAS_ASSERT(thread->is_running);
    PAS_ASSERT(!pthread_setspecific(zthread_key, thread));

    deluge_ptr return_buffer;
    pas_zero_memory(&return_buffer, sizeof(return_buffer));

    deluge_ptr* args = deluge_allocate_one(deluge_get_heap(&deluge_one_ptr_type));
    deluge_ptr_store(args, deluge_ptr_load(&thread->arg_ptr));

    PAS_ASSERT(thread->callback);

    thread->callback(args, args + 1, &deluge_one_ptr_type,
                     &return_buffer, &return_buffer + 1, &deluge_one_ptr_type);

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(thread->is_running);
    deluge_ptr_store(&thread->result_ptr, return_buffer);
    thread->is_running = false;
    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock);
    return NULL;
}

void deluded_f_zthread_create(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_create",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr callback_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr arg_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets, &origin);
    deluge_check_function_call(callback_ptr, &origin);
    zthread* thread = (zthread*)deluge_allocate_one(&zthread_heap);
    pas_lock_lock(&thread->lock);
    /* I don't see how this could ever happen. */
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_joining);
    PAS_ASSERT(!thread->is_running);
    PAS_ASSERT(should_destroy_thread(thread));
    thread->callback = (void (*)(DELUDED_SIGNATURE))deluge_ptr_ptr(callback_ptr);
    deluge_ptr_store(&thread->arg_ptr, arg_ptr);
    thread->is_running = true;
    int result = pthread_create(&thread->thread, NULL, start_thread, thread);
    if (result) {
        int my_errno = result;
        PAS_ASSERT(!thread->thread);
        thread->is_running = false;
        PAS_ASSERT(should_destroy_thread(thread));
        deluge_deallocate(thread);
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_invalid(NULL);
        return;
    }
    PAS_ASSERT(thread->thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);
    *(deluge_ptr*)deluge_ptr_ptr(rets) = deluge_ptr_forge_byte(thread, &zthread_type);
}

void deluded_f_zthread_join(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_join",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr thread_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    deluge_ptr result_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(bool), &origin);
    deluge_check_access_opaque(thread_ptr, &zthread_type, &origin);
    if (deluge_ptr_ptr(result_ptr))
        deluge_check_access_ptr(result_ptr, &origin);
    zthread* thread = (zthread*)deluge_ptr_ptr(thread_ptr);
    pas_lock_lock(&thread->lock);
    DELUGE_CHECK(
        thread->thread,
        &origin,
        "Cannot join thread that has already been joined or detached (thread = %p).",
        thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(!thread->is_joining);
    thread->is_joining = true;
    pthread_t system_thread = thread->thread;
    thread->thread = NULL;
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);

    int result = pthread_join(system_thread, NULL);
    if (result) {
        int my_errno = result;
        pas_lock_lock(&thread->lock);
        PAS_ASSERT(!should_destroy_thread(thread));
        PAS_ASSERT(thread->is_joining);
        PAS_ASSERT(!thread->thread);
        thread->is_joining = false;
        thread->thread = system_thread;
        PAS_ASSERT(!should_destroy_thread(thread));
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(bool*)deluge_ptr_ptr(rets) = false;
        return;
    }

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(thread->is_joining);
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_running);
    thread->is_joining = false;
    if (deluge_ptr_ptr(result_ptr))
        deluge_ptr_store((deluge_ptr*)deluge_ptr_ptr(result_ptr), thread->result_ptr);
    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock);
    *(bool*)deluge_ptr_ptr(rets) = true;
    return;
}

void deluded_f_zthread_detach(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zthread_detach",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr thread_ptr = deluge_ptr_get_next_ptr(&args, &origin);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(bool), &origin);
    deluge_check_access_opaque(thread_ptr, &zthread_type, &origin);
    zthread* thread = (zthread*)deluge_ptr_ptr(thread_ptr);
    pas_lock_lock(&thread->lock);
    DELUGE_CHECK(
        thread->thread,
        &origin,
        "Cannot detach thread that has already been joined or detached (thread = %p).",
        thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(!thread->is_joining);
    pthread_t system_thread = thread->thread;
    thread->is_joining = true;
    thread->thread = NULL;
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);

    int result = pthread_detach(system_thread);
    if (result) {
        int my_errno = result;
        pas_lock_lock(&thread->lock);
        PAS_ASSERT(!should_destroy_thread(thread));
        PAS_ASSERT(thread->is_joining);
        PAS_ASSERT(!thread->thread);
        thread->is_joining = false;
        thread->thread = system_thread;
        PAS_ASSERT(!should_destroy_thread(thread));
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(bool*)deluge_ptr_ptr(rets) = false;
        return;
    }

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(thread->is_joining);
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_running);
    thread->is_joining = false;
    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock);
    *(bool*)deluge_ptr_ptr(rets) = true;
    return;
}

#endif /* PAS_ENABLE_DELUGE */

#endif /* LIBPAS_ENABLED */

