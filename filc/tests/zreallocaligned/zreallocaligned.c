#include <stdfil.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

static bool is_aligned(void* p, size_t alignment)
{
    ZASSERT(alignment && !(alignment & (alignment - 1)));
    return !((uintptr_t)p & (alignment - 1));
}

int main()
{
    ZASSERT(is_aligned(zgc_aligned_alloc(8,    0),    8));
    ZASSERT(is_aligned(zgc_aligned_alloc(8,    1),    8));
    ZASSERT(is_aligned(zgc_aligned_alloc(8,    8),    8));
    ZASSERT(is_aligned(zgc_aligned_alloc(8,    16),   8));
    ZASSERT(is_aligned(zgc_aligned_alloc(16,   0),    16));
    ZASSERT(is_aligned(zgc_aligned_alloc(16,   1),    16));
    ZASSERT(is_aligned(zgc_aligned_alloc(16,   8),    16));
    ZASSERT(is_aligned(zgc_aligned_alloc(16,   16),   16));
    ZASSERT(is_aligned(zgc_aligned_alloc(16,   32),   16));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   0),    32));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   1),    32));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   8),    32));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   16),   32));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   32),   32));
    ZASSERT(is_aligned(zgc_aligned_alloc(32,   64),   32));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   0),    64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   1),    64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   8),    64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   16),   64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   32),   64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   64),   64));
    ZASSERT(is_aligned(zgc_aligned_alloc(64,   128),  64));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 0),    4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 1),    4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 8),    4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 16),   4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 32),   4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 4096), 4096));
    ZASSERT(is_aligned(zgc_aligned_alloc(4096, 8192), 4096));
    
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 8,    0),    8));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 8,    1),    8));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 8,    8),    8));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 8,    16),   8));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 16,   0),    16));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 16,   1),    16));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 16,   8),    16));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 16,   16),   16));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 16,   32),   16));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   0),    32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   1),    32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   8),    32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   16),   32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   32),   32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 32,   64),   32));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   0),    64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   1),    64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   8),    64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   16),   64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   32),   64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   64),   64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 64,   128),  64));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 0),    4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 1),    4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 8),    4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 16),   4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 32),   4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 4096), 4096));
    ZASSERT(is_aligned(zgc_aligned_realloc(zgc_alloc(100), 4096, 8192), 4096));
    
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(8,    100), 0),    8));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(8,    100), 1),    8));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(8,    100), 8),    8));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(8,    100), 16),   8));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(16,   100), 0),    16));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(16,   100), 1),    16));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(16,   100), 8),    16));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(16,   100), 16),   16));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(16,   100), 32),   16));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 0),    32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 1),    32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 8),    32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 16),   32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 32),   32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(32,   100), 64),   32));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 0),    64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 1),    64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 8),    64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 16),   64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 32),   64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 64),   64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(64,   100), 128),  64));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 0),    4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 1),    4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 8),    4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 16),   4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 32),   4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 4096), 4096));
    ZASSERT(is_aligned(zgc_realloc_preserving_alignment(zgc_aligned_alloc(4096, 100), 8192), 4096));

    return 0;
}

