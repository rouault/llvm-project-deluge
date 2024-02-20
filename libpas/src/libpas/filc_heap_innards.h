#ifndef FILC_HEAP_INNARDS_H
#define FILC_HEAP_INNARDS_H

#include "filc_runtime.h"
#include "pas_config.h"
#include "pas_allocator_counts.h"
#include "pas_dynamic_primitive_heap_map.h"
#include "pas_intrinsic_heap_support.h"
#include "pas_heap_ref.h"

#if PAS_ENABLE_FILC

PAS_BEGIN_EXTERN_C;

PAS_API extern pas_heap filc_int_heap;
PAS_API extern pas_intrinsic_heap_support filc_int_heap_support;
PAS_API extern pas_heap filc_utility_heap;
PAS_API extern pas_intrinsic_heap_support filc_utility_heap_support;
PAS_API extern pas_heap filc_hard_int_heap;
PAS_API extern pas_intrinsic_heap_support filc_hard_int_heap_support;
PAS_API extern pas_allocator_counts filc_allocator_counts;

PAS_END_EXTERN_C;

#endif /* PAS_ENABLE_FILC */

#endif /* FILC_HEAP_INNARDS_h */

