/*
 * Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FUGC_H
#define FUGC_H

#include "filc_runtime.h"
#include "verse_heap.h"

/* This contains the collector loop portion of Fil's Unbelievable Garbage Collector.
   
   Most of what makes this GC possible is scattered throughout the runtime and compiler. It's core to
   how Fil-C works. Examples: pollchecks, enter/exit, filc_thread, using the verse_heap, store barriers,
   etc. */

PAS_API void fugc_initialize(void);

/* Needed for fork(). Has no other purpose. */
PAS_API void fugc_suspend(void);
PAS_API void fugc_resume(void);

/* Needed for munmap/mprotect support. */
PAS_API void fugc_handshake(void);

static inline void fugc_mark(filc_object_array* mark_stack, filc_object* object)
{
    static const bool verbose = false;
    if (verbose)
        pas_log("Marking object %p\n", object);
    if (!object)
        return;
    if ((object->flags & FILC_OBJECT_FLAG_GLOBAL))
        return;
    if (!verse_heap_set_is_marked_relaxed(object, true))
        return;
    filc_object_array_push(mark_stack, object);
}

static inline void fugc_mark_or_free(filc_object_array* mark_stack, filc_ptr* ptr)
{
    for (;;) {
        filc_object* object = filc_ptr_load_object(ptr);
        if (!object)
            return;
        if (!(object->flags & FILC_OBJECT_FLAG_FREE)) {
            fugc_mark(mark_stack, object);
            return;
        }
        if (object == filc_free_singleton)
            return;
        if (filc_ptr_unfenced_unbarriered_weak_cas_object(ptr, object, filc_free_singleton))
            return;
    }
}

PAS_API void fugc_donate(filc_object_array* mark_stack);

/* Request that a collection cycle begins. If one is already running, then that's the one you get.
 
   Normally, GCs are requested by the verse_heap calling the verse_heap_live_bytes_trigger_callback(),
   which FUGC registers. So, under normal operation, you shouldn't have to use this. */
PAS_API uint64_t fugc_request(void);

/* Request that a totally fresh collection cycle begins. If one is already running, make sure another
   one is scheduled right after it. */
PAS_API uint64_t fugc_request_fresh(void);

/* Wait for the given collection cycle to finish.
 
   To do the equivalent of "System.gc()", you do fugc_wait(fugc_request_fresh()). */
PAS_API void fugc_wait(uint64_t cycle);

PAS_API void fugc_dump_setup(void);

#endif /* FUGC_H */

