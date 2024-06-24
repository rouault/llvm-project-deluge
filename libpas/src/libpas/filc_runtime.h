/*
 * Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
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

#ifndef FILC_RUNTIME_H
#define FILC_RUNTIME_H

#include "bmalloc_heap.h"
#include "pas_allocation_config.h"
#include "pas_hashtable.h"
#include "pas_heap_ref.h"
#include "pas_lock.h"
#include "pas_lock_free_read_ptr_ptr_hashtable.h"
#include "pas_ptr_hash_map.h"
#include "pas_range.h"
#include "pas_segmented_vector.h"
#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>

PAS_BEGIN_EXTERN_C;

/* Internal FilC runtime header, defining how the FilC runtime maintains its state.
 
   Currently, including this header is the only way to perform FFI to FilC code, and the API for
   that is too low-level for comfort. That's probably fine, since the FilC ABI is going to
   change the moment I start giving a shit about performance.

   This runtime is engineered under the following principles:

   - It's based on libpas with the verse_heap, so we get to have a concurrent GC.

   - Coding standards have to be extremely high, and assert usage should be full-ass
     belt-and-suspenders. The goal of this code is to achieve memory safety under the FilC
     "Bounded P^I" type system. It's fine to take extra cycles or bytes to achieve that goal.

   - There are no optimizations yet, but the structure of this code is such that when I choose to 
     go into optimization mode, I will be able to wreak havoc I'm just holding back from going
     there, for now. Lots of running code is better than a small amount of fast code. */

struct filc_constant_relocation;
struct filc_constexpr_node;
struct filc_exception_and_int;
struct filc_frame;
struct filc_function_origin;
struct filc_global_initialization_context;
struct filc_jmp_buf;
struct filc_native_frame;
struct filc_object;
struct filc_object_array;
struct filc_origin;
struct filc_origin_with_eh;
struct filc_ptr;
struct filc_ptr_table;
struct filc_ptr_table_array;
struct filc_ptr_uintptr_hash_map_entry;
struct filc_signal_handler;
struct filc_return_buffer;
struct filc_thread;
struct pas_basic_heap_runtime_config;
struct pas_stream;
struct pas_thread_local_cache_node;
struct verse_heap_object_set;
typedef struct filc_constant_relocation filc_constant_relocation;
typedef struct filc_constexpr_node filc_constexpr_node;
typedef struct filc_exception_and_int filc_exception_and_int;
typedef struct filc_frame filc_frame;
typedef struct filc_function_origin filc_function_origin;
typedef struct filc_global_initialization_context filc_global_initialization_context;
typedef struct filc_jmp_buf filc_jmp_buf;
typedef struct filc_native_frame filc_native_frame;
typedef struct filc_object filc_object;
typedef struct filc_object_array filc_object_array;
typedef struct filc_origin filc_origin;
typedef struct filc_origin_with_eh filc_origin_with_eh;
typedef struct filc_ptr filc_ptr;
typedef struct filc_ptr_table filc_ptr_table;
typedef struct filc_ptr_table_array filc_ptr_table_array;
typedef struct filc_ptr_uintptr_hash_map_entry filc_ptr_uintptr_hash_map_entry;
typedef struct filc_return_buffer filc_return_buffer;
typedef struct filc_signal_handler filc_signal_handler;
typedef struct filc_thread filc_thread;
typedef struct pas_basic_heap_runtime_config pas_basic_heap_runtime_config;
typedef struct pas_stream pas_stream;
typedef struct pas_thread_local_cache_node pas_thread_local_cache_node;
typedef struct verse_heap_object_set verse_heap_object_set;

typedef uint8_t filc_word_type;
typedef uint16_t filc_object_flags;

/* Each word in memory monotonically transitions type. The lattice is:

   unset -> int
   unset -> ptr
   unset -> free
   int -> free
   ptr -> free
   dirstream -> free
   
   Note that some states (dirstream, function, and thread) have no transition from unset. These are
   allocated with filc_allocate_special, which sets the type straight away. These special types all
   have upper = lower + 16 even though the payload is not 16 bytes!

   Note that some of the states have no transition to free.

   Hence, some types have no edges in the lattice (if they have no transition from unset and no
   transition to free). */
#define FILC_WORD_TYPE_UNSET              ((filc_word_type)0)     /* 128-bit word whose type hasn't been
                                                                     set yet. */
#define FILC_WORD_TYPE_INT                ((filc_word_type)1)     /* 128-bit word that contains ints. */
#define FILC_WORD_TYPE_PTR                ((filc_word_type)2)     /* 128-bit word that contains a ptr. */
#define FILC_WORD_TYPE_FREE               ((filc_word_type)3)     /* 128-bit word that has been freed. */
#define FILC_WORD_TYPE_FUNCTION           ((filc_word_type)4)     /* Indicates the special function type.
                                                                     The lower points at the function but
                                                                     the GC-allocated payload is empty. */
#define FILC_WORD_TYPE_THREAD             ((filc_word_type)5)     /* Indicates the special thread type.
                                                                     The lower points at the payload. */
#define FILC_WORD_TYPE_DIRSTREAM          ((filc_word_type)6)     /* Indicates the special dirstream type.
                                                                     The lower points at the payload. */
#define FILC_WORD_TYPE_SIGNAL_HANDLER     ((filc_word_type)7)     /* Indicates the special signal_handler.
                                                                     The lower points at the payload. */
#define FILC_WORD_TYPE_PTR_TABLE          ((filc_word_type)8)     /* Indicates the special ptr_table.
                                                                     The lower points at the payload. */
#define FILC_WORD_TYPE_PTR_TABLE_ARRAY    ((filc_word_type)9)     /* Indicates the special
                                                                     ptr_table_array. The lower points at
                                                                     the payload. */
#define FILC_WORD_TYPE_DL_HANDLE          ((filc_word_type)10)    /* Indicates the special dlopen handle
                                                                     type. The lower points at the hanle
                                                                     but the GC-allocated payload is
                                                                     empty. */
#define FILC_WORD_TYPE_JMP_BUF            ((filc_word_type)11)    /* Indicates the special jmp_buf type.
                                                                     The lower points at the payload. */

#define FILC_WORD_SIZE                    sizeof(pas_uint128)

#define FILC_OBJECT_FLAG_FREE             ((filc_object_flags)1)  /* The object has been freed. */
#define FILC_OBJECT_FLAG_RETURN_BUFFER    ((filc_object_flags)2)  /* This is a return buffer (so it's not
                                                                     GC'd and should never be seen by GC).
                                                                     Useful for assertions only! */
#define FILC_OBJECT_FLAG_SPECIAL          ((filc_object_flags)4)  /* It's a special object. If there are
                                                                     no words, or any of them are
                                                                     unset/int/ptr, then this cannot be
                                                                     set. If this is set, then there must
                                                                     be one word, and that word must be
                                                                     one of free/function/
                                                                     thread/dirstream/signal_handler. */
#define FILC_OBJECT_FLAG_GLOBAL           ((filc_object_flags)8)  /* Pointer to a global, so cannot be
                                                                     freed. */
#define FILC_OBJECT_FLAG_MMAP             ((filc_object_flags)16) /* Pointer to mmap. */
#define FILC_OBJECT_FLAG_READONLY         ((filc_object_flags)32) /* Object is readonly. */
#define FILC_OBJECT_FLAGS_PIN_SHIFT       ((filc_object_flags)6)  /* Data is pinned by the runtime, so
                                                                     cannot be freed. This is only useful
                                                                     for munmap scenarios. */

#define FILC_MAX_MUSL_SIGNUM              31u
                                          
#define FILC_THREAD_STATE_ENTERED         ((uint8_t)1)
#define FILC_THREAD_STATE_CHECK_REQUESTED ((uint8_t)2)
#define FILC_THREAD_STATE_STOP_REQUESTED  ((uint8_t)4)
#define FILC_THREAD_STATE_DEFERRED_SIGNAL ((uint8_t)8)

#define FILC_MAX_BYTES_BETWEEN_POLLCHECKS ((size_t)1000)

#define FILC_PTR_TABLE_OFFSET             ((uintptr_t)66666)
#define FILC_PTR_TABLE_SHIFT              ((uintptr_t)4)

#define FILC_NUM_UNWIND_REGISTERS         2u

#define PIZLONATED_SIGNATURE \
    filc_thread* my_thread, \
    filc_ptr args, \
    filc_ptr rets

#define FILC_DEFINE_RUNTIME_ORIGIN_WITH_FILENAME(origin_name, function_name, passed_num_objects, passed_filename) \
    static const filc_function_origin function_ ## origin_name = { \
        .function = (function_name), \
        .filename = (passed_filename), \
        .num_objects = (passed_num_objects), \
        .personality_getter = NULL, \
        .can_throw = true, \
        .can_catch = false, \
        .num_setjmps = 0 \
    }; \
    static const filc_origin origin_name = { \
        .function_origin = &function_ ## origin_name, \
        .line = 0, \
        .column = 0 \
    }

#define FILC_DEFINE_RUNTIME_ORIGIN(origin_name, function_name, passed_num_objects) \
    FILC_DEFINE_RUNTIME_ORIGIN_WITH_FILENAME(origin_name, function_name, passed_num_objects, "<runtime>")

struct PAS_ALIGNED(FILC_WORD_SIZE) filc_ptr {
    pas_uint128 word;
};

enum filc_access_kind {
    filc_read_access,

    /* Since there is no write-only data, checking for write means you're also checking for
       read. */
    filc_write_access,
};

typedef enum filc_access_kind filc_access_kind;

struct filc_object {
    /* NOTE: In the interest of simplicity, we say that lower and upper have to be word-aligned for
       non-special objects and that upper-lower (i.e. the size) has to be word-aligned no matter
       what.
       
       This means that the minimum allocation size is 16 bytes.
       
       This isn't a strict requirement for the system to work. It's only a requirement for things
       that have ptrs. For now we make it a requirement (and obey it in the compiler and assert it
       in the runtime) because it makes things more obvious. */
    
    void* lower;
    void* upper;
    filc_object_flags flags;
    filc_word_type word_types[];
};

/* The size of filc_object if it's a special object. This is based on special objects having just one
   word_type.

   Note that the compiler pass hardcodes this constant. */
#define FILC_SPECIAL_OBJECT_SIZE \
    PAS_ROUND_UP_TO_POWER_OF_2(PAS_OFFSETOF(filc_object, word_types) + 1, FILC_WORD_SIZE)

struct filc_return_buffer {
    void* lower;
    void* upper;
    filc_object_flags flags;
    filc_word_type word_type;
    pas_uint128 data;
};

/* NOTE: A function may have two different function origins - one for origins that are capable of
   catching, and one for origins not capable of catching. */
struct filc_function_origin {
    const char* function;
    const char* filename;

    unsigned num_objects;
    
    /* If this is not NULL, then the filc_origin is really a filc_origin_with_eh, so that it includes
       the eh_data_getter.
    
       We store this as a pointer to the getter for the personality function so that the origin
       struct doesn't need our linking tricks. That implies that the personality function cannot be
       an arbitrary llvm::Constant; it must be a llvm::Function. That's fine since clang will never
       do anything but that.
    
       If this is not NULL, then the function can handle exceptions, which means that post-call
       pollchecks will check if the pollcheck returned FILC_POLLCHECK_EXCEPTION. */
    filc_ptr (*personality_getter)(filc_global_initialization_context*);

    /* Tells whether a function can throw exceptions. Note that a function might be can_catch and have
       a personality function, but not throw, or vice-versa. */
    bool can_throw;

    /* Tells whether a function can catch exceptions. All functions that have a personality_getter can
       also catch exceptions, but not necessarily the other way around. Also, can_catch could mean that
       the function is merely capable of passing exceptions through it (i.e. it doesn't catch anything
       but just rethrows - that's what happens if a C function is compiled with -fexceptions). */
    bool can_catch;

    /* The number of setjmps in the given function's frame. These are always the highest-indexed
       object slots. */
    unsigned num_setjmps;
};

struct filc_origin {
    const filc_function_origin* function_origin;
    unsigned line;
    unsigned column;
};

/* Origins are guaranteed to be of this type if !!origin->function_origin->personality_getter. */
struct filc_origin_with_eh {
    filc_origin base;

    filc_ptr (*eh_data_getter)(filc_global_initialization_context*);
};

#define FILC_FRAME_BODY \
    filc_frame* parent; \
    const filc_origin* origin

struct filc_frame {
    FILC_FRAME_BODY;
    filc_object* objects[];
};

struct filc_object_array {
    size_t num_objects;
    size_t objects_capacity;
    filc_object** objects;
};

struct filc_native_frame {
    filc_native_frame* parent;
    filc_object_array array;
    bool locked;
};

struct filc_signal_handler {
    filc_ptr function_ptr; /* This has to be pre-checked to actually be a callable function, but out
                              of an abundance of caution, we check it again anyway when calling it. */
    sigset_t mask;
    int musl_signum; /* This is only needed for assertion discipline. */
};

struct filc_thread {
    /* Begin fields that the compiler has to know about. */
    uint8_t state;
    filc_frame* top_frame;

    /* These are not tracked by GC, since they must be consumed by the landingpad right after
       calling filc_landing_pad. */
    filc_ptr unwind_registers[FILC_NUM_UNWIND_REGISTERS];
    
    /* End fields that the compiler has to know about. */
    
    filc_native_frame* top_native_frame;

    void (*pollcheck_callback)(filc_thread* my_thread, void* arg);
    void* pollcheck_arg;

    /* protected by the thread_list_lock. */
    filc_thread* next_thread;
    filc_thread* prev_thread;

    pas_thread_local_cache_node* tlc_node;
    uint64_t tlc_node_version;

    /* Array of allocated but not constructed objects. This needs to be an array, since we could
       get a signal in the middle of allocation and have more than one of these. */
    filc_object_array allocation_roots;

    filc_object_array mark_stack;

    pas_system_mutex lock; /* We grab all of these during fork(). */
    pas_system_condition cond;
    bool has_started; /* set to true when we actually commence starting the thread, after grabbing
                         the handshake/stw locks. so, crucially, writes are protected by both the
                         soft_handshake and the stop_the_world lock, and reads are protected by
                         either one. */
    bool is_stopping; /* set to true when the thread has proceeded far enough in the stop sequence
                         that it no longer has allocators to stop or a mark stack. written to
                         while entered and affects pollchecks only. */
    bool has_stopped; /* set to true when the thread is shut down. This is what you want for when
                         joining. */
    bool error_starting; /* set to true if we failed to start the thread. This is useful just for
                            the assertion in zthread_join that disallows joining on a thread that
                            wasn't actually ever started. */
    bool forked; /* set to true if this thread died due to forking. We use this to implement super
                    precise semantics in that case; it allows zthread_join to return false/ESRCH if
                    you try to join a thread that died due to fork. */
    pthread_t thread; /* the underlying thread is always detached and this stays non-NULL so long
                         as the thread is running.
                         
                         This is set to non-NULL the moment that the thread is fully started and
                         is set back to NULL when the thread starts stopping. */
    bool (*thread_main)(PIZLONATED_SIGNATURE);
    filc_ptr arg_ptr;
    filc_ptr result_ptr;
    filc_ptr cookie_ptr;

    filc_ptr unwind_context_ptr;
    filc_ptr exception_object_ptr;
    filc_frame* found_frame_for_unwind;

    sigset_t initial_blocked_sigs;

    /* musl relies on each thread having a 32-bit id, so we oblige. */
    unsigned tid;

    /* We allow deferring signals aside from running entered. This is rare but useful. If this count is
       nonzero, then the signal_pizlonator will not set DEFERRED_SIGNAL flag in the state, but will set
       have_deferred_signal_special. */
    unsigned special_signal_deferral_depth;
    bool have_deferred_signal_special;
    
    uint64_t num_deferred_signals[FILC_MAX_MUSL_SIGNUM + 1];
};

struct filc_global_initialization_context {
    size_t ref_count;
    
    /* Maps the location in memory that stores the persistent authoritative filc_ptr to the global
       to the filc_object* that we are in the process of initializing.
       
       Key: filc_ptr* pizlonated_gptr
       Value: filc_object* object */
    pas_ptr_hash_map map;
};

enum filc_constant_kind {
    /* The target is a getter that returns a pointer to the global.
     
       This is used for both functions and globals. */
    filc_global_constant,

    /* The target is a constexpr node. */
    filc_expr_constant
};

typedef enum filc_constant_kind filc_constant_kind;

enum filc_constexpr_opcode {
    filc_constexpr_add_ptr_immediate
};

typedef enum filc_constexpr_opcode filc_constexpr_opcode;

struct filc_constexpr_node {
    filc_constexpr_opcode opcode;

    /* This will eventually be an operand union, I guess? */
    filc_constant_kind left_kind;
    void* left_target;
    uintptr_t right_value;
};

struct filc_constant_relocation {
    size_t offset;
    filc_constant_kind kind;
    void* target;
};

typedef filc_ptr filc_ptr_uintptr_hash_map_key;

struct filc_ptr_uintptr_hash_map_entry {
    filc_ptr key;
    uintptr_t value;
};

static inline filc_ptr_uintptr_hash_map_entry filc_ptr_uintptr_hash_map_entry_create_empty(void)
{
    filc_ptr_uintptr_hash_map_entry result;
    result.key.word = 0;
    result.value = 0;
    return result;
}

static inline filc_ptr_uintptr_hash_map_entry
filc_ptr_uintptr_hash_map_entry_create_deleted(void)
{
    filc_ptr_uintptr_hash_map_entry result;
    result.key.word = 0;
    result.value = 1;
    return result;
}

static inline bool filc_ptr_uintptr_hash_map_entry_is_empty_or_deleted(
    filc_ptr_uintptr_hash_map_entry entry)
{
    if (!entry.key.word) {
        PAS_ASSERT(!entry.value || entry.value == 1);
        return true;
    }
    return false;
}

static inline bool
filc_ptr_uintptr_hash_map_entry_is_empty(filc_ptr_uintptr_hash_map_entry entry)
{
    if (!entry.key.word) {
        PAS_ASSERT(!entry.value || entry.value == 1);
        return !entry.value;
    }
    return false;
}

static inline bool
filc_ptr_uintptr_hash_map_entry_is_deleted(filc_ptr_uintptr_hash_map_entry entry)
{
    if (!entry.key.word) {
        PAS_ASSERT(!entry.value || entry.value == 1);
        return entry.value;
    }
    return false;
}

static inline filc_ptr
filc_ptr_uintptr_hash_map_entry_get_key(filc_ptr_uintptr_hash_map_entry entry)
{
    return entry.key;
}

static inline unsigned filc_ptr_uintptr_hash_map_key_get_hash(filc_ptr ptr)
{
    return pas_hash128(ptr.word);
}

static inline bool filc_ptr_uintptr_hash_map_key_is_equal(filc_ptr a, filc_ptr b)
{
    return a.word == b.word;
}

PAS_CREATE_HASHTABLE(filc_ptr_uintptr_hash_map,
                     filc_ptr_uintptr_hash_map_entry,
                     filc_ptr_uintptr_hash_map_key);

struct filc_ptr_table {
    pas_lock lock;
    filc_ptr_uintptr_hash_map encode_map;
    uintptr_t* free_indices;
    size_t num_free_indices;
    size_t free_indices_capacity;
    filc_ptr_table_array* array;
};

struct filc_ptr_table_array {
    size_t num_entries;
    size_t capacity;
    filc_ptr ptrs[];
};

struct filc_exception_and_int {
    bool has_exception;
    int value;
};

enum filc_jmp_buf_kind {
    filc_jmp_buf_setjmp,
    filc_jmp_buf__setjmp,
    filc_jmp_buf_sigsetjmp,
};

typedef enum filc_jmp_buf_kind filc_jmp_buf_kind;

struct filc_jmp_buf {
    /* The jmp_buf union must be the first thing since the compiler relies on it. */
    union {
        jmp_buf system_buf;
        sigjmp_buf system_sigbuf;
    } u;
    filc_jmp_buf_kind kind;
    filc_frame* saved_top_frame; /* This is only here for assertions since longjmp does a search to
                                    find the frame, and the frame knows about the jmp_buf. */
    filc_native_frame* saved_top_native_frame;
    size_t saved_allocation_roots_num_objects;
    /* Need to save the GC objects referenced at that point in the stack. These must be marked so
       long as the jmp_buf is around, and they must be splatted back into place when we longjmp. */
    size_t num_objects;
    filc_object* objects[];
};

#define FILC_FOR_EACH_LOCK(macro) \
    macro(thread_list); \
    macro(stop_the_world)

/* We use the system mutex for our global locks so that they are fork-friendly. The Darwin
   os_unfair_lock, which we use for most of libpas, is not fork-friendly. That's because
   os_unfair_lock has an assertion on unlock that the current thread holds the lock, and
   os_unfair_locks held across fork into the child are not seen as being held by the calling
   (child process) thread. */
#define FILC_DECLARE_LOCK(name) \
    PAS_API extern pas_system_mutex filc_ ## name ## _lock; \
    PAS_API void filc_ ## name ## _lock_lock(void); \
    PAS_API void filc_ ## name ## _lock_unlock(void); \
    PAS_API void filc_ ## name ## _lock_assert_held(void)
FILC_FOR_EACH_LOCK(FILC_DECLARE_LOCK);
#undef FILC_DECLARE_LOCK

/* These locks don't need to be held across fork, so no big deal. */
PAS_DECLARE_LOCK(filc_soft_handshake);
PAS_DECLARE_LOCK(filc_global_initialization);

PAS_API extern unsigned filc_stop_the_world_count;
PAS_API extern pas_system_condition filc_stop_the_world_cond;

PAS_API extern filc_thread* filc_first_thread;
PAS_API extern pthread_key_t filc_thread_key;

PAS_API extern bool filc_is_marking;

PAS_API extern pas_heap* filc_default_heap;
PAS_API extern pas_heap* filc_destructor_heap;
PAS_API extern verse_heap_object_set* filc_destructor_set;

PAS_API extern filc_object* filc_free_singleton;

PAS_API extern filc_object_array filc_global_variable_roots;

/* Anything that takes origin for checking has the following meaning:
   
   - If the origin is NULL, we just use the origin that's at the top of the stack already.
   - If the origin is not NULL, then this sets the top frame's origin to what is passed. */
void filc_safety_panic(const filc_origin* origin, const char* format, ...); /* memory safety */
void filc_internal_panic(const filc_origin* origin, const char* format, ...); /* internal error */
void filc_user_panic(const filc_origin* origin, const char* format, ...); /* user-triggered */

#define FILC_CHECK(exp, origin, ...) do { \
        if ((exp)) \
            break; \
        filc_safety_panic(origin, __VA_ARGS__); \
    } while (0)

/* Ideally, all FILC_ASSERTs would be turned into FILC_CHECKs.
 
   Also, some FILC_ASSERTs are asserting things that cannot happen unless the filc runtime or
   compiler are broken or memory safey was violated some other way; it would be great to better
   distinguish those. Most of them aren't FILC_TESTING_ASSERTs. */
#define FILC_ASSERT(exp, origin) do { \
        if ((exp)) \
            break; \
        filc_safety_panic( \
            origin, "%s:%d: %s: safety assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

#define FILC_TESTING_ASSERT(exp, origin) do { \
        if (!PAS_ENABLE_TESTING) \
            break; \
        if ((exp)) \
            break; \
        filc_internal_panic( \
            origin, "%s:%d: %s: testing assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

/* Must be called from CRT before any FilC happens. If we ever allow FilC dylibs to be loaded 
   into non-FilC code, then we'll have to call it from compiler-generated initializers, too. It's
   not fine to call this more than once or at any other time than in the CRT. */
PAS_API void filc_initialize(void);

PAS_API filc_thread* filc_thread_create(void);

PAS_API void filc_thread_mark_outgoing_ptrs(filc_thread* thread, filc_object_array* stack);
PAS_API void filc_thread_destruct(filc_thread* thread);

/* Gives the thread's tid back. Has to be done while still entered. */
PAS_API void filc_thread_relinquish_tid(filc_thread* thread);

/* This undoes thread creation. It destroys the things that are normally destroyed by end of
   start_thread, or in the case where the thread had an error starting. */
PAS_API void filc_thread_undo_create(filc_thread* thread);

/* This removes the thread from the thread list and reuses its tid. */
PAS_API void filc_thread_dispose(filc_thread* thread);

PAS_API filc_thread* filc_get_my_thread(void);

static inline bool filc_thread_is_entered(filc_thread* thread)
{
    return thread->state & FILC_THREAD_STATE_ENTERED;
}

PAS_API void filc_assert_my_thread_is_not_entered(void);

PAS_API void filc_soft_handshake_no_op_callback(filc_thread* my_thread, void* arg);

/* Calls the callback from every thread. Returns when every thread has done so. */
PAS_API void filc_soft_handshake(void (*callback)(filc_thread* my_thread, void* arg), void* arg);

PAS_API void filc_stop_the_world(void);
PAS_API void filc_resume_the_world(void);

PAS_API void filc_wait_for_world_resumption_holding_lock(void);

/* Begin execution in Fil-C. Executing Fil-C comes with the promise that you'll periodically do
   a pollcheck and that all signals will be deferred to pollchecks. */
PAS_API void filc_enter(filc_thread* my_thread);

/* End execution in Fil-C. Call this before doing anything that might block or anything to
   affect signal masks.
   
   You can exit and then reenter as much as you like. It'll be super cheap eventually. */
PAS_API void filc_exit(filc_thread* my_thread);

/* These have to be called entered, currently. The only thing stopping us from making them work
   exited is that then, decrease_special_signal_deferral_depth would have to
   handle_deferred_signals. */
PAS_API void filc_increase_special_signal_deferral_depth(filc_thread* my_thread);
PAS_API void filc_decrease_special_signal_deferral_depth(filc_thread* my_thread);

/* It's hilarious that these are outline function calls right now. It's also hilarious that pop_frame
   takes the frame. In the future, it'll only use it for assertions. */
static inline void filc_push_frame(filc_thread* my_thread, filc_frame* frame)
{
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_TESTING_ASSERT(my_thread->top_frame != frame);
    frame->parent = my_thread->top_frame;
    my_thread->top_frame = frame;
}
static inline void filc_pop_frame(filc_thread* my_thread, filc_frame* frame)
{
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_TESTING_ASSERT(my_thread->top_frame == frame);
    my_thread->top_frame = frame->parent;
}

static inline void filc_object_array_construct(filc_object_array* array)
{
    array->num_objects = 0;
    array->objects_capacity = 0;
    array->objects = NULL;
}

static inline void filc_object_array_destruct(filc_object_array* array)
{
    bmalloc_deallocate(array->objects);
}

PAS_API void filc_object_array_push(filc_object_array* array, filc_object* object);

static filc_object* filc_object_array_pop(filc_object_array* array)
{
    if (!array->num_objects)
        return NULL;
    return array->objects[--array->num_objects];
}

PAS_API void filc_object_array_reset(filc_object_array* array);
PAS_API void filc_object_array_push_all(filc_object_array* to, filc_object_array* from);
PAS_API void filc_object_array_pop_all_from_and_push_to(filc_object_array* from,
                                                        filc_object_array* to);

static inline void filc_push_allocation_root(filc_thread* my_thread, filc_object* allocation_root)
{
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    filc_object_array_push(&my_thread->allocation_roots, allocation_root);
}

static inline void filc_pop_allocation_root(filc_thread* my_thread, filc_object* allocation_root)
{
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_ASSERT(filc_object_array_pop(&my_thread->allocation_roots) == allocation_root);
}

PAS_API void filc_enter_with_allocation_root(filc_thread* my_thread, filc_object* allocation_root);
PAS_API void filc_exit_with_allocation_root(filc_thread* my_thread, filc_object* allocation_root);

/* Locking the native frame prevents us from accidentally adding stuff to the top_native_frame if
   it doesn't belong to us. */
static inline void filc_native_frame_lock(filc_native_frame* frame)
{
    PAS_ASSERT(!frame->locked);
    frame->locked = true;
}

static inline void filc_native_frame_unlock(filc_native_frame* frame)
{
    PAS_ASSERT(frame->locked);
    frame->locked = false;
}

static inline void filc_native_frame_assert_locked(filc_native_frame* frame)
{
    PAS_ASSERT(frame->locked);
}

static inline void filc_lock_top_native_frame(filc_thread* thread)
{
    if (thread->top_native_frame)
        filc_native_frame_lock(thread->top_native_frame);
}

static inline void filc_unlock_top_native_frame(filc_thread* thread)
{
    if (thread->top_native_frame)
        filc_native_frame_unlock(thread->top_native_frame);
}

static inline void filc_assert_top_frame_locked(filc_thread* thread)
{
    if (thread->top_native_frame)
        filc_native_frame_assert_locked(thread->top_native_frame);
}

static inline void filc_push_native_frame(filc_thread* my_thread, filc_native_frame* frame)
{
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    filc_object_array_construct(&frame->array);
    frame->locked = false;
    
    PAS_TESTING_ASSERT(my_thread->top_native_frame != frame);
    filc_assert_top_frame_locked(my_thread);
    frame->parent = my_thread->top_native_frame;
    my_thread->top_native_frame = frame;
}

static inline void filc_pop_native_frame(filc_thread* my_thread, filc_native_frame* frame)
{
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    filc_object_array_destruct(&frame->array);
    PAS_TESTING_ASSERT(!frame->locked);
    
    PAS_TESTING_ASSERT(my_thread->top_native_frame == frame);
    my_thread->top_native_frame = frame->parent;
}

PAS_API void filc_native_frame_add(filc_native_frame* frame, filc_object* object);

/* Requires that we have a top_native_frame, so can only be called from native functions. */
PAS_API void filc_thread_track_object(filc_thread* my_thread, filc_object* object);

PAS_API void filc_pollcheck_slow(filc_thread* my_thread, const filc_origin* origin);

/* Check if the GC needs us to do work. Also check if there's a pending signal, and if so, run its
   handler.
   
   This mechanism allows us to have signal handlers that allocate even though the allocator uses
   locks. It also means that signal handlers can call into almost all stdfil API and all
   compiler-facing runtime API.
   
   This mechanism also allows us to handle GC safepoints.

   Only call this inside Fil-C execution and never after exiting.

   Returns true if the pollcheck fired. */
static inline bool filc_pollcheck(filc_thread* my_thread, const filc_origin* origin)
{
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    if ((my_thread->state & (FILC_THREAD_STATE_CHECK_REQUESTED |
                             FILC_THREAD_STATE_STOP_REQUESTED |
                             FILC_THREAD_STATE_DEFERRED_SIGNAL))) {
        filc_pollcheck_slow(my_thread, origin);
        return true;
    }
    return false;
}

/* This is purely to make it easier for the compiler to emit pollchecks for now. It's a bug that
   the compiler uses this, but, like, fugc it. */
PAS_API void filc_pollcheck_outline(filc_thread* my_thread, const filc_origin* origin);

void filc_thread_stop_allocators(filc_thread* my_thread);
void filc_thread_mark_roots(filc_thread* my_thread);
void filc_thread_sweep_mark_stack(filc_thread* my_thread);
void filc_thread_donate(filc_thread* my_thread);

void filc_mark_global_roots(filc_object_array* mark_stack);

void filc_origin_dump(const filc_origin* origin, pas_stream* stream);

void filc_thread_dump_stack(filc_thread* thread, pas_stream* stream);

void filc_validate_object(filc_object* object, const filc_origin* origin);
void filc_validate_normal_object(filc_object* object, const filc_origin* origin);
void filc_validate_return_buffer_object(filc_object* object, const filc_origin* origin);

static inline void filc_testing_validate_object(filc_object* object, const filc_origin* origin)
{
    if (PAS_ENABLE_TESTING)
        filc_validate_object(object, origin);
}

/* Run assertions on the ptr itself. The runtime isn't guaranteed to ever run this check. Pointers
   are expected to be valid by construction. This asserts properties that are going to be true
   even for user-forged pointers using unsafe API, so the only way to break these asserts is if there
   is a bug in filc itself (compiler or runtime), or by unsafely forging a pointer and then using
   that to corrupt the bits of a pointer.
   
   One example invariant: lower/upper must be aligned on the type's required alignment.
   Another example invariant: !((upper - lower) % type->size)
   
   There may be others.
   
   This does not check if the pointer is in bounds or that it's pointing at something that has any
   particular type. This isn't the actual FilC check that the compiler uses to achieve memory
   safety! */
void filc_validate_ptr(filc_ptr ptr, const filc_origin* origin);

void filc_validate_normal_ptr(filc_ptr ptr, const filc_origin* origin);
void filc_validate_return_buffer_ptr(filc_ptr ptr, const filc_origin* origin);

static inline void filc_testing_validate_ptr(filc_ptr ptr)
{
    if (PAS_ENABLE_TESTING)
        filc_validate_ptr(ptr, NULL);
}

static inline filc_object* filc_object_for_special_payload(void* payload)
{
    if (!payload)
        return NULL;
    filc_object* result = (filc_object*)((char*)payload - FILC_SPECIAL_OBJECT_SIZE);
    PAS_TESTING_ASSERT(result->lower == payload);
    PAS_TESTING_ASSERT(result->upper == (char*)payload + FILC_WORD_SIZE);
    PAS_TESTING_ASSERT(result->word_types[0] == FILC_WORD_TYPE_THREAD ||
                       result->word_types[0] == FILC_WORD_TYPE_DIRSTREAM ||
                       result->word_types[0] == FILC_WORD_TYPE_SIGNAL_HANDLER ||
                       result->word_types[0] == FILC_WORD_TYPE_PTR_TABLE ||
                       result->word_types[0] == FILC_WORD_TYPE_PTR_TABLE_ARRAY ||
                       result->word_types[0] == FILC_WORD_TYPE_JMP_BUF);
    return result;
}

static inline void* filc_object_special_payload(filc_object* object)
{
    PAS_TESTING_ASSERT(object->lower == (char*)object + FILC_SPECIAL_OBJECT_SIZE);
    return (char*)object + FILC_SPECIAL_OBJECT_SIZE;
}

static inline filc_object* filc_ptr_object(filc_ptr ptr)
{
    return (filc_object*)(uintptr_t)(ptr.word >> 64);
}

static inline void* filc_ptr_ptr(filc_ptr ptr)
{
    return (void*)(uintptr_t)ptr.word;
}

static inline bool filc_ptr_is_boxed_int(filc_ptr ptr)
{
    return !filc_ptr_object(ptr);
}

static inline void* filc_ptr_upper(filc_ptr ptr)
{
    if (filc_ptr_is_boxed_int(ptr))
        return NULL;
    return filc_ptr_object(ptr)->upper;
}

static inline void* filc_ptr_lower(filc_ptr ptr)
{
    if (filc_ptr_is_boxed_int(ptr))
        return NULL;
    return filc_ptr_object(ptr)->lower;
}

static inline uintptr_t filc_ptr_offset(filc_ptr ptr)
{
    return (char*)filc_ptr_ptr(ptr) - (char*)filc_ptr_lower(ptr);
}

static inline uintptr_t filc_ptr_available(filc_ptr ptr)
{
    return (char*)filc_ptr_upper(ptr) - (char*)filc_ptr_ptr(ptr);
}

static inline filc_ptr filc_ptr_from_word(pas_uint128 word)
{
    filc_ptr result;
    result.word = word;
    filc_testing_validate_ptr(result);
    return result;
}

static inline filc_ptr filc_ptr_create_with_ptr_and_manual_tracking(filc_object* object, void* ptr)
{
    filc_ptr result;
    result.word = ((pas_uint128)(uintptr_t)object << 64) | (pas_uint128)(uintptr_t)ptr;
    PAS_TESTING_ASSERT(filc_ptr_object(result) == object);
    PAS_TESTING_ASSERT(filc_ptr_ptr(result) == ptr);
    filc_testing_validate_ptr(result);
    return result;
}

static inline filc_ptr filc_ptr_create_with_manual_tracking(filc_object* object)
{
    return filc_ptr_create_with_ptr_and_manual_tracking(object, object->lower);
}

static inline filc_ptr filc_ptr_create(filc_thread* my_thread, filc_object* object)
{
    filc_thread_track_object(my_thread, object);
    return filc_ptr_create_with_manual_tracking(object);
}

static inline filc_ptr filc_ptr_for_return_buffer(filc_return_buffer* return_buffer,
                                                  filc_word_type word_type)
{
    PAS_ASSERT(word_type == FILC_WORD_TYPE_INT || word_type == FILC_WORD_TYPE_PTR);
    return_buffer->lower = &return_buffer->data;
    return_buffer->upper = &return_buffer->data + 1;
    return_buffer->flags = FILC_OBJECT_FLAG_RETURN_BUFFER;
    return_buffer->word_type = word_type;
    return_buffer->data = 0;
    return filc_ptr_create_with_manual_tracking((filc_object*)return_buffer);
}

static inline filc_ptr filc_ptr_for_int_return_buffer(filc_return_buffer* return_buffer)
{
    return filc_ptr_for_return_buffer(return_buffer, FILC_WORD_TYPE_INT);
}

static inline filc_ptr filc_ptr_for_ptr_return_buffer(filc_return_buffer* return_buffer)
{
    return filc_ptr_for_return_buffer(return_buffer, FILC_WORD_TYPE_PTR);
}

static inline filc_ptr filc_ptr_forge_null(void)
{
    filc_ptr result;
    result.word = 0;
    PAS_TESTING_ASSERT(filc_ptr_is_boxed_int(result));
    PAS_TESTING_ASSERT(!filc_ptr_ptr(result));
    filc_testing_validate_ptr(result);
    return result;
}

/* Creates a boxed int ptr, which cannot be accessed at all. */
static inline filc_ptr filc_ptr_forge_invalid(void* ptr)
{
    filc_ptr result;
    result.word = (pas_uint128)(uintptr_t)ptr;
    PAS_TESTING_ASSERT(filc_ptr_is_boxed_int(result));
    PAS_TESTING_ASSERT(filc_ptr_ptr(result) == ptr);
    filc_testing_validate_ptr(result);
    return result;
}

static inline filc_ptr filc_ptr_with_ptr(filc_ptr ptr, void* new_ptr)
{
    filc_ptr result;
    result = ptr;
    result.word &= ~(pas_uint128)UINTPTR_MAX;
    result.word |= (pas_uint128)(uintptr_t)new_ptr;
    PAS_TESTING_ASSERT(filc_ptr_object(result) == filc_ptr_object(ptr));
    PAS_TESTING_ASSERT(filc_ptr_ptr(result) == new_ptr);
    filc_testing_validate_ptr(result);
    return result;
}

static inline filc_ptr filc_ptr_with_offset(filc_ptr ptr, uintptr_t offset)
{
    return filc_ptr_with_ptr(ptr, (char*)filc_ptr_ptr(ptr) + offset);
}

static inline filc_ptr filc_ptr_for_special_payload_with_manual_tracking(void* payload)
{
    return filc_ptr_create_with_manual_tracking(filc_object_for_special_payload(payload));
}

static inline filc_ptr filc_ptr_for_special_payload(filc_thread* my_thread, void* payload)
{
    return filc_ptr_create(my_thread, filc_object_for_special_payload(payload));
}

static inline bool filc_ptr_is_totally_equal(filc_ptr a, filc_ptr b)
{
    return a.word == b.word;
}

static inline bool filc_ptr_is_totally_null(filc_ptr ptr)
{
    return filc_ptr_is_totally_equal(ptr, filc_ptr_forge_null());
}

static inline filc_ptr filc_ptr_load_with_manual_tracking(filc_ptr* ptr)
{
    filc_ptr result;
    result.word = __c11_atomic_load((_Atomic pas_uint128*)&ptr->word, __ATOMIC_RELAXED);
    return result;
}

static inline filc_ptr filc_ptr_load(filc_thread* my_thread, filc_ptr* ptr)
{
    filc_ptr result = filc_ptr_load_with_manual_tracking(ptr);
    filc_thread_track_object(my_thread, filc_ptr_object(result));
    return result;
}

PAS_API void filc_store_barrier_slow(filc_thread* my_thread, filc_object* target);

static inline void filc_store_barrier(filc_thread* my_thread, filc_object* target)
{
    if (PAS_UNLIKELY(filc_is_marking) && target)
        filc_store_barrier_slow(my_thread, target);
}

PAS_API void filc_store_barrier_outline(filc_thread* my_thread, filc_object* target);

static inline void filc_ptr_store_without_barrier(filc_ptr* ptr, filc_ptr value)
{
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->word, value.word, __ATOMIC_RELAXED);
}

static inline void filc_ptr_store(filc_thread* my_thread, filc_ptr* ptr, filc_ptr value)
{
    filc_store_barrier(my_thread, filc_ptr_object(value));
    filc_ptr_store_without_barrier(ptr, value);
}

static inline bool filc_ptr_unfenced_unbarriered_weak_cas(
    filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->word, &expected.word, new_value.word,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

static inline bool filc_ptr_unfenced_weak_cas(
    filc_thread* my_thread, filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    return filc_ptr_unfenced_unbarriered_weak_cas(ptr, expected, new_value);
}

static inline filc_ptr filc_ptr_unfenced_strong_cas(
    filc_thread* my_thread, filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->word, &expected.word, new_value.word,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    return filc_ptr_from_word(expected.word);
}

static inline bool filc_ptr_weak_cas(
    filc_thread* my_thread, filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->word, &expected.word, new_value.word,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline filc_ptr filc_ptr_strong_cas(
    filc_thread* my_thread, filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->word, &expected.word, new_value.word,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return filc_ptr_from_word(expected.word);
}

static inline filc_ptr filc_ptr_unfenced_xchg(filc_thread* my_thread, filc_ptr* ptr, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    return filc_ptr_from_word(
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->word, new_value.word, __ATOMIC_RELAXED));
}

static inline filc_ptr filc_ptr_xchg(filc_thread* my_thread, filc_ptr* ptr, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    return filc_ptr_from_word(
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->word, new_value.word, __ATOMIC_SEQ_CST));
}

static inline void filc_ptr_store_fenced(filc_thread* my_thread, filc_ptr* ptr, filc_ptr new_value)
{
    filc_store_barrier(my_thread, filc_ptr_object(new_value));
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->word, new_value.word, __ATOMIC_SEQ_CST);
}

static inline filc_ptr filc_ptr_load_fenced_with_manual_tracking(filc_ptr* ptr)
{
    filc_ptr result;
    result.word = __c11_atomic_load((_Atomic pas_uint128*)&ptr->word, __ATOMIC_SEQ_CST);
    return result;
}

PAS_API void filc_object_flags_dump_with_comma(filc_object_flags flags, bool* comma, pas_stream* stream);
PAS_API void filc_object_flags_dump(filc_object_flags flags, pas_stream* stream);
PAS_API void filc_object_dump_for_ptr(filc_object* object, void* ptr, pas_stream* stream);
PAS_API void filc_object_dump(filc_object* object, pas_stream* stream);
PAS_API void filc_ptr_dump(filc_ptr ptr, pas_stream* stream);
PAS_API char* filc_object_to_new_string(filc_object* object);
PAS_API char* filc_ptr_to_new_string(filc_ptr ptr); /* WARNING: this is different from
                                                       zptr_to_new_string. This uses the
                                                       bmalloc heap - WHICH MUST NEVER LEAK TO
                                                       PIZLONATED CODE - whereas the other one
                                                       uses the GC heap. */

static inline size_t filc_object_size(filc_object* object)
{
    PAS_TESTING_ASSERT(object->upper >= object->lower);
    size_t result = (char*)object->upper - (char*)object->lower;
    PAS_TESTING_ASSERT(pas_is_aligned(result, FILC_WORD_SIZE));
    return result;
}

static inline size_t filc_object_num_words_for_size(size_t size)
{
    PAS_ASSERT(pas_is_aligned(size, FILC_WORD_SIZE));
    return size / FILC_WORD_SIZE;
}

static inline size_t filc_object_num_words(filc_object* object)
{
    return filc_object_size(object) / FILC_WORD_SIZE;
}

static inline size_t filc_object_word_type_index_for_ptr(filc_object* object, void* ptr)
{
    PAS_TESTING_ASSERT(object);
    PAS_TESTING_ASSERT(ptr >= object->lower);
    PAS_TESTING_ASSERT(ptr < object->upper);
    return ((char*)ptr - (char*)object->lower) / FILC_WORD_SIZE;
}

static inline filc_word_type filc_object_get_word_type(filc_object* object,
                                                       size_t word_type_index)
{
    PAS_TESTING_ASSERT(word_type_index < filc_object_num_words(object));
    return object->word_types[word_type_index];
}

PAS_API void filc_word_type_dump(filc_word_type type, pas_stream* stream);
PAS_API char* filc_word_type_to_new_string(filc_word_type type);

static inline bool filc_word_type_is_special(filc_word_type word_type)
{
    switch (word_type) {
    case FILC_WORD_TYPE_FUNCTION:
    case FILC_WORD_TYPE_THREAD:
    case FILC_WORD_TYPE_SIGNAL_HANDLER:
    case FILC_WORD_TYPE_DIRSTREAM:
    case FILC_WORD_TYPE_PTR_TABLE:
    case FILC_WORD_TYPE_PTR_TABLE_ARRAY:
    case FILC_WORD_TYPE_DL_HANDLE:
    case FILC_WORD_TYPE_JMP_BUF:
        return true;
    default:
        return false;
    }
}

static inline bool filc_special_word_type_has_destructor(filc_word_type word_type)
{
    switch (word_type) {
    case FILC_WORD_TYPE_THREAD:
    case FILC_WORD_TYPE_PTR_TABLE:
        return true;
    case FILC_WORD_TYPE_FUNCTION:
    case FILC_WORD_TYPE_SIGNAL_HANDLER:
    case FILC_WORD_TYPE_DIRSTREAM:
    case FILC_WORD_TYPE_PTR_TABLE_ARRAY:
    case FILC_WORD_TYPE_DL_HANDLE:
    case FILC_WORD_TYPE_JMP_BUF:
        return false;
    default:
        PAS_ASSERT(!"Not a special word type");
        return false;
    }
}

/* This is basically va_arg, but it doesn't check that the type matches or that it's even in bounds.
   It's up to you to check the returned ptr. Also, it's assumed that filc_ptr* ptr points at a local
   pointer that you own. */
static inline filc_ptr filc_ptr_get_next_bytes(filc_ptr* ptr, size_t size, size_t alignment)
{
    filc_ptr ptr_value;
    uintptr_t ptr_as_int;
    filc_ptr result;

    ptr_value = *ptr;
    ptr_as_int = (uintptr_t)filc_ptr_ptr(ptr_value);
    ptr_as_int = pas_round_up_to_power_of_2(ptr_as_int, alignment);

    result = filc_ptr_with_ptr(ptr_value, (void*)ptr_as_int);

    *ptr = filc_ptr_with_ptr(ptr_value, (char*)ptr_as_int + size);

    return result;
}

PAS_API filc_ptr filc_get_next_bytes_for_va_arg(
    filc_thread* my_thread, filc_ptr ptr_ptr, size_t size, size_t alignment, const filc_origin* origin);

/* Allocates a "special" object; this is used for functions, threads, and dirstreams. For these types,
   the object's lower/upper pretends to have just one word but the payload size could be anything. The
   one word is set to word_type. */
filc_object* filc_allocate_special(filc_thread* my_thread, size_t size, filc_word_type word_type);

/* Same as filc_allocate_special, but usable before we have ever created threads and before we have
   any thread context. */
filc_object* filc_allocate_special_early(size_t size, filc_word_type word_type);

filc_object* filc_allocate_with_existing_data(
    filc_thread* my_thread, void* data, size_t size, filc_object_flags object_flags,
    filc_word_type initial_word_type);

filc_object* filc_allocate_special_with_existing_payload(
    filc_thread* my_thread, void* payload, filc_word_type word_type);

/* Allocates an object with a payload of the given size and brings all words into the unset state. The
   object's lower/upper are set accordingly. */
filc_object* filc_allocate(filc_thread* my_thread, size_t size);

/* Allocates an object with a payload of the given size and alignment. The object itself may or may not
   have that alignment. Word types start out unset and the object's lower/upper are set accordingly. */
filc_object* filc_allocate_with_alignment(filc_thread* my_thread, size_t size, size_t alignment);

/* Allocates an object and initializes it to be an int object. */
filc_object* filc_allocate_int(filc_thread* my_thread, size_t size);

/* Reallocates the given object. The old object is freed and the new object contains a copy of the old
   one up to the new_size. Any fresh memory not copied from the old object starts out with unset state. */
filc_object* filc_reallocate(filc_thread* my_thread, filc_object* object, size_t new_size);
filc_object* filc_reallocate_with_alignment(filc_thread* my_thread, filc_object* object,
                                            size_t new_size, size_t alignment);

/* "Frees" the object. This transitions the state of all words to the free state. */
void filc_free(filc_thread* my_thread, filc_object* object);

/* Frees the object without checking that it's an object that can be freed. Used for freeing special
   objects that we know we can free, like dirstreams. Only call this after checking that it's
   something that is OK to free. This still does the is-not-already-free check. */
void filc_free_yolo(filc_thread* my_thread, filc_object* object);

filc_ptr_table* filc_ptr_table_create(filc_thread* my_thread);
void filc_ptr_table_destruct(filc_ptr_table* ptr_table);
uintptr_t filc_ptr_table_encode(filc_thread* my_thread, filc_ptr_table* ptr_table, filc_ptr ptr);
filc_ptr filc_ptr_table_decode_with_manual_tracking(
    filc_ptr_table* ptr_table, uintptr_t encoded_ptr);
void filc_ptr_table_mark_outgoing_ptrs(filc_ptr_table* ptr_table, filc_object_array* stack);

filc_ptr_table_array* filc_ptr_table_array_create(filc_thread* my_thread, size_t capacity);
void filc_ptr_table_array_mark_outgoing_ptrs(filc_ptr_table_array* array, filc_object_array* stack);

/* munmap() can free memory while we're exited. If we use memory while exited, and it might be
   mmap memory, then we must pin it first. This will cause munmap() to fail.

   For convenience, these functions forgive you for passing a NULL object. */
void filc_pin(filc_object* object);
void filc_unpin(filc_object* object);

void filc_check_access_int(filc_ptr ptr, uintptr_t bytes, filc_access_kind kind,
                           const filc_origin* origin);
void filc_check_access_ptr(filc_ptr ptr, filc_access_kind kind, const filc_origin* origin);

void filc_check_read_int(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin);
void filc_check_write_int(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin);
void filc_check_read_ptr(filc_ptr ptr, const filc_origin* origin);
void filc_check_write_ptr(filc_ptr ptr, const filc_origin* origin);

#define FILC_CHECK_INT_FIELD(ptr, struct_type, field_name, access_kind) do { \
        struct_type check_temp; \
        filc_check_access_int(filc_ptr_with_offset((ptr), PAS_OFFSETOF(struct_type, field_name)), \
                              sizeof(check_temp.field_name), (access_kind), NULL); \
    } while (false)

#define FILC_CHECK_PTR_FIELD(ptr, struct_type, field_name, access_kind) do { \
        filc_check_access_ptr(filc_ptr_with_offset((ptr), PAS_OFFSETOF(struct_type, field_name)), \
                              (access_kind), NULL); \
    } while (false)

void filc_check_function_call(filc_ptr ptr);

void filc_check_access_special(
    filc_ptr ptr, filc_word_type expected_type, const filc_origin* origin);

void filc_memset_with_exit(filc_thread* my_thread, filc_object* object, void* ptr, unsigned value, size_t bytes);
void filc_memcpy_with_exit(
    filc_thread* my_thread, filc_object* dst_object, filc_object* src_object,
    void* dst, const void* src, size_t bytes);
void filc_memmove_with_exit(
    filc_thread* my_thread, filc_object* dst_object, filc_object* src_object,
    void* dst, const void* src, size_t bytes);

/* You can call these if you know that you're copying pointers (or possible pointers) and you've
   already proved that it's safe and the pointer/size are aligned.
   
   These also happen to be used as the implementation of filc_memset_impl/filc_memcpy_impl/
   filc_memmove_impl in those cases where pointer copying is detected. Those also do all the
   checks needed to ensure memory safety. So, usually, you want those, not these.

   The number of bytes must be a multiple of 16. */
void filc_low_level_ptr_safe_bzero(void* ptr, size_t bytes);

void filc_low_level_ptr_safe_bzero_with_exit(
    filc_thread* my_thread, filc_object* object, void* ptr, size_t bytes);

void filc_memset(filc_thread* my_thread, filc_ptr ptr, unsigned value, size_t count,
                 const filc_origin* origin);

/* We don't have a separate memcpy right now. We could, in the future. But likely, the cost
   difference between the two is much smaller than the cost overhead of checking, so it might
   never be worth it. */
void filc_memmove(filc_thread* my_thread, filc_ptr dst, filc_ptr src, size_t count,
                  const filc_origin* origin);

/* Checks that the ptr points at a valid C string. That is, there is a null terminator before we
   get to the upper bound. Returns a copy of that string allocated in the utility heap, and checks
   that it still has the null terminator at the end. Kills the shit out of the program if any of the
   checks fail.
   
   The fact that the string is allocated in the utility heap - and the fact that the utility heap
   has no capabilities into it other than immortal and/or opaque ones - and the fact that there is
   no way for the user to cause us to free an object of their choice in the utility heap - means
   that the string returned by this can't change under you.
   
   It's safe to call legacy C string functions on strings returned from this, since if they lacked
   an in-bounds terminator, then this would have trapped.

   It's necessary to free the string when you're done with it. */
char* filc_check_and_get_new_str(filc_ptr ptr);

/* Assumes that the given memory has already been checked to be int and that it has the given size,
   but otherwise does all of the same checks as filc_check_and_get_new_str(). */
char* filc_check_and_get_new_str_for_int_memory(char* base, size_t size);

char* filc_check_and_get_new_str_or_null(filc_ptr ptr);

/* Safely create a Fil-C string from a legacy C string. */
filc_ptr filc_strdup(filc_thread* my_thread, const char* str);

/* NOTE: It's tempting to add a macro that takes a type and does get_next, but I don't see how
   that would handle pointers correctly. */

static inline filc_ptr filc_ptr_get_next_ptr_with_manual_tracking(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(filc_ptr), alignof(filc_ptr));
    filc_check_read_ptr(slot_ptr, NULL);
    return filc_ptr_load_with_manual_tracking((filc_ptr*)filc_ptr_ptr(slot_ptr));
}

static inline filc_ptr filc_ptr_get_next_ptr(filc_thread* my_thread, filc_ptr* ptr)
{
    filc_ptr result = filc_ptr_get_next_ptr_with_manual_tracking(ptr);
    filc_thread_track_object(my_thread, filc_ptr_object(result));
    return result;
}

static inline int filc_ptr_get_next_int(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(int), alignof(int));
    filc_check_read_int(slot_ptr, sizeof(int), NULL);
    return *(int*)filc_ptr_ptr(slot_ptr);
}

static inline unsigned filc_ptr_get_next_unsigned(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(unsigned), alignof(unsigned));
    filc_check_read_int(slot_ptr, sizeof(unsigned), NULL);
    return *(unsigned*)filc_ptr_ptr(slot_ptr);
}

static inline long filc_ptr_get_next_long(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(long), alignof(long));
    filc_check_read_int(slot_ptr, sizeof(long), NULL);
    return *(long*)filc_ptr_ptr(slot_ptr);
}

static inline unsigned long filc_ptr_get_next_unsigned_long(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(unsigned long), alignof(unsigned long));
    filc_check_read_int(slot_ptr, sizeof(unsigned long), NULL);
    return *(unsigned long*)filc_ptr_ptr(slot_ptr);
}

static inline size_t filc_ptr_get_next_size_t(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(size_t), alignof(size_t));
    filc_check_read_int(slot_ptr, sizeof(size_t), NULL);
    return *(size_t*)filc_ptr_ptr(slot_ptr);
}

static inline double filc_ptr_get_next_double(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(double), alignof(double));
    filc_check_read_int(slot_ptr, sizeof(double), NULL);
    return *(double*)filc_ptr_ptr(slot_ptr);
}

static inline bool filc_ptr_get_next_bool(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(bool), alignof(bool));
    filc_check_read_int(slot_ptr, sizeof(bool), NULL);
    return *(bool*)filc_ptr_ptr(slot_ptr);
}

static inline ssize_t filc_ptr_get_next_ssize_t(filc_ptr* ptr)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(ssize_t), alignof(ssize_t));
    filc_check_read_int(slot_ptr, sizeof(ssize_t), NULL);
    return *(ssize_t*)filc_ptr_ptr(slot_ptr);
}

static inline filc_exception_and_int filc_exception_and_int_with_int(int value)
{
    filc_exception_and_int result;
    result.has_exception = false;
    result.value = value;
    return result;
}

static inline filc_exception_and_int filc_exception_and_int_with_exception(void)
{
    filc_exception_and_int result;
    result.has_exception = true;
    result.value = 0;
    return result;
}

/* If parent is not NULL, increases its ref count and returns it. Otherwise, creates a new context. */
filc_global_initialization_context* filc_global_initialization_context_create(
    filc_global_initialization_context* parent);
/* Attempts to add the given global to be initialized.
   
   If it's already in the set then this returns false.
   
   If it's not in the set, then it's added to the set and true is returned. */
bool filc_global_initialization_context_add(
    filc_global_initialization_context* context, filc_ptr* pizlonated_gptr, filc_object* object);
/* Derefs the context. If the refcount reaches zero, it gets destroyed.
 
   Destroying the set means storing all known ptr_capabilities into their corresponding pizlonated_gptrs
   atomically. */
void filc_global_initialization_context_destroy(filc_global_initialization_context* context);

void filc_execute_constant_relocations(
    void* constant, filc_constant_relocation* relocations, size_t num_relocations,
    filc_global_initialization_context* context);

void filc_defer_or_run_global_ctor(bool (*global_ctor)(PIZLONATED_SIGNATURE));
void filc_run_deferred_global_ctors(filc_thread* my_thread); /* Important safety property: libc must
                                                                call this before letting the user
                                                                start threads. But it's OK if the
                                                                deferred constructors that this calls
                                                                start threads, as far as safety
                                                                goes. */
void filc_run_global_dtor(bool (*global_dtor)(PIZLONATED_SIGNATURE));
void filc_error(const char* reason, const filc_origin* origin);

/* This works out whether we're supposed to catch the exception or keep going by calling the personality
   function.
   
   Returns true if we're supposed to catch the exception. In that case, my_thread->unwind_registers
   contain the values that are supposed to be consumed by the landingpad.
   
   Returns false if we're supposed to let the exception propagate. */
bool filc_landing_pad(filc_thread* my_thread);

/* This is basically _Unwind_Resume, except that since the compiler is calling it, it's easier to make
   it a native C function. Also, this is just an assertion to ensure that the unwinder didn't screw
   up. Because of the way that the unwinder is driven by personality functions, this is a mandatory
   safety assertion. For example, the personality function could request that we unwind to a destructor,
   which will appear safe to the unwinder (since that will mean unwinding to a frame that supports
   unwinding) but then the landing pad ends in a resume, even though the frame after the resume doesn't
   support unwinding.

   Unlike with legacy C unwinding, this function is also called whenever we do an early return as a
   result of a CallInst returning exceptionally. This is necessary since this only checks that our
   caller will be able to handle exceptional returns, not that our caller's caller also will be able
   to. */
void filc_resume_unwind(filc_thread* my_thread, filc_origin* origin);

static inline const char* filc_jmp_buf_kind_get_string(filc_jmp_buf_kind kind)
{
    switch (kind) {
    case filc_jmp_buf_setjmp:
        return "setjmp";
    case filc_jmp_buf__setjmp:
        return "_setjmp";
    case filc_jmp_buf_sigsetjmp:
        return "sigsetjmp";
    }
    PAS_ASSERT(!"Should not be reached");
    return NULL;
}

static inline const char* filc_jmp_buf_kind_get_longjmp_string(filc_jmp_buf_kind kind)
{
    switch (kind) {
    case filc_jmp_buf_setjmp:
        return "longjmp";
    case filc_jmp_buf__setjmp:
        return "_longjmp";
    case filc_jmp_buf_sigsetjmp:
        return "siglongjmp";
    }
    PAS_ASSERT(!"Should not be reached");
    return NULL;
}

/* This creates all of the filc_jmp_buf except for the system_buf, which must be populated by the
   caller. */
filc_jmp_buf* filc_jmp_buf_create(filc_thread* my_thread, filc_jmp_buf_kind kind);
void filc_jmp_buf_mark_outgoing_ptrs(filc_jmp_buf* jmp_buf, filc_object_array* stack);

PAS_API void filc_system_mutex_lock(filc_thread* my_thread, pas_system_mutex* lock);

PAS_API bool filc_get_bool_env(const char* name, bool default_value);
PAS_API unsigned filc_get_unsigned_env(const char* name, unsigned default_value);
PAS_API size_t filc_get_size_env(const char* name, size_t default_value);

PAS_END_EXTERN_C;

#endif /* FILC_RUNTIME_H */

