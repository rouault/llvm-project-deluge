#ifndef DELUGE_HEAP_CONFIG_H
#define DELUGE_HEAP_CONFIG_H

#include "pas_config.h"

#if PAS_ENABLE_DELUGE

#include "deluge_runtime.h"
#include "pas_heap_config_utils.h"

PAS_API void deluge_heap_config_activate(void);

#define DELUGE_HEAP_CONFIG PAS_BASIC_HEAP_CONFIG( \
    deluge, \
    .activate = deluge_heap_config_activate, \
    .get_type_size = deluge_type_as_heap_type_get_type_size, \
    .get_type_alignment = deluge_type_as_heap_type_get_type_alignment, \
    .dump_type = deluge_type_as_heap_type_dump, \
    .check_deallocation = true, \
    .small_segregated_min_align_shift = PAS_MIN_ALIGN_SHIFT, \
    .small_segregated_sharing_shift = PAS_SMALL_SHARING_SHIFT, \
    .small_segregated_page_size = PAS_SMALL_PAGE_DEFAULT_SIZE, \
    .small_segregated_wasteage_handicap = PAS_SMALL_PAGE_HANDICAP, \
    .small_exclusive_segregated_logging_mode = pas_segregated_deallocation_size_oblivious_logging_mode, \
    .small_shared_segregated_logging_mode = pas_segregated_deallocation_no_logging_mode, \
    .small_exclusive_segregated_enable_empty_word_eligibility_optimization = true, \
    .small_shared_segregated_enable_empty_word_eligibility_optimization = false, \
    .small_segregated_use_reversed_current_word = PAS_ARM64, \
    .enable_view_cache = true, \
    .use_small_bitfit = true, \
    .small_bitfit_min_align_shift = PAS_MIN_ALIGN_SHIFT, \
    .small_bitfit_page_size = PAS_SMALL_BITFIT_PAGE_DEFAULT_SIZE, \
    .medium_page_size = PAS_MEDIUM_PAGE_DEFAULT_SIZE, \
    .granule_size = PAS_GRANULE_DEFAULT_SIZE, \
    .use_medium_segregated = true, \
    .medium_segregated_min_align_shift = PAS_MIN_MEDIUM_ALIGN_SHIFT, \
    .medium_segregated_sharing_shift = PAS_MEDIUM_SHARING_SHIFT, \
    .medium_segregated_wasteage_handicap = PAS_MEDIUM_PAGE_HANDICAP, \
    .medium_exclusive_segregated_logging_mode = pas_segregated_deallocation_size_aware_logging_mode, \
    .medium_shared_segregated_logging_mode = pas_segregated_deallocation_no_logging_mode, \
    .use_medium_bitfit = true, \
    .medium_bitfit_min_align_shift = PAS_MIN_MEDIUM_ALIGN_SHIFT, \
    .use_marge_bitfit = true, \
    .marge_bitfit_min_align_shift = PAS_MIN_MARGE_ALIGN_SHIFT, \
    .marge_bitfit_page_size = PAS_MARGE_PAGE_DEFAULT_SIZE)

PAS_API extern const pas_heap_config deluge_heap_config;

PAS_BASIC_HEAP_CONFIG_DECLARATIONS(deluge, DELUGE);

#endif /* PAS_ENABLE_DELUGE */

#endif /* DELUGE_HEAP_CONFIG_H */

