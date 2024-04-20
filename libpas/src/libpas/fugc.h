#ifndef FUGC_H
#define FUGC_H

#include "filc_runtime.h"
#include "verse_heap.h"

/* This contains the collector loop portion of Fil's Unbelievable Garbage Collector.
   
   Most of what makes this GC possible is scattered throughout the runtime and compiler. It's core to
   how Fil-C works. Examples: pollchecks, enter/exit, filc_thread, using the verse_heap, store barriers,
   etc. */

PAS_API extern bool fugc_world_is_stopped;

PAS_API void fugc_initialize(void);

/* Needed for fork(). Has no other purpose. */
PAS_API void fugc_suspend(void);
PAS_API void fugc_resume(void);

static inline void fugc_mark(filc_object_array* mark_stack, filc_object* object)
{
    if (!object)
        return;
    PAS_TESTING_ASSERT(!(object->flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
    if ((object->flags & FILC_OBJECT_FLAG_GLOBAL))
        return;
    if (!verse_heap_set_is_marked_relaxed(object, true))
        return;
    filc_object_array_push(mark_stack, object);
}

static inline void fugc_mark_or_free(filc_object_array* mark_stack, filc_ptr* ptr)
{
    for (;;) {
        filc_ptr value = filc_ptr_load_with_manual_tracking(ptr);
        filc_object* object = filc_ptr_object(value);
        if (!object)
            return;
        if (!(object->flags & FILC_OBJECT_FLAG_FREE)) {
            fugc_mark(mark_stack, object);
            return;
        }
        if (filc_ptr_object(value) == filc_free_singleton)
            return;
        filc_ptr new_value = filc_ptr_with_ptr(
            filc_ptr_create_with_manual_tracking(filc_free_singleton), filc_ptr_ptr(value));
        if (filc_ptr_unfenced_unbarriered_weak_cas(ptr, value, new_value))
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

