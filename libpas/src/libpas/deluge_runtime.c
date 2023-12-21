#include "deluge_heap_config.h"
#include "deluge_heap_innards.h"
#include "deluge_runtime.h"
#include "deluge_type_table.h"
#include "pas_deallocate.h"
#include "pas_try_allocate.h"
#include "pas_try_allocate_array.h"
#include "pas_try_allocate_intrinsic.h"
#include "pas_utils.h"

const deluge_type deluge_int_type = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { DELUGE_WORD_TYPE_INT }
};

const pas_allocation_config deluge_utility_allocation_config = {
    .allocate = deluge_allocate_utility_for_allocation_config,
    .deallocate = deluge_deallocate_for_allocation_config,
    .arg = NULL
};

deluge_type_table deluge_type_table_instance = PAS_HASHTABLE_INSTANCE;

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
        pas_stream_printf(stream, "Of");
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
    size_t index;
    pas_stream_printf(stream, "delty{%zu,%zu,", type->size, type->alignment);
    if (type->trailing_array) {
        deluge_type_dump(type->trailing_array, stream);
        pas_stream_printf(stream, ",");
    }
    for (index = 0; index < deluge_type_num_words(type); ++index)
        deluge_word_type_dump(type->word_types[index], stream);
    pas_stream_printf(stream, "}");
}

void deluge_type_as_heap_type_dmp(const pas_heap_type* type, pas_stream* stream)
{
    deluge_type_dump((const deluge_type*)type, stream);
}

static pas_heap_ref* get_heap_impl(const deluge_type* type)
{
    pas_allocation_config allocation_config;
    deluge_type_table_add_result add_result;
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = deluge_type_table_add(&deluge_type_table_instance, type, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        add_result.entry.type = type;
        add_result.entry.heap = (pas_heap_ref*)deluge_allocate_utility(sizeof(pas_heap_ref));
        add_result.entry.heap->type = type;
        add_result.entry.heap->heap = NULL;
        add_result.enrty.heap->allocator_index = 0;
    }

    return add_result.entry.heap;
}

pas_heap_ref* deluge_get_heap(const deluge_type* type)
{
    pas_heap_ref* result;
    deluge_lock_lock();
    result = get_heap_impl(type);
    deluge_lock_unlock();
    return result;
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    deluge_try_allocate_int_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_intrinsic_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_identity,
    &deluge_int_heap,
    &deluge_int_heap_support,
    pas_intrinsic_heap_is_designated);

pas_intrinsic_heap_support deluge_int_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap deluge_int_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &deluge_int_heap,
        &deluge_int_type,
        deluge_int_heap_support,
        DELUGE_HEAP_CONFIG,
        &deluge_intrinsic_runtime_config.base);

pas_allocator_counts deluge_allocator_counts;

void* deluge_try_allocate_int(size_t size)
{
    return deluge_try_allocate_int_impl_ptr(size, 1);
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

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    deluge_try_allocate_with_size_impl,
    DELUGE_HEAP_CONFIG,
    &deluge_typed_runtime_config.base,
    &deluge_allocator_counts,
    pas_allocation_result_identity);

void* deluge_try_allocate_with_size(pas_heap_ref* ref, size_t size)
{
    return (void*)deluge_try_allocate_with_size_impl_by_size(ref, size, 1).begin;
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

pas_intrinsic_heap_support deluge_utility_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap deluge_utility_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &deluge_utility_heap,
        &deluge_int_type,
        deluge_utility_heap_support,
        DELUGE_HEAP_CONFIG,
        &deluge_intrinsic_runtime_config.base);

void* deluge_allocate_utility(size_t size)
{
    return deluge_allocate_utility_impl_ptr(size, 1);
}

void deluge_deallocate(void* ptr)
{
    pas_deallocate(ptr, DELUGE_HEAP_CONFIG);
}

void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type)
{
    if (!type) {
        PAS_ASSERT(!lower);
        PAS_ASSERT(!upper);
        return;
    }

    PAS_ASSERT(upper > lower);
    PAS_ASSERT(upper <= (void*)PAS_MAX_ADDRESS);
    PAS_ASSERT(pas_is_aligned((uintptr_t)lower, type->alignment));
    PAS_ASSERT(pas_is_aligned((uintptr_t)upper, type->alignment));
    PAS_ASSERT(!((upper - lower) % type->size));
}

static void check_access_common(void* ptr, void* lower, void* upper, uintptr_t bytes)
{
    PAS_ASSERT(ptr >= lower);
    PAS_ASSERT(ptr < upper);
    PAS_ASSERT((char*)ptr + bytes <= upper);
}

static void check_int(void* ptr, void* lower, const deluge_type* type, uintptr_t bytes)
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
         word_type_index++)
        PAS_ASSERT(type->word_types[word_type_index % deluge_type_num_words(type)] == DELUGE_WORD_TYPE_INT);
}

void deluge_check_access_int_impl(
    void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes)
{
    check_access_common(ptr, lower, upper, bytes);
    check_int(ptr, lower, type, bytes);
}

void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type)
{
    uintptr_t offset;
    uintptr_t word_type_index;

    check_access_common(ptr, lower, upper, bytes);

    offset = (char*)ptr - (char*)lower;
    PAS_ASSERT(pas_is_aligned(offset, 8));
    word_type_index = offset / 8;
    PAS_ASSERT(type->word_types[word_type_index + 0] == DELUGE_WORD_TYPE_PTR_PART1);
    PAS_ASSERT(type->word_types[word_type_index + 1] == DELUGE_WORD_TYPE_PTR_PART2);
    PAS_ASSERT(type->word_types[word_type_index + 2] == DELUGE_WORD_TYPE_PTR_PART3);
    PAS_ASSERT(type->word_types[word_type_index + 3] == DELUGE_WORD_TYPE_PTR_PART4);
}

void deluge_memset_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                        unsigned value, size_t count)
{
    check_access_common(ptr, lower, upper, count);
    
    if (!value) {
        memset(ptr, 0, count);
        return;
    }

    check_int(ptr, lower, type, count);
    memset(ptr, value, count);
}

static void check_copy(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                       void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                       size_t count)
{
    check_access_common(dst_ptr, dst_lower, dst_upper, count);
    check_access_common(src_ptr, src_lower, src_upper, count);
    
    if (dst_type == &deluge_int_type && src_type == &deluge_int_type)
        return;

    /* FIXME: Lots of shit to check for. */
}

void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                        size_t count)
{
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count);
    
    memcpy(dst_ptr, src_ptr, count);
}

void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                         size_t count)
{
    check_copy(dst_ptr, dst_lower, dst_upper, dst_type,
               src_ptr, src_lower, src_upper, src_type,
               count);
    
    memmove(dst_ptr, src_ptr, count);
}
