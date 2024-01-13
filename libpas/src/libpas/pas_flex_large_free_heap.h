#ifndef PAS_FLEX_LARGE_FREE_HEAP_H
#define PAS_FLEX_LARGE_FREE_HEAP_H

#include "pas_allocation_result.h"
#include "pas_large_free_heap_config.h"
#include "pas_large_free_visitor.h"
#include "pas_range.h"
#include "pas_red_black_tree.h"

PAS_BEGIN_EXTERN_C;

struct pas_flex_large_free_heap;
struct pas_flex_large_free_node;
typedef struct pas_flex_large_free_heap pas_flex_large_free_heap;
typedef struct pas_flex_large_free_node pas_flex_large_free_node;

struct pas_flex_large_free_heap {
    pas_red_black_tree tree;
};

struct pas_flex_large_free_node {
    pas_red_black_tree_node node;
    pas_range range;
};

PAS_API void pas_flex_large_free_heap_construct(pas_flex_large_free_heap* heap);

PAS_API pas_range pas_flex_large_free_heap_try_allocate(pas_flex_large_free_heap* heap,
                                                        size_t size,
                                                        size_t alignment,
                                                        pas_large_free_heap_config* config);

PAS_API void pas_flex_large_free_heap_deallocate(pas_flex_large_free_heap* heap,
                                                 pas_range range);

PAS_API void pas_flex_large_free_heap_for_each_free(pas_flex_large_free_heap* heap,
                                                    pas_large_free_visitor visitor,
                                                    void* arg);

PAS_API size_t pas_flex_large_free_heap_get_num_free_bytes(pas_flex_large_free_heap* heap);

PAS_END_EXTERN_C;

#endif /* PAS_FLEX_LARGE_FREE_HEAP_H */

