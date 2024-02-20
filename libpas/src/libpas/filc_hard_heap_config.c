#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_hard_heap_config.h"

#if PAS_ENABLE_FILC

#include "filc_heap_config.h"
#include "pas_heap_config_utils_inlines.h"
#include "pas_large_sharing_pool.h"
#include "pas_reservation.h"
#include "pas_reservation_free_heap.h"
#include "pas_root.h"

PAS_BEGIN_EXTERN_C;

const pas_heap_config filc_hard_heap_config = FILC_HARD_HEAP_CONFIG;

pas_page_header_table filc_hard_page_header_table =
    PAS_PAGE_HEADER_TABLE_INITIALIZER(PAS_SMALL_PAGE_DEFAULT_SIZE);

pas_heap_runtime_config filc_hard_int_runtime_config = {
    .sharing_mode = pas_share_pages,
    .statically_allocated = true,
    .is_part_of_heap = true,
    .is_flex = false,
    .directory_size_bound_for_partial_views = 0,
    .directory_size_bound_for_baseline_allocators = 0,
    .directory_size_bound_for_no_view_cache = 0,
    .max_segregated_object_size = 0,
    .max_bitfit_object_size = UINT_MAX,
    .view_cache_capacity_for_object_size = pas_heap_runtime_config_zero_view_cache_capacity,
    .initialize_fresh_memory = NULL
};

pas_heap_runtime_config filc_hard_typed_runtime_config = {
    .sharing_mode = pas_share_pages,
    .statically_allocated = false,
    .is_part_of_heap = true,
    .is_flex = false,
    .directory_size_bound_for_partial_views = PAS_TYPED_BOUND_FOR_PARTIAL_VIEWS,
    .directory_size_bound_for_baseline_allocators = 0,
    .directory_size_bound_for_no_view_cache = 0,
    .max_segregated_object_size = UINT_MAX,
    .max_bitfit_object_size = 0,
    .view_cache_capacity_for_object_size = pas_heap_runtime_config_zero_view_cache_capacity,
    .initialize_fresh_memory = NULL
};

pas_heap_runtime_config filc_hard_flex_runtime_config = {
    .sharing_mode = pas_share_pages,
    .statically_allocated = false,
    .is_part_of_heap = true,
    .is_flex = true,
    .directory_size_bound_for_partial_views = PAS_FLEX_BOUND_FOR_PARTIAL_VIEWS,
    .directory_size_bound_for_baseline_allocators = 0,
    .directory_size_bound_for_no_view_cache = 0,
    .max_segregated_object_size = UINT_MAX,
    .max_bitfit_object_size = 0,
    .view_cache_capacity_for_object_size = pas_heap_runtime_config_zero_view_cache_capacity,
    .initialize_fresh_memory = NULL
};

pas_segregated_shared_page_directory filc_hard_shared_page_directory =
    PAS_SEGREGATED_SHARED_PAGE_DIRECTORY_INITIALIZER(
        FILC_HARD_HEAP_CONFIG.small_segregated_config, pas_share_pages, NULL);

filc_hard_heap_config_root_data filc_hard_root_data = {
    .small_page_header_table = &filc_hard_page_header_table,
    .shared_directory = &filc_hard_shared_page_directory
};

void filc_hard_heap_config_activate(void)
{
    pas_heap_config_activate(&filc_heap_config);
}

pas_page_base* filc_hard_page_header_for_boundary_remote(pas_enumerator* enumerator,
                                                           void* boundary)
{
    filc_hard_heap_config_enumerator_data* data;
    data = (filc_hard_heap_config_enumerator_data*)
        enumerator->heap_config_datas[pas_heap_config_kind_filc_hard];
    PAS_ASSERT(data);

    return (pas_page_base*)pas_ptr_hash_map_get(&data->page_header_table, boundary).value;
}

static void* allocate_page(pas_segregated_heap* heap, pas_physical_memory_transaction* transaction)
{
    pas_allocation_result result;
    bool lock_result;

    PAS_UNUSED_PARAM(heap);
    PAS_UNUSED_PARAM(transaction);

    result = pas_reservation_free_heap_try_allocate_with_alignment(
        PAS_SMALL_PAGE_DEFAULT_SIZE * 3, pas_alignment_create_traditional(PAS_SMALL_PAGE_DEFAULT_SIZE),
        "filc_hard_page", pas_delegate_allocation);
    if (!result.did_succeed)
        return NULL;

    pas_page_malloc_protect_reservation((void*)result.begin, PAS_SMALL_PAGE_DEFAULT_SIZE);
    pas_page_malloc_protect_reservation((void*)(result.begin + PAS_SMALL_PAGE_DEFAULT_SIZE * 2),
                                        PAS_SMALL_PAGE_DEFAULT_SIZE);

    pas_reservation_commit((void*)(result.begin + PAS_SMALL_PAGE_DEFAULT_SIZE),
                           PAS_SMALL_PAGE_DEFAULT_SIZE);

    lock_result = pas_page_malloc_lock((void*)(result.begin + PAS_SMALL_PAGE_DEFAULT_SIZE),
                                       PAS_SMALL_PAGE_DEFAULT_SIZE);
    PAS_ASSERT(lock_result); /* FIXME: We could handle this, but it would require returning memory to the
                                sharing cache, which would require implementing another function, and I
                                don't feel like it right now. */
    
    return (void*)(result.begin + PAS_SMALL_PAGE_DEFAULT_SIZE);
}

void* filc_hard_segregated_allocate_page(
    pas_segregated_heap* heap, pas_physical_memory_transaction* transaction, pas_segregated_page_role role)
{
    PAS_UNUSED_PARAM(role);
    return allocate_page(heap, transaction);
}

void* filc_hard_bitfit_allocate_page(
    pas_segregated_heap* heap, pas_physical_memory_transaction* transaction)
{
    return allocate_page(heap, transaction);
}

pas_page_base* filc_hard_segregated_create_page_header(
    void* boundary, pas_page_kind kind, pas_lock_hold_mode heap_lock_hold_mode)
{
    pas_page_base* result;
    PAS_ASSERT(kind == pas_small_shared_segregated_page_kind
               || kind == pas_small_exclusive_segregated_page_kind);
    pas_heap_lock_lock_conditionally(heap_lock_hold_mode);
    result = pas_page_header_table_add(
        &filc_hard_page_header_table,
        PAS_SMALL_PAGE_DEFAULT_SIZE,
        pas_segregated_page_header_size(
            FILC_HARD_HEAP_CONFIG.small_segregated_config,
            pas_page_kind_get_segregated_role(kind)),
        boundary);
    pas_heap_lock_unlock_conditionally(heap_lock_hold_mode);
    return result;
}

pas_page_base* filc_hard_bitfit_create_page_header(
    void* boundary, pas_page_kind kind, pas_lock_hold_mode heap_lock_hold_mode)
{
    pas_page_base* result;
    PAS_ASSERT(kind == pas_small_bitfit_page_kind);
    pas_heap_lock_lock_conditionally(heap_lock_hold_mode);
    result = pas_page_header_table_add(
        &filc_hard_page_header_table,
        PAS_SMALL_PAGE_DEFAULT_SIZE,
        pas_bitfit_page_header_size(FILC_HARD_HEAP_CONFIG.small_bitfit_config),
        boundary);
    pas_heap_lock_unlock_conditionally(heap_lock_hold_mode);
    return result;
}

void filc_hard_destroy_page_header(pas_page_base* page, pas_lock_hold_mode heap_lock_hold_mode)
{
    pas_heap_lock_lock_conditionally(heap_lock_hold_mode);
    pas_page_header_table_remove(&filc_hard_page_header_table,
                                 PAS_SMALL_PAGE_DEFAULT_SIZE,
                                 page);
    pas_heap_lock_unlock_conditionally(heap_lock_hold_mode);
}

pas_segregated_shared_page_directory* filc_hard_shared_page_directory_selector(
    pas_segregated_heap* heap, pas_segregated_size_directory* directory)
{
    PAS_UNUSED_PARAM(heap);
    PAS_UNUSED_PARAM(directory);
    return &filc_hard_shared_page_directory;
}

pas_aligned_allocation_result filc_hard_aligned_allocator(
    size_t size, pas_alignment alignment, pas_large_heap* heap, const pas_heap_config* config)
{
    size_t actual_alignment; /* Doubles as the padding size. */
    size_t actual_payload_size;
    size_t actual_size;
    pas_allocation_result allocation_result;
    pas_aligned_allocation_result result;
    bool lock_result;

    PAS_UNUSED_PARAM(heap);
    PAS_ASSERT(config == &filc_hard_heap_config);
    
    pas_zero_memory(&result, sizeof(result));
    
    PAS_ASSERT(!alignment.alignment_begin);

    actual_alignment = pas_max_uintptr(alignment.alignment, PAS_SMALL_PAGE_DEFAULT_SIZE);
    actual_payload_size = pas_round_up_to_power_of_2(size, actual_alignment);
    actual_size = actual_payload_size + actual_alignment * 2;

    allocation_result = pas_reservation_free_heap_try_allocate_with_alignment(
        actual_size, pas_alignment_create_traditional(actual_alignment),
        "filc_hard_large", pas_delegate_allocation);

    if (!allocation_result.did_succeed)
        return result;

    pas_page_malloc_protect_reservation((void*)allocation_result.begin, actual_alignment);
    pas_page_malloc_protect_reservation((void*)(allocation_result.begin
                                                + actual_alignment + actual_payload_size),
                                        actual_alignment);

    pas_reservation_commit((void*)(allocation_result.begin + actual_alignment), actual_payload_size);
    lock_result = pas_page_malloc_lock((void*)(allocation_result.begin + PAS_SMALL_PAGE_DEFAULT_SIZE),
                                       actual_payload_size);
    PAS_ASSERT(lock_result); /* FIXME: We could handle this, but it would require returning memory to the
                                sharing cache, which would require implementing another function, and I
                                don't feel like it right now. */

    pas_large_sharing_pool_boot_free(
        pas_range_create(allocation_result.begin + actual_alignment,
                         allocation_result.begin + actual_alignment + actual_payload_size),
        pas_physical_memory_is_locked_by_virtual_range_common_lock,
        pas_mmap_hard);

    result.left_padding = (void*)(allocation_result.begin + actual_alignment);
    result.left_padding_size = 0;
    result.result = (void*)(allocation_result.begin + actual_alignment);
    result.result_size = size;
    result.right_padding = (void*)(allocation_result.begin + actual_alignment + size);
    result.right_padding_size = actual_payload_size - size;
    result.zero_mode = allocation_result.zero_mode;
    
    return result;
}

void* filc_hard_prepare_to_enumerate(pas_enumerator* enumerator)
{
    filc_hard_heap_config_enumerator_data* result;
    const pas_heap_config** configs;
    const pas_heap_config* config;
    filc_hard_heap_config_root_data* root_data;

    configs = (const pas_heap_config**)pas_enumerator_read(
        enumerator, enumerator->root->heap_configs,
        sizeof(const pas_heap_config*) * pas_heap_config_kind_num_kinds);
    if (!configs)
        return NULL;
    
    config = (const pas_heap_config*)pas_enumerator_read(
        enumerator, (void*)(uintptr_t)configs[pas_heap_config_kind_filc_hard], sizeof(pas_heap_config));
    if (!config)
        return NULL;

    root_data = (filc_hard_heap_config_root_data*)pas_enumerator_read(
        enumerator, config->root_data, sizeof(filc_hard_heap_config_root_data));
    if (!root_data)
        return NULL;

    result = (filc_hard_heap_config_enumerator_data*)pas_enumerator_allocate(
        enumerator, sizeof(filc_hard_heap_config_enumerator_data));
    
    pas_ptr_hash_map_construct(&result->page_header_table);

    if (!pas_add_remote_page_header_table_for_enumeration(
            &result->page_header_table,
            enumerator,
            (pas_page_header_table*)pas_enumerator_read(
                enumerator, root_data->small_page_header_table, sizeof(pas_page_header_table))))
        return NULL;

    result->shared_directory = pas_enumerator_read(
        enumerator, root_data->shared_directory, sizeof(pas_segregated_shared_page_directory));
    if (!result->shared_directory)
        return NULL;
    
    return result;
}

bool filc_hard_for_each_shared_page_directory(
    pas_segregated_heap* heap,
    bool (*callback)(pas_segregated_shared_page_directory* directory,
                     void* arg),
    void* arg)
{
    PAS_UNUSED_PARAM(heap);
    return callback(&filc_hard_shared_page_directory, arg);
}

bool filc_hard_for_each_shared_page_directory_remote(
    pas_enumerator* enumerator,
    pas_segregated_heap* heap,
    bool (*callback)(pas_enumerator* enumerator,
                     pas_segregated_shared_page_directory* directory,
                     void* arg),
    void* arg)
{
    filc_hard_heap_config_enumerator_data* data;

    PAS_UNUSED_PARAM(heap);

    data = (filc_hard_heap_config_enumerator_data*)
        enumerator->heap_config_datas[pas_heap_config_kind_filc_hard];
    PAS_ASSERT(data);
    
    return callback(enumerator, data->shared_directory, arg);
}

void filc_hard_dump_shared_page_directory_arg(
    pas_stream* stream, pas_segregated_shared_page_directory* directory)
{
    PAS_ASSERT(!directory->dump_arg);
    PAS_ASSERT(directory == &filc_hard_shared_page_directory);
    pas_stream_printf(stream, "default");
}

PAS_SEGREGATED_PAGE_CONFIG_SPECIALIZATION_DEFINITIONS(
    filc_hard_segregated_config, FILC_HARD_HEAP_CONFIG.small_segregated_config);
PAS_BITFIT_PAGE_CONFIG_SPECIALIZATION_DEFINITIONS(
    filc_hard_bitfit_config, FILC_HARD_HEAP_CONFIG.small_bitfit_config);
PAS_HEAP_CONFIG_SPECIALIZATION_DEFINITIONS(
    filc_hard_heap_config, FILC_HARD_HEAP_CONFIG);

PAS_END_EXTERN_C;

#endif /* PAS_ENABLE_FILC */

#endif /* LIBPAS_ENABLED */



