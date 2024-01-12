#include "pas_config.h"

#if LIBPAS_ENABLED

#include "deluge_runtime.h"

#if PAS_ENABLE_DELUGE

#include "deluge_heap_config.h"
#include "deluge_heap_innards.h"
#include "deluge_type_table.h"
#include "pas_deallocate.h"
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

const deluge_type deluge_int_type = {
    .size = 1,
    .alignment = 1,
    .num_words = 1,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = { DELUGE_WORD_TYPE_INT }
};

const deluge_type deluge_ptr_type = {
    .size = sizeof(deluge_ptr),
    .alignment = alignof(deluge_ptr),
    .num_words = 4,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = {
        DELUGE_WORD_TYPE_PTR_PART1,
        DELUGE_WORD_TYPE_PTR_PART2,
        DELUGE_WORD_TYPE_PTR_PART3,
        DELUGE_WORD_TYPE_PTR_PART4
    }
};

const deluge_type deluge_function_type = {
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

const deluge_type deluge_type_type = {
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

deluge_type_table deluge_type_table_instance = PAS_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable deluge_fast_type_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;

PAS_DEFINE_LOCK(deluge_type);
PAS_DEFINE_LOCK(deluge_global_initialization);

static void* allocate_utility_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return deluge_allocate_utility(size);
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
        DELUGE_ASSERT(!(type->size % type->alignment), origin);
    } else
        DELUGE_ASSERT(!type->alignment, origin);
    if (type->num_words) {
        DELUGE_ASSERT((type->size + 7) / 8 == type->num_words, origin);
        if (type->u.trailing_array) {
            DELUGE_ASSERT(type->u.trailing_array->num_words, origin);
            DELUGE_ASSERT(!type->u.trailing_array->u.trailing_array, origin);
            DELUGE_ASSERT(type->u.trailing_array->size, origin);
            DELUGE_ASSERT(type->u.trailing_array->alignment <= type->alignment, origin);
            deluge_validate_type(type->u.trailing_array, origin);
        }
    }
}

bool deluge_type_is_equal(const deluge_type* a, const deluge_type* b)
{
    size_t index;
    if (a == b)
        return true;
    if (a->num_words != b->num_words)
        return false;
    if (!a->num_words)
        return false;
    if (a->size != b->size)
        return false;
    if (a->alignment != b->alignment)
        return false;
    if (a->u.trailing_array) {
        if (!b->u.trailing_array)
            return false;
        return deluge_type_is_equal(a->u.trailing_array, b->u.trailing_array);
    } else if (b->u.trailing_array)
        return false;
    for (index = a->num_words; index--;) {
        if (a->word_types[index] != b->word_types[index])
            return false;
    }
    return true;
}

unsigned deluge_type_hash(const deluge_type* type)
{
    unsigned result;
    size_t index;
    if (!type->num_words)
        return pas_hash_ptr(type);
    result = type->size + 3 * type->alignment;
    if (type->u.trailing_array)
        result += 7 * deluge_type_hash(type->u.trailing_array);
    for (index = type->num_words; index--;) {
        result *= 11;
        result += type->word_types[index];
    }
    return result;
}

void deluge_word_type_dump(deluge_word_type type, pas_stream* stream)
{
    switch (type) {
    case DELUGE_WORD_TYPE_OFF_LIMITS:
        pas_stream_printf(stream, "//");
        return;
    case DELUGE_WORD_TYPE_INT:
        pas_stream_printf(stream, "In");
        return;
    case DELUGE_WORD_TYPE_PTR_PART1:
    case DELUGE_WORD_TYPE_PTR_PART2:
    case DELUGE_WORD_TYPE_PTR_PART3:
    case DELUGE_WORD_TYPE_PTR_PART4:
        pas_stream_printf(stream, "P%u", type - DELUGE_WORD_TYPE_PTR_PART1 + 1);
        return;
    default:
        pas_stream_printf(stream, "?%u", type);
        return;
    }
}

void deluge_type_dump(const deluge_type* type, pas_stream* stream)
{
    static const bool dump_ptr = false;
    static const bool dump_only_ptr = false;
    
    size_t index;

    if (dump_only_ptr) {
        pas_stream_printf(stream, "%p", type);
        return;
    }

    if (dump_ptr)
        pas_stream_printf(stream, "%p:", type);

    if (!type) {
        pas_stream_printf(stream, "delty{null}");
        return;
    }

    if (type == &deluge_function_type) {
        pas_stream_printf(stream, "delty{function}");
        return;
    }

    if (type == &deluge_type_type) {
        pas_stream_printf(stream, "delty{type}");
        return;
    }

    if (type == &deluge_int_type) {
        pas_stream_printf(stream, "delty{int}");
        return;
    }

    if (!type->num_words) {
        pas_stream_printf(stream, "delty{unique:%p", type);
        if (type->size)
            pas_stream_printf(stream, ",%zu,%zu", type->size, type->alignment);
        if (type->u.runtime_config)
            pas_stream_printf(stream, ",%p", type->u.runtime_config);
        pas_stream_printf(stream, "}");
        return;
    }
    
    pas_stream_printf(stream, "delty{%zu,%zu,", type->size, type->alignment);
    if (type->u.trailing_array) {
        deluge_type_dump(type->u.trailing_array, stream);
        pas_stream_printf(stream, ",");
    }
    for (index = 0; index < type->num_words; ++index)
        deluge_word_type_dump(type->word_types[index], stream);
    pas_stream_printf(stream, "}");
}

pas_heap_runtime_config* deluge_type_as_heap_type_get_runtime_config(
    const pas_heap_type* heap_type, pas_heap_runtime_config* config)
{
    const deluge_type* type = (const deluge_type*)heap_type;
    if (!type->num_words && type->u.runtime_config)
        return &type->u.runtime_config->base;
    return config;
}

void deluge_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream)
{
    deluge_type_dump((const deluge_type*)type, stream);
}

char* deluge_type_to_new_string(const deluge_type* type)
{
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    initialize_utility_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    deluge_type_dump(type, &stream.base);
    return pas_string_stream_take_string(&stream);
}

static pas_heap_ref* get_heap_impl(const deluge_type* type)
{
    pas_allocation_config allocation_config;
    deluge_type_table_add_result add_result;

    PAS_ASSERT(type != &deluge_int_type);
    PAS_ASSERT(type->size);
    PAS_ASSERT(type->alignment);
    deluge_validate_type(type, NULL);
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = deluge_type_table_add(&deluge_type_table_instance, type, NULL, &allocation_config);
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

pas_heap_ref* deluge_get_heap(const deluge_type* type)
{
    static const bool verbose = false;
    pas_heap_ref* result;
    if (verbose) {
        pas_log("Getting heap for ");
        deluge_type_dump(type, &pas_log_stream.base);
        pas_log("\n");
    }
    result = (pas_heap_ref*)pas_lock_free_read_ptr_ptr_hashtable_find(
        &deluge_fast_type_table, fast_type_hash, NULL, type);
    if (result)
        return result;
    deluge_type_lock_lock();
    result = get_heap_impl(type);
    deluge_type_lock_unlock();
    pas_heap_lock_lock();
    pas_lock_free_read_ptr_ptr_hashtable_set(
        &deluge_fast_type_table, fast_type_hash, NULL, type, result,
        pas_lock_free_read_ptr_ptr_hashtable_set_maybe_existing);
    pas_heap_lock_unlock();
    return result;
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

void* deluge_try_allocate_int(size_t size)
{
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

void* deluge_try_allocate_int_with_alignment(size_t size, size_t alignment)
{
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

void* deluge_allocate_int(size_t size)
{
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

void* deluge_allocate_int_with_alignment(size_t size, size_t alignment)
{
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
    return (void*)deluge_try_allocate_many_impl_by_count(ref, count, 1).begin;
}

void* deluge_try_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment)
{
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
    return (void*)deluge_allocate_many_impl_by_count(ref, count, 1).begin;
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
        return deluge_try_allocate_int(size);
    DELUGE_CHECK(
        !(size % type->size),
        &origin,
        "cannot allocate %zu bytes of type %s (have %zu remainder).",
        size, deluge_type_to_new_string(type), size % type->size);
    DELUGE_CHECK(
        type != &deluge_function_type && type != &deluge_type_type,
        &origin,
        "cannot allocate special type %s.",
        deluge_type_to_new_string(type));
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

void* deluge_try_reallocate_int(void* ptr, size_t size)
{
    return pas_try_reallocate_intrinsic(
        ptr,
        &deluge_int_heap,
        size,
        DELUGE_HEAP_CONFIG,
        deluge_try_allocate_int_impl_for_realloc,
        pas_reallocate_disallow_heap_teleport,
        pas_reallocate_free_if_successful);
}

void* deluge_try_reallocate_int_with_alignment(void* ptr, size_t size, size_t alignment)
{
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

void deluded_f_zfree(DELUDED_SIGNATURE)
{
    static deluge_origin origin = {
        .filename = __FILE__,
        .function = "zfree",
        .line = 0,
        .column = 0
    };
    deluge_ptr args = DELUDED_ARGS;
    deluge_deallocate(deluge_ptr_get_next_ptr(&args, &origin).ptr);
    DELUDED_DELETE_ARGS();
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
    if (__builtin_mul_overflow(left, right, (size_t*)result.ptr)) {
        return_value = false;
        set_errno(ENOMEM);
    } else
        return_value = true;
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)rets.ptr = return_value;
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
    *(deluge_ptr*)rets.ptr = deluge_ptr_forge(ptr.lower, ptr.lower, ptr.upper, ptr.type);
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
    *(deluge_ptr*)rets.ptr = deluge_ptr_forge(ptr.upper, ptr.lower, ptr.upper, ptr.type);
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
    *(deluge_ptr*)rets.ptr = deluge_ptr_forge((void*)ptr.type, (void*)ptr.type, (char*)ptr.type + 1, &deluge_type_type);
}

void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                              const deluge_origin* origin)
{
    static const bool verbose = false;

    if (verbose) {
        pas_log("validating ptr: %p,%p,%p,", ptr, lower, upper);
        if (type)
            deluge_type_dump(type, &pas_log_stream.base);
        else
            pas_log("%p", NULL);
        pas_log("\n");
    }
    
    if (!lower || !upper || !type) {
        /* It's possible for part of the ptr to fall off into a page that gets decommitted.
           In that case part of it will become null, and we'll treat the ptr as valid but
           inaccessible. For example, lower=0 always fails check_access_common. */
        return;
    }

    DELUGE_ASSERT(upper > lower, origin);
    DELUGE_ASSERT(upper <= (void*)PAS_MAX_ADDRESS, origin);
    if (type->size) {
        DELUGE_ASSERT(pas_is_aligned((uintptr_t)lower, type->alignment), origin);
        DELUGE_ASSERT(pas_is_aligned((uintptr_t)upper, type->alignment), origin);
        DELUGE_ASSERT(!((upper - lower) % type->size), origin);
    }
    deluge_validate_type(type, origin);
}

static void check_access_common_maybe_opaque(void* ptr, void* lower, void* upper, const deluge_type* type,
                                             uintptr_t bytes, const deluge_origin* origin)
{
    if (PAS_ENABLE_TESTING)
        deluge_validate_ptr_impl(ptr, lower, upper, type, origin);

    /* This check is necessary in case a wide pointer spans page boundary and the
       first of the two pages gets decommitted in a way that causes it to become
       zero.
       
       We could avoid this, probably a bunch of ways. Here are two ways:
       
       - Make wide pointers 32-byte aligned instead of 8-byte aligned. Hardware gonna
       like that better wanyway, if there's ever shit like atomic 32-byte loads and
       stores.
       
       - Require that decommit is Windows-style in that decommitted pages are not
       accessible.
       
       It seems like the alignment solution is strictly simpler.
       
       But also, it's just one little check. It almost certainly doesn't matter. */
    DELUGE_CHECK(
        lower,
        origin,
        "cannot access pointer with null lower (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        ptr >= lower,
        origin,
        "cannot access pointer with ptr < lower (ptr = %p,%p,%p,%s).", 
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        ptr < upper,
        origin,
        "cannot access pointer with ptr >= upper (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        bytes <= (uintptr_t)((char*)upper - (char*)ptr),
        origin,
        "cannot access %zu bytes when upper - ptr = %zu (ptr = %p,%p,%p,%s).",
        bytes, (size_t)((char*)upper - (char*)ptr),
        ptr, lower, upper, deluge_type_to_new_string(type));
        
    DELUGE_CHECK(
        type,
        origin,
        "cannot access ptr with null type (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

static void check_access_common(void* ptr, void* lower, void* upper, const deluge_type* type,
                                uintptr_t bytes, const deluge_origin* origin)
{
    check_access_common_maybe_opaque(ptr, lower, upper, type, bytes, origin);
    
    DELUGE_CHECK(
        type->num_words,
        origin,
        "cannot access %zu bytes, span has opaque type (ptr = %p,%p,%p,%s).\n",
        bytes, ptr, lower, upper, deluge_type_to_new_string(type));
}

static void check_int(void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes,
                      const deluge_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;

    if (type == &deluge_int_type)
        return;

    offset = (char*)ptr - (char*)lower;
    first_word_type_index = offset / 8;
    last_word_type_index = (offset + bytes - 1) / 8;

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        DELUGE_CHECK(
            deluge_type_get_word_type(type, word_type_index) == DELUGE_WORD_TYPE_INT,
            origin,
            "cannot access %zu bytes as int, span contains non-ints (ptr = %p,%p,%p,%s).",
            bytes, ptr, lower, upper, deluge_type_to_new_string(type));
    }
}

void deluge_check_access_int_impl(
    void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes,
    const deluge_origin* origin)
{
    check_access_common(ptr, lower, upper, type, bytes, origin);
    check_int(ptr, lower, upper, type, bytes, origin);
}

void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                                  const deluge_origin* origin)
{
    uintptr_t offset;
    uintptr_t word_type_index;

    check_access_common(ptr, lower, upper, type, 32, origin);

    offset = (char*)ptr - (char*)lower;
    DELUGE_CHECK(
        pas_is_aligned(offset, 8),
        origin,
        "cannot access memory as ptr without 8-byte alignment; in this case ptr %% 8 = %zu (ptr = %p,%p,%p,%s).",
        (size_t)(offset % 8), ptr, lower, upper, deluge_type_to_new_string(type));
    word_type_index = offset / 8;
    if (type->u.trailing_array) {
        uintptr_t num_words = type->num_words;
        if (word_type_index < num_words)
            DELUGE_ASSERT(word_type_index + 3 < num_words, origin);
        else {
            word_type_index -= num_words;
            type = type->u.trailing_array;
        }
    }
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 0) == DELUGE_WORD_TYPE_PTR_PART1,
        origin,
        "memory type error accessing ptr part1 (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 1) == DELUGE_WORD_TYPE_PTR_PART2,
        origin,
        "memory type error accessing ptr part2 (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 2) == DELUGE_WORD_TYPE_PTR_PART3,
        origin,
        "memory type error accessing ptr part3 (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 3) == DELUGE_WORD_TYPE_PTR_PART4,
        origin,
        "memory type error accessing ptr part4 (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

void deluge_check_function_call_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                                     const deluge_origin* origin)
{
    check_access_common_maybe_opaque(ptr, lower, upper, type, 1, origin);
    DELUGE_CHECK(
        type == &deluge_function_type,
        origin,
        "attempt to call pointer that is not a function (ptr = %p,%p,%p,%s).",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

void deluge_check_access_opaque(
    deluge_ptr ptr, const deluge_type* expected_type, const deluge_origin* origin)
{
    PAS_TESTING_ASSERT(!expected_type->num_words);
    check_access_common_maybe_opaque(ptr.ptr, ptr.lower, ptr.upper, ptr.type, 1, origin);
    DELUGE_CHECK(
        ptr.type == expected_type,
        origin,
        "expected ptr to %s during internal opaque access (ptr = %p,%p,%p,%s).",
        deluge_type_to_new_string(expected_type),
        ptr.ptr, ptr.lower, ptr.upper, deluge_type_to_new_string(ptr.type));
}

void deluge_memset_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                        unsigned value, size_t count, const deluge_origin* origin)
{
    if (!count)
        return;
    
    check_access_common(ptr, lower, upper, type, count, origin);
    
    if (!value) {
        if (type != &deluge_int_type) {
            uintptr_t offset;
            deluge_word_type word_type;
            
            offset = (char*)ptr - (char*)lower;
            word_type = deluge_type_get_word_type(type, offset / 8);
            if (offset % 8) {
                DELUGE_ASSERT(
                    word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
                    origin);
            } else {
                DELUGE_ASSERT(
                    word_type < DELUGE_WORD_TYPE_PTR_PART2 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
                    origin);
            }
            word_type = deluge_type_get_word_type(type, (offset + count - 1) / 8);
            if ((offset + count) % 8) {
                DELUGE_ASSERT(
                    word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
                    origin);
            } else {
                DELUGE_ASSERT(
                    word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART3,
                    origin);
            }
        }
        
        memset(ptr, 0, count);
        return;
    }

    check_int(ptr, lower, upper, type, count, origin);
    memset(ptr, value, count);
}

static void check_type_overlap(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                               void* src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                               size_t count, const deluge_origin* origin)
{
    uintptr_t dst_offset;
    uintptr_t src_offset;
    deluge_word_type word_type;
    uintptr_t num_word_types;
    uintptr_t word_type_index_offset;
    uintptr_t first_dst_word_type_index;
    uintptr_t first_src_word_type_index;

    if (dst_type == &deluge_int_type && src_type == &deluge_int_type)
        return;

    dst_offset = (char*)dst_ptr - (char*)dst_lower;
    src_offset = (char*)src_ptr - (char*)src_lower;
    if ((dst_offset % 8) != (src_offset % 8)) {
        /* No chance we could copy pointers if the offsets are skewed within a word.
           
           It would be harder to write a generalized checking algorithm for this case (the
           non-offset-skew version lower in this function can't handle it) and we don't need
           to, since that path could only succeed if there were only integers on both sides of
           the copy. */
        check_int(dst_ptr, dst_lower, dst_upper, dst_type, count, origin);
        check_int(src_ptr, src_lower, src_upper, src_type, count, origin);
        return;
    }

    num_word_types = (dst_offset + count - 1) / 8 - dst_offset / 8 + 1;
    first_dst_word_type_index = dst_offset / 8;
    first_src_word_type_index = src_offset / 8;

    for (word_type_index_offset = num_word_types; word_type_index_offset--;) {
        DELUGE_ASSERT(
            deluge_type_get_word_type(dst_type, first_dst_word_type_index + word_type_index_offset)
            ==
            deluge_type_get_word_type(src_type, first_src_word_type_index + word_type_index_offset),
            origin);
    }

    /* We cannot copy parts of pointers. */
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index);
    if (dst_offset % 8) {
        DELUGE_ASSERT(
            word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
            origin);
    } else {
        DELUGE_ASSERT(
            word_type < DELUGE_WORD_TYPE_PTR_PART2 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
            origin);
    }
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index + num_word_types - 1);
    if ((dst_offset + count) % 8) {
        DELUGE_ASSERT(
            word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4,
            origin);
    } else {
        DELUGE_ASSERT(
            word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART3,
            origin);
    }
}

static void check_copy(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                       void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                       size_t count, const deluge_origin* origin)
{
    check_access_common(dst_ptr, dst_lower, dst_upper, dst_type, count, origin);
    check_access_common(src_ptr, src_lower, src_upper, src_type, count, origin);
    check_type_overlap(dst_ptr, dst_lower, dst_upper, dst_type,
                       src_ptr, src_lower, src_upper, src_type,
                       count, origin);
}

void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                        size_t count, const deluge_origin* origin)
{
    if (!count)
        return;
    
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count, origin);
    
    memcpy(dst_ptr, src_ptr, count);
}

void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                         size_t count, const deluge_origin* origin)
{
    if (!count)
        return;
    
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count, origin);
    
    memmove(dst_ptr, src_ptr, count);
}

void deluge_check_restrict(void* ptr, void* lower, void* upper, const deluge_type* type,
                           void* new_upper, const deluge_type* new_type, const deluge_origin* origin)
{
    check_access_common(ptr, lower, upper, type, (char*)new_upper - (char*)ptr, origin);
    DELUGE_CHECK(new_type, origin, "cannot restrict to NULL type");
    check_type_overlap(ptr, ptr, new_upper, new_type,
                       ptr, lower, upper, type,
                       (char*)new_upper - (char*)ptr, origin);
}

const char* deluge_check_and_get_str(deluge_ptr str, const deluge_origin* origin)
{
    size_t available;
    size_t length;
    check_access_common(str.ptr, str.lower, str.upper, str.type, 1, origin);
    available = (char*)str.upper - (char*)str.ptr;
    length = strnlen((char*)str.ptr, available);
    PAS_ASSERT(length < available);
    PAS_ASSERT(length + 1 <= available);
    check_int(str.ptr, str.lower, str.upper, str.type, length + 1, origin);
    return (char*)str.ptr;
}

void* deluge_va_arg_impl(
    void* va_list_ptr, void* va_list_lower, void* va_list_upper, const deluge_type* va_list_type,
    size_t count, size_t alignment, const deluge_type* type, const deluge_origin* origin)
{
    deluge_ptr va_list;
    deluge_ptr* va_list_impl;
    void* result;
    va_list = deluge_ptr_forge(va_list_ptr, va_list_lower, va_list_upper, va_list_type);
    deluge_check_access_ptr(va_list, origin);
    va_list_impl = (deluge_ptr*)va_list.ptr;
    return deluge_ptr_get_next(va_list_impl, count, alignment, type, origin).ptr;
}

deluge_global_initialization_context* deluge_global_initialization_context_lock_and_find(
    deluge_global_initialization_context* context, void* global_getter)
{
    if (!context) {
        deluge_global_initialization_lock_lock();
        return NULL;
    }
    for (; context; context = context->outer) {
        if (context->global_getter == global_getter)
            return context;
    }
    return NULL;
}

void deluge_global_initialization_context_unlock(deluge_global_initialization_context* context)
{
    if (!context)
        deluge_global_initialization_lock_unlock();
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
    pas_string_stream_printf(&stream, "%p,%p,%p,", ptr.ptr, ptr.lower, ptr.upper);
    deluge_type_dump(ptr.type, &stream.base);
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
    *(int*)rets.ptr = strlen(str);
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
    *(deluge_ptr*)rets.ptr =
        deluge_ptr_forge(strchr(str, chr), str_ptr.lower, str_ptr.upper, str_ptr.type);
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
    *(int*)rets.ptr = isdigit(chr);
}

void deluded_f_zfence(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    DELUDED_DELETE_ARGS();
    pas_fence();
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
    deluded_errno_handler = (void(*)(DELUDED_SIGNATURE))errno_handler.ptr;
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
    args = (int*)deluge_allocate_int(sizeof(int));
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
        musl_winsize = (struct musl_winsize*)musl_winsize_ptr.ptr;
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
    *(int*)rets.ptr = result;
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
        musl_iov_base = ((struct musl_iovec*)musl_iov_entry.ptr)->iov_base;
        iov_len = ((struct musl_iovec*)musl_iov_entry.ptr)->iov_len;
        deluge_check_access_int(musl_iov_base, iov_len, &origin);
        iov[index].iov_base = musl_iov_base.ptr;
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
    *(ssize_t*)rets.ptr = result;
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
    int result = read(fd, buf.ptr, size);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)rets.ptr = result;
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
    *(ssize_t*)rets.ptr = result;
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
    int result = write(fd, buf.ptr, size);
    if (result < 0)
        set_errno(errno);
    deluge_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)rets.ptr = result;
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
    *(int*)rets.ptr = result;
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
    *(long*)rets.ptr = result;
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

#define DEFINE_RUNTIME_CONFIG(name, type, fresh_memory_constructor) \
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

static deluge_type thread_specific_type = {
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
        
        args = deluge_allocate_one(deluge_get_heap(&deluge_ptr_type));
        *args = value->value;
        destructor(args, args + 1, &deluge_ptr_type, return_buffer, return_buffer + 2, &deluge_int_type);
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

    if (destructor_ptr.ptr)
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

    result->destructor = (void(*)(DELUDED_SIGNATURE))destructor_ptr.ptr;

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)rets.ptr = deluge_ptr_forge(result, result, result + 1, &thread_specific_type);
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

    thread_specific* specific = (thread_specific*)thread_specific_ptr.ptr;
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

    thread_specific* specific = (thread_specific*)thread_specific_ptr.ptr;
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

    specific_value->value = value;
    deluge_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)rets.ptr = true;
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

    thread_specific* specific = (thread_specific*)thread_specific_ptr.ptr;
    thread_specific_value* value = pthread_getspecific(specific->key);
    if (!value)
        return;

    if (value->version != value->parent->version)
        return;

    deluge_check_access_ptr(rets, &origin);
    *(deluge_ptr*)rets.ptr = value->value;
}

static void rwlock_construct(pthread_rwlock_t* rwlock)
{
    int result = pthread_rwlock_init(rwlock, NULL);
    PAS_ASSERT(!result);
}

DEFINE_RUNTIME_CONFIG(rwlock_runtime_config, pthread_rwlock_t, rwlock_construct);

static deluge_type rwlock_type = {
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
    *(deluge_ptr*)rets.ptr = deluge_ptr_forge(result, result, result + 1, &rwlock_type);
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
    int my_errno = pthread_rwlock_rdlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)rets.ptr = true;
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
    int my_errno = pthread_rwlock_tryrdlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)rets.ptr = true;
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
    int my_errno = pthread_rwlock_wrlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)rets.ptr = true;
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
    int my_errno = pthread_rwlock_trywrlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)rets.ptr = true;
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

    pthread_rwlock_t* rwlock = (pthread_rwlock_t*)rwlock_ptr.ptr;
    int my_errno = pthread_rwlock_unlock(rwlock);
    if (my_errno) {
        set_errno(my_errno);
        return;
    }
    *(bool*)rets.ptr = true;
}

#endif /* PAS_ENABLE_DELUGE */

#endif /* LIBPAS_ENABLED */

