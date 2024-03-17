/* Copyright Epic Games, Inc. All Rights Reserved. */

#include "pas_config.h"

#if LIBPAS_ENABLED

#include "verse_heap_chunk_map_entry.h"

#include "verse_heap_large_entry.h"
#include "pas_stream.h"

#if PAS_ENABLE_VERSE

void verse_heap_chunk_map_entry_header_dump(verse_heap_chunk_map_entry_header header, pas_stream* stream)
{
    if (verse_heap_chunk_map_entry_header_is_empty(header)) {
        pas_stream_printf(stream, "empty");
        return;
    }

    if (verse_heap_chunk_map_entry_header_is_small_segregated(header)) {
        pas_stream_printf(stream, "small");
        return;
    }

    if (verse_heap_chunk_map_entry_header_is_medium_segregated(header)) {
        pas_stream_printf(
            stream, "medium:%p/%s",
            verse_heap_chunk_map_entry_header_medium_segregated_header_object(header),
            pas_empty_mode_get_string(
                verse_heap_chunk_map_entry_header_medium_segregated_empty_mode(header)));
        return;
    }

    if (verse_heap_chunk_map_entry_header_is_large(header)) {
        verse_heap_large_entry* large_entry;

        large_entry = verse_heap_chunk_map_entry_header_large_entry(header);

        pas_stream_printf(stream, "large:%zx-%zx,%p", large_entry->begin, large_entry->end, large_entry->heap);
        return;
    }

    PAS_ASSERT(!"Should not be reached");
}

void verse_heap_chunk_map_entry_dump(verse_heap_chunk_map_entry entry, pas_stream* stream)
{
    verse_heap_chunk_map_entry_header header;
    header = verse_heap_chunk_map_entry_get_header(entry);
    if (verse_heap_chunk_map_entry_header_is_small_segregated(header)) {
        size_t index;
        pas_stream_printf(stream, "small:");
        for (index = 0; index < VERSE_HEAP_CHUNK_MAP_ENTRY_NUM_WORDS; ++index) {
            pas_stream_printf(
                stream, "%08x",
                verse_heap_chunk_map_entry_small_segregated_ownership_bitvector(&entry)[index]);
        }
        return;
    }

    verse_heap_chunk_map_entry_header_dump(header, stream);
}

#endif /* PAS_ENABLE_VERSE */

#endif /* LIBPAS_ENABLED */

