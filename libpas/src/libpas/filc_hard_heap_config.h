#ifndef FILC_HARD_HEAP_CONFIG_H
#define FILC_HARD_HEAP_CONFIG_H

#include "pas_config.h"

#if PAS_ENABLE_FILC

#include "pas_bitfit_max_free.h"
#include "pas_heap_config_utils.h"
#include "pas_ptr_hash_map.h"

PAS_BEGIN_EXTERN_C;

struct filc_hard_heap_config_enumerator_data;
struct filc_hard_heap_config_root_data;
struct pas_page_header_table;
typedef struct filc_hard_heap_config_enumerator_data filc_hard_heap_config_enumerator_data;
typedef struct filc_hard_heap_config_root_data filc_hard_heap_config_root_data;
typedef struct pas_page_header_table pas_page_header_table;

struct filc_hard_heap_config_root_data {
    pas_page_header_table* small_page_header_table;
    pas_segregated_shared_page_directory* shared_directory;
};

struct filc_hard_heap_config_enumerator_data {
    pas_ptr_hash_map page_header_table;
    pas_segregated_shared_page_directory* shared_directory;
};

PAS_API void filc_hard_heap_config_activate(void);

PAS_API pas_page_base* filc_hard_page_header_for_boundary_remote(pas_enumerator* enumerator,
                                                                   void* boundary);

static PAS_ALWAYS_INLINE pas_page_base* filc_hard_page_header_for_boundary(void* boundary);
static PAS_ALWAYS_INLINE void* filc_hard_boundary_for_page_header(pas_page_base* page);
PAS_API void* filc_hard_segregated_allocate_page(
    pas_segregated_heap* heap, pas_physical_memory_transaction* transaction,
    pas_segregated_page_role role);
PAS_API void* filc_hard_bitfit_allocate_page(
    pas_segregated_heap* heap, pas_physical_memory_transaction* transaction);
PAS_API pas_page_base* filc_hard_segregated_create_page_header(
    void* boundary, pas_page_kind kind, pas_lock_hold_mode heap_lock_hold_mode);
PAS_API pas_page_base* filc_hard_bitfit_create_page_header(
    void* boundary, pas_page_kind kind, pas_lock_hold_mode heap_lock_hold_mode);
PAS_API void filc_hard_destroy_page_header(
    pas_page_base* page, pas_lock_hold_mode heap_lock_hold_mode);
PAS_API pas_segregated_shared_page_directory* filc_hard_shared_page_directory_selector(
    pas_segregated_heap* heap, pas_segregated_size_directory* directory);

PAS_SEGREGATED_PAGE_CONFIG_SPECIALIZATION_DECLARATIONS(filc_hard_segregated_config);
PAS_BITFIT_PAGE_CONFIG_SPECIALIZATION_DECLARATIONS(filc_hard_bitfit_config);

static PAS_ALWAYS_INLINE pas_fast_megapage_kind filc_hard_fast_megapage_kind(uintptr_t begin)
{
    PAS_UNUSED_PARAM(begin);
    return pas_not_a_fast_megapage_kind;
}

static PAS_ALWAYS_INLINE pas_page_base* filc_hard_config_page_header(uintptr_t begin);
PAS_API pas_aligned_allocation_result filc_hard_aligned_allocator(
    size_t size, pas_alignment alignment, pas_large_heap* heap, const pas_heap_config* config);
PAS_API void* filc_hard_prepare_to_enumerate(pas_enumerator* enumerator);
PAS_API bool filc_hard_for_each_shared_page_directory(
    pas_segregated_heap* heap,
    bool (*callback)(pas_segregated_shared_page_directory* directory,
                     void* arg),
    void* arg);
PAS_API bool filc_hard_for_each_shared_page_directory_remote(
    pas_enumerator* enumerator,
    pas_segregated_heap* heap,
    bool (*callback)(pas_enumerator* enumerator,
                     pas_segregated_shared_page_directory* directory,
                     void* arg),
    void* arg);
PAS_API void filc_hard_dump_shared_page_directory_arg(
    pas_stream* stream, pas_segregated_shared_page_directory* directory);

PAS_HEAP_CONFIG_SPECIALIZATION_DECLARATIONS(filc_hard_heap_config);

#define FILC_HARD_HEAP_CONFIG ((pas_heap_config){ \
        .config_ptr = &filc_hard_heap_config, \
        .kind = pas_heap_config_kind_filc_hard, \
        .activate_callback = filc_hard_heap_config_activate, \
        .get_type_size = filc_type_as_heap_type_get_type_size, \
        .get_type_alignment = filc_type_as_heap_type_get_type_alignment, \
        .dump_type = filc_type_as_heap_type_dump, \
        .get_type_runtime_config = filc_type_as_heap_type_assert_default_runtime_config, \
        .large_alignment = PAS_MIN_ALIGN, \
        .small_segregated_config = { \
            .base = { \
                .is_enabled = true, \
                .heap_config_ptr = &filc_hard_heap_config, \
                .page_config_ptr = &filc_hard_heap_config.small_segregated_config.base, \
                .page_config_kind = pas_page_config_kind_segregated, \
                .min_align_shift = PAS_MIN_ALIGN_SHIFT, \
                .page_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
                .granule_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
                .non_committable_granule_bitvector = NULL, \
                .max_object_size = PAS_MAX_OBJECT_SIZE(PAS_SMALL_PAGE_DEFAULT_SIZE), \
                .page_header_for_boundary = filc_hard_page_header_for_boundary, \
                .boundary_for_page_header = filc_hard_boundary_for_page_header, \
                .page_header_for_boundary_remote = filc_hard_page_header_for_boundary_remote, \
                .create_page_header = filc_hard_segregated_create_page_header, \
                .destroy_page_header = filc_hard_destroy_page_header, \
            }, \
            .variant = pas_small_segregated_page_config_variant, \
            .kind = pas_segregated_page_config_kind_filc_hard_segregated, \
            .wasteage_handicap = 1., \
            .sharing_shift = PAS_SMALL_SHARING_SHIFT, \
            .num_alloc_bits = PAS_BASIC_SEGREGATED_NUM_ALLOC_BITS( \
                PAS_MIN_ALIGN_SHIFT, PAS_SMALL_PAGE_DEFAULT_SIZE), \
            .shared_payload_offset = 0, \
            .exclusive_payload_offset = 0, \
            .shared_payload_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
            .exclusive_payload_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
            .shared_logging_mode = pas_segregated_deallocation_no_logging_mode, \
            .exclusive_logging_mode = pas_segregated_deallocation_no_logging_mode, \
            .use_reversed_current_word = PAS_ARM64, \
            .check_deallocation = true, \
            .enable_empty_word_eligibility_optimization_for_shared = false, \
            .enable_empty_word_eligibility_optimization_for_exclusive = false, \
            .enable_view_cache = false, \
            .page_allocator = filc_hard_segregated_allocate_page, \
            .shared_page_directory_selector = filc_hard_shared_page_directory_selector, \
            PAS_SEGREGATED_PAGE_CONFIG_SPECIALIZATIONS(filc_hard_segregated_config) \
        }, \
        .medium_segregated_config = { \
            .base = { \
                .is_enabled = false \
            } \
        }, \
        .small_bitfit_config = { \
            .base = { \
                .is_enabled = true, \
                .heap_config_ptr = &filc_hard_heap_config, \
                .page_config_ptr = &filc_hard_heap_config.small_bitfit_config.base, \
                .page_config_kind = pas_page_config_kind_bitfit, \
                .min_align_shift = PAS_MIN_ALIGN_SHIFT, \
                .page_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
                .granule_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
                .non_committable_granule_bitvector = NULL, \
                .max_object_size = \
                    PAS_BITFIT_MAX_FREE_MAX_VALID << PAS_MIN_ALIGN_SHIFT, \
                .page_header_for_boundary = filc_hard_page_header_for_boundary, \
                .boundary_for_page_header = filc_hard_boundary_for_page_header, \
                .page_header_for_boundary_remote = filc_hard_page_header_for_boundary_remote, \
                .create_page_header = filc_hard_bitfit_create_page_header, \
                .destroy_page_header = filc_hard_destroy_page_header, \
            }, \
            .variant = pas_small_bitfit_page_config_variant, \
            .kind = pas_bitfit_page_config_kind_filc_hard_bitfit, \
            .page_object_payload_offset = 0, \
            .page_object_payload_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
            .page_allocator = filc_hard_bitfit_allocate_page, \
            PAS_BITFIT_PAGE_CONFIG_SPECIALIZATIONS(filc_hard_bitfit_config), \
        }, \
        .medium_bitfit_config = { \
            .base = { \
                .is_enabled = false \
            } \
        }, \
        .marge_bitfit_config = { \
            .base = { \
                .is_enabled = false \
            } \
        }, \
        .small_lookup_size_upper_bound = PAS_SMALL_LOOKUP_SIZE_UPPER_BOUND, \
        .fast_megapage_kind_func = filc_hard_fast_megapage_kind, \
        .small_segregated_is_in_megapage = false, \
        .small_bitfit_is_in_megapage = false, \
        .page_header_func = filc_hard_config_page_header, \
        .aligned_allocator = filc_hard_aligned_allocator, \
        .aligned_allocator_talks_to_sharing_pool = true, \
        .deallocator = NULL, \
        .mmap_capability = pas_mmap_hard, \
        .root_data = &filc_hard_root_data, \
        .prepare_to_enumerate = filc_hard_prepare_to_enumerate, \
        .for_each_shared_page_directory = filc_hard_for_each_shared_page_directory, \
        .for_each_shared_page_directory_remote = filc_hard_for_each_shared_page_directory_remote, \
        .dump_shared_page_directory_arg = filc_hard_dump_shared_page_directory_arg, \
        PAS_HEAP_CONFIG_SPECIALIZATIONS(filc_hard_heap_config) \
    })

PAS_API extern const pas_heap_config filc_hard_heap_config;

PAS_API extern pas_page_header_table filc_hard_page_header_table;
PAS_API extern pas_heap_runtime_config filc_hard_int_runtime_config;
PAS_API extern pas_heap_runtime_config filc_hard_typed_runtime_config;
PAS_API extern pas_heap_runtime_config filc_hard_flex_runtime_config;
PAS_API extern pas_segregated_shared_page_directory filc_hard_shared_page_directory;
PAS_API extern filc_hard_heap_config_root_data filc_hard_root_data;

static PAS_ALWAYS_INLINE pas_page_base* filc_hard_page_header_for_boundary(void* boundary)
{
    return pas_page_header_table_get_for_boundary(
        &filc_hard_page_header_table, PAS_SMALL_PAGE_DEFAULT_SIZE, boundary);
}

static PAS_ALWAYS_INLINE void* filc_hard_boundary_for_page_header(pas_page_base* page)
{
    return pas_page_header_table_get_boundary(
        &filc_hard_page_header_table, PAS_SMALL_PAGE_DEFAULT_SIZE, page);
}

static PAS_ALWAYS_INLINE pas_page_base* filc_hard_config_page_header(uintptr_t begin)
{
    return pas_page_header_table_get_for_address(
        &filc_hard_page_header_table, PAS_SMALL_PAGE_DEFAULT_SIZE, (void*)begin);
}

PAS_END_EXTERN_C;

#endif /* PAS_ENABLE_FILC */

#endif /* FILC_HARD_HEAP_CONFIG_H */

