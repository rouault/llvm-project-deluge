/* Copyright Epic Games, Inc. All Rights Reserved. */

#ifndef VERSE_HEAP_CONFIG_UE_H
#define VERSE_HEAP_CONFIG_UE_H

/* This has to be the max of what the system supports (so it's wrong for Darwin/arm64, for example).

   FIXME: This should just match PAS_GRANULE_DEFAULT_SHIFT, but I'm not sure that thing is dialed in. */
/* This has to be the max of what the system supports. */
#if (defined(__arm64__) && defined(__APPLE__)) || defined(__SCE__)
#define VERSE_HEAP_PAGE_SIZE_SHIFT 14u
#else
#define VERSE_HEAP_PAGE_SIZE_SHIFT 12u
#endif
#define VERSE_HEAP_PAGE_SIZE (1u << VERSE_HEAP_PAGE_SIZE_SHIFT)

/* The first page of a chunk is always the mark bits for the whole chunk. */
#define VERSE_HEAP_CHUNK_SIZE_SHIFT (VERSE_HEAP_PAGE_SIZE_SHIFT + VERSE_HEAP_MIN_ALIGN_SHIFT + 3u)
#define VERSE_HEAP_CHUNK_SIZE (1u << VERSE_HEAP_CHUNK_SIZE_SHIFT)

#define VERSE_HEAP_MIN_ALIGN_SHIFT 4u
#define VERSE_HEAP_MIN_ALIGN (1u << VERSE_HEAP_MIN_ALIGN_SHIFT)

#define VERSE_HEAP_SMALL_SEGREGATED_MIN_ALIGN_SHIFT VERSE_HEAP_MIN_ALIGN_SHIFT
#define VERSE_HEAP_SMALL_SEGREGATED_MIN_ALIGN (1u << VERSE_HEAP_SMALL_SEGREGATED_MIN_ALIGN_SHIFT)
#define VERSE_HEAP_SMALL_SEGREGATED_PAGE_SIZE 16384
#define VERSE_HEAP_SMALL_SEGREGATED_PAGES_PER_CHUNK \
    (VERSE_HEAP_CHUNK_SIZE / VERSE_HEAP_SMALL_SEGREGATED_PAGE_SIZE)

#define VERSE_HEAP_MEDIUM_SEGREGATED_MIN_ALIGN_SHIFT 9u
#define VERSE_HEAP_MEDIUM_SEGREGATED_MIN_ALIGN (1u << VERSE_HEAP_MEDIUM_SEGREGATED_MIN_ALIGN_SHIFT)
#define VERSE_HEAP_MEDIUM_SEGREGATED_PAGE_SIZE VERSE_HEAP_CHUNK_SIZE

#endif /* VERSE_HEAP_CONFIG_UE_H */
