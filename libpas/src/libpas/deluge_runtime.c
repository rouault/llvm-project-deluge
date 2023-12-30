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

const deluge_type deluge_int_type = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { DELUGE_WORD_TYPE_INT }
};

const deluge_type deluge_function_type = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { DELUGE_WORD_TYPE_FUNCTION }
};

const deluge_type deluge_type_type = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { DELUGE_WORD_TYPE_TYPE }
};

deluge_type_table deluge_type_table_instance = PAS_HASHTABLE_INITIALIZER;

PAS_DEFINE_LOCK(deluge);

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

void deluge_validate_type(const deluge_type* type)
{
    if (deluge_type_is_equal(type, &deluge_int_type))
        PAS_ASSERT(type == &deluge_int_type);
    if (deluge_type_is_equal(type, &deluge_function_type))
        PAS_ASSERT(type == &deluge_function_type);
    if (deluge_type_is_equal(type, &deluge_type_type))
        PAS_ASSERT(type == &deluge_type_type);
    PAS_ASSERT(type->size);
    PAS_ASSERT(type->alignment);
    PAS_ASSERT(pas_is_power_of_2(type->alignment));
    PAS_ASSERT(!(type->size % type->alignment));
    if (type->trailing_array) {
        PAS_ASSERT(!type->trailing_array->trailing_array);
        deluge_validate_type(type->trailing_array);
    }
}

bool deluge_type_is_equal(const deluge_type* a, const deluge_type* b)
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
        return deluge_type_is_equal(a->trailing_array, b->trailing_array);
    } else if (b->trailing_array)
        return false;
    for (index = deluge_type_num_words(a); index--;) {
        if (a->word_types[index] != b->word_types[index])
            return false;
    }
    return true;
}

unsigned deluge_type_hash(const deluge_type* type)
{
    unsigned result;
    size_t index;
    result = type->size + 3 * type->alignment;
    if (type->trailing_array)
        result += 7 * deluge_type_hash(type->trailing_array);
    for (index = deluge_type_num_words(type); index--;) {
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
    case DELUGE_WORD_TYPE_FUNCTION:
        pas_stream_printf(stream, "Fu");
        return;
    case DELUGE_WORD_TYPE_TYPE:
        pas_stream_printf(stream, "Ty");
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
        pas_stream_printf(stream, "delty{invalid}");
        return;
    }
    
    pas_stream_printf(stream, "delty{%zu,%zu,", type->size, type->alignment);
    if (type->trailing_array) {
        deluge_type_dump(type->trailing_array, stream);
        pas_stream_printf(stream, ",");
    }
    for (index = 0; index < deluge_type_num_words(type); ++index)
        deluge_word_type_dump(type->word_types[index], stream);
    pas_stream_printf(stream, "}");
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

    deluge_validate_type(type);
    
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

pas_heap_ref* deluge_get_heap(const deluge_type* type)
{
    static const bool verbose = false;
    pas_heap_ref* result;
    if (verbose) {
        pas_log("Getting heap for ");
        deluge_type_dump(type, &pas_log_stream.base);
        pas_log("\n");
    }
    PAS_ASSERT(type != &deluge_int_type);
    deluge_lock_lock();
    result = get_heap_impl(type);
    deluge_lock_unlock();
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
    pas_allocation_result_identity,
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
    pas_allocation_result_identity,
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
    pas_allocation_result_identity);

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
    pas_allocation_result_identity);

void* deluge_try_allocate_many(pas_heap_ref* ref, size_t count)
{
    return (void*)deluge_try_allocate_many_impl_by_count(ref, count, 1).begin;
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

void deluded_zfree(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_deallocate(deluge_ptr_get_next_ptr(&args).ptr);
    DELUDED_DELETE_ARGS();
}

void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type)
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
    
    PAS_UNUSED_PARAM(ptr); /* It's totally OK for the ptr component to be anything, so long as
                              we don't access it. That means checks must treat the ptr component
                              as untrusted. */
    
    if (type == &deluge_function_type) {
        PAS_ASSERT(lower == ptr);
        PAS_ASSERT((char*)upper == (char*)ptr + 1);
        return;
    }

    if (!lower || !upper || !type) {
        /* It's possible for part of the ptr to fall off into a page that gets decommitted.
           In that case part of it will become null, and we'll treat the ptr as valid but
           inaccessible. For example, lower=0 always fails check_access_common. */
        return;
    }

    PAS_ASSERT(upper > lower);
    PAS_ASSERT(upper <= (void*)PAS_MAX_ADDRESS);
    PAS_ASSERT(pas_is_aligned((uintptr_t)lower, type->alignment));
    PAS_ASSERT(pas_is_aligned((uintptr_t)upper, type->alignment));
    PAS_ASSERT(!((upper - lower) % type->size));
    deluge_validate_type(type);
}

static void check_access_common(void* ptr, void* lower, void* upper, const deluge_type* type,
                                uintptr_t bytes)
{
    if (PAS_ENABLE_TESTING)
        deluge_validate_ptr_impl(ptr, lower, upper, type);

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
        "cannot access pointer with null lower (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        ptr >= lower,
        "cannot access pointer with ptr < lower (ptr = %p,%p,%p,%s)\n", 
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        ptr < upper,
        "cannot access pointer with ptr >= upper (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));

    DELUGE_CHECK(
        bytes <= (uintptr_t)((char*)upper - (char*)ptr),
        "cannot access %zu bytes when upper - ptr = %zu (ptr = %p,%p,%p,%s)\n",
        bytes, (size_t)((char*)upper - (char*)ptr),
        ptr, lower, upper, deluge_type_to_new_string(type));
        
    DELUGE_CHECK(
        type,
        "cannot access ptr with null type (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

static void check_int(void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes)
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
            "cannot access %zu bytes as int, span contains non-ints (ptr = %p,%p,%p,%s)\n",
            bytes, ptr, lower, upper, deluge_type_to_new_string(type));
    }
}

void deluge_check_access_int_impl(
    void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes)
{
    check_access_common(ptr, lower, upper, type, bytes);
    check_int(ptr, lower, upper, type, bytes);
}

void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type)
{
    uintptr_t offset;
    uintptr_t word_type_index;

    check_access_common(ptr, lower, upper, type, 32);

    offset = (char*)ptr - (char*)lower;
    DELUGE_CHECK(
        pas_is_aligned(offset, 8),
        "cannot access memory as ptr without 8-byte alignment; in this case ptr %% 8 = %zu (ptr = %p,%p,%p,%s)\n",
        (size_t)(offset % 8), ptr, lower, upper, deluge_type_to_new_string(type));
    word_type_index = offset / 8;
    if (type->trailing_array) {
        uintptr_t num_words = deluge_type_num_words_exact(type);
        if (word_type_index < num_words)
            PAS_ASSERT(word_type_index + 3 < num_words);
        else {
            word_type_index -= num_words;
            type = type->trailing_array;
        }
    }
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 0) == DELUGE_WORD_TYPE_PTR_PART1,
        "memory type error accessing ptr part1 (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 1) == DELUGE_WORD_TYPE_PTR_PART2,
        "memory type error accessing ptr part2 (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 2) == DELUGE_WORD_TYPE_PTR_PART3,
        "memory type error accessing ptr part3 (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
    DELUGE_CHECK(
        deluge_type_get_word_type(type, word_type_index + 3) == DELUGE_WORD_TYPE_PTR_PART4,
        "memory type error accessing ptr part4 (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

void deluge_check_access_function_call_impl(void* ptr, void* lower, void* upper, const deluge_type* type)
{
    check_access_common(ptr, lower, upper, type, 1);
    DELUGE_CHECK(
        type == &deluge_function_type,
        "attempt to call pointer that is not a function (ptr = %p,%p,%p,%s)\n",
        ptr, lower, upper, deluge_type_to_new_string(type));
}

void deluge_memset_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                        unsigned value, size_t count)
{
    if (!count)
        return;
    
    check_access_common(ptr, lower, upper, type, count);
    
    if (!value) {
        if (type != &deluge_int_type) {
            uintptr_t offset;
            deluge_word_type word_type;
            
            offset = (char*)ptr - (char*)lower;
            word_type = deluge_type_get_word_type(type, offset / 8);
            if (offset % 8)
                PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
            else
                PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART2 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
            word_type = deluge_type_get_word_type(type, (offset + count - 1) / 8);
            if ((offset + count) % 8)
                PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
            else
                PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART3);
        }
        
        memset(ptr, 0, count);
        return;
    }

    check_int(ptr, lower, upper, type, count);
    memset(ptr, value, count);
}

static void check_type_overlap(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                               void* src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                               size_t count)
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
        check_int(dst_ptr, dst_lower, dst_upper, dst_type, count);
        check_int(src_ptr, src_lower, src_upper, src_type, count);
        return;
    }

    num_word_types = (dst_offset + count - 1) / 8 - dst_offset / 8 + 1;
    first_dst_word_type_index = dst_offset / 8;
    first_src_word_type_index = src_offset / 8;

    for (word_type_index_offset = num_word_types; word_type_index_offset--;) {
        PAS_ASSERT(deluge_type_get_word_type(dst_type, first_dst_word_type_index + word_type_index_offset)
                   ==
                   deluge_type_get_word_type(src_type, first_src_word_type_index + word_type_index_offset));
    }

    /* We cannot copy parts of pointers. */
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index);
    if (dst_offset % 8)
        PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
    else
        PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART2 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
    word_type = deluge_type_get_word_type(dst_type, first_dst_word_type_index + num_word_types - 1);
    if ((dst_offset + count) % 8)
        PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART4);
    else
        PAS_ASSERT(word_type < DELUGE_WORD_TYPE_PTR_PART1 || word_type > DELUGE_WORD_TYPE_PTR_PART3);
}

static void check_copy(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                       void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                       size_t count)
{
    check_access_common(dst_ptr, dst_lower, dst_upper, dst_type, count);
    check_access_common(src_ptr, src_lower, src_upper, src_type, count);
    check_type_overlap(dst_ptr, dst_lower, dst_upper, dst_type,
                       src_ptr, src_lower, src_upper, src_type,
                       count);
}

void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                        size_t count)
{
    if (!count)
        return;
    
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count);
    
    memcpy(dst_ptr, src_ptr, count);
}

void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                         size_t count)
{
    if (!count)
        return;
    
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count);
    
    memmove(dst_ptr, src_ptr, count);
}

void deluge_check_restrict(void* ptr, void* lower, void* upper, const deluge_type* type,
                           void* new_upper, const deluge_type* new_type)
{
    check_access_common(ptr, lower, upper, type, (char*)new_upper - (char*)ptr);
    DELUGE_CHECK(new_type, "cannot restrict to NULL type\n");
    check_type_overlap(ptr, ptr, new_upper, new_type,
                       ptr, lower, upper, type,
                       (char*)new_upper - (char*)ptr);
}

const char* deluge_check_and_get_str(deluge_ptr str)
{
    size_t available;
    size_t length;
    check_access_common(str.ptr, str.lower, str.upper, str.type, 1);
    available = (char*)str.upper - (char*)str.ptr;
    length = strnlen((char*)str.ptr, available);
    PAS_ASSERT(length < available);
    PAS_ASSERT(length + 1 <= available);
    check_int(str.ptr, str.lower, str.upper, str.type, length + 1);
    return (char*)str.ptr;
}

void* deluge_va_arg_impl(
    void* va_list_ptr, void* va_list_lower, void* va_list_upper, const deluge_type* va_list_type,
    size_t count, size_t alignment, const deluge_type* type)
{
    deluge_ptr va_list;
    deluge_ptr* va_list_impl;
    void* result;
    va_list = deluge_ptr_forge(va_list_ptr, va_list_lower, va_list_upper, va_list_type);
    deluge_check_access_ptr(va_list);
    va_list_impl = (deluge_ptr*)va_list.ptr;
    return deluge_ptr_get_next(va_list_impl, count, alignment, type).ptr;
}

void deluge_error(void)
{
    PAS_ASSERT(!"deluge error");
}

static void print_str(const char* str)
{
    write(1, str, strlen(str));
}

void deluded_zprint(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr str;
    str = deluge_ptr_get_next_ptr(&args);
    DELUDED_DELETE_ARGS();
    print_str(deluge_check_and_get_str(str));
}

void deluded_zprint_long(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    long x;
    char buf[100];
    x = deluge_ptr_get_next_long(&args);
    DELUDED_DELETE_ARGS();
    snprintf(buf, sizeof(buf), "%ld", x);
    print_str(buf);
}

void deluded_zprint_ptr(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr ptr;
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    ptr = deluge_ptr_get_next_ptr(&args);
    DELUDED_DELETE_ARGS();
    initialize_utility_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    pas_string_stream_printf(&stream, "%p,%p,%p,", ptr.ptr, ptr.lower, ptr.upper);
    deluge_type_dump(ptr.type, &stream.base);
    print_str(pas_string_stream_get_string(&stream));
    pas_string_stream_destruct(&stream);
}

void deluded_zerror(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    const char* str = deluge_check_and_get_str(deluge_ptr_get_next_ptr(&args));
    DELUDED_DELETE_ARGS();
    pas_panic("zerror() called with: %s\n", str);
}

void deluded_zstrlen(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    const char* str = deluge_check_and_get_str(deluge_ptr_get_next_ptr(&args));
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int));
    *(int*)rets.ptr = strlen(str);
}

void deluded_zstrchr(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr str_ptr = deluge_ptr_get_next_ptr(&args);
    const char* str = deluge_check_and_get_str(str_ptr);
    int chr = deluge_ptr_get_next_int(&args);
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets);
    *(deluge_ptr*)rets.ptr =
        deluge_ptr_forge(strchr(str, chr), str_ptr.lower, str_ptr.upper, str_ptr.type);
}

void deluded_zmemchr(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    deluge_ptr str_ptr = deluge_ptr_get_next_ptr(&args);
    int chr = deluge_ptr_get_next_int(&args);
    size_t length = deluge_ptr_get_next_size_t(&args);
    void* result;
    DELUDED_DELETE_ARGS();
    deluge_check_access_ptr(rets);
    if (!length)
        result = NULL;
    else {
        deluge_check_access_int(str_ptr, length);
        result = memchr(str_ptr.ptr, chr, length);
    }
    *(deluge_ptr*)rets.ptr =
        deluge_ptr_forge(result, str_ptr.lower, str_ptr.upper, str_ptr.type);
}

void deluded_zisdigit(DELUDED_SIGNATURE)
{
    deluge_ptr args = DELUDED_ARGS;
    deluge_ptr rets = DELUDED_RETS;
    int chr = deluge_ptr_get_next_int(&args);
    DELUDED_DELETE_ARGS();
    deluge_check_access_int(rets, sizeof(int));
    *(int*)rets.ptr = isdigit(chr);
}

#endif /* PAS_ENABLE_DELUGE */

#endif /* LIBPAS_ENABLED */

