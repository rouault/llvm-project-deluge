#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_heap_config.h"

#if PAS_ENABLE_FILC

#include "filc_heap_innards.h"
#include "pas_designated_intrinsic_heap.h"
#include "pas_heap_config_utils_inlines.h"
#include "pas_root.h"

PAS_BEGIN_EXTERN_C;

const pas_heap_config filc_heap_config = FILC_HEAP_CONFIG;

PAS_BASIC_HEAP_CONFIG_DEFINITIONS(
    filc, FILC,
    .allocate_page_should_zero = true,
    .intrinsic_view_cache_capacity = pas_heap_runtime_config_aggressive_view_cache_capacity);

void filc_heap_config_activate(void)
{
#if PAS_OS(DARWIN)
    static const bool register_with_libmalloc = true;
#endif
    
    pas_designated_intrinsic_heap_initialize(&filc_int_heap.segregated_heap,
                                             &filc_heap_config);

#if PAS_OS(DARWIN)
    if (register_with_libmalloc)
        pas_root_ensure_for_libmalloc_enumeration();
#endif
}

PAS_END_EXTERN_C;

#endif /* PAS_ENABLE_FILC */

#endif /* LIBPAS_ENABLED */
