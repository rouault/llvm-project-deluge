#ifndef DELUGE_HEAP_INNARDS_H
#define DELUGE_HEAP_INNARDS_H

#include "deluge_runtime.h"
#include "pas_config.h"
#include "pas_allocator_counts.h"
#include "pas_dynamic_primitive_heap_map.h"
#include "pas_intrinsic_heap_support.h"
#include "pas_heap_ref.h"

#if PAS_ENABLE_DELUGE

PAS_BEGIN_EXTERN_C;

PAS_API extern pas_heap deluge_int_heap;
PAS_API extern pas_intrinsic_heap_support deluge_int_heap_support;
PAS_API extern pas_heap deluge_utility_heap;
PAS_API extern pas_intrinsic_heap_support deluge_utility_heap_support;
PAS_API extern pas_heap deluge_hard_heap;
PAS_API extern pas_intrinsic_heap_support deluge_hard_heap_support;
PAS_API extern pas_allocator_counts deluge_allocator_counts;

PAS_END_EXTERN_C;

#endif /* PAS_ENABLE_DELUGE */

#endif /* DELUGE_HEAP_INNARDS_h */

