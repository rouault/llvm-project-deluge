#ifndef FILC_RUNTIME_H
#define FILC_RUNTIME_H

#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pas_allocation_config.h"
#include "pas_hashtable.h"
#include "pas_heap_ref.h"
#include "pas_lock.h"
#include "pas_lock_free_read_ptr_ptr_hashtable.h"
#include "pas_ptr_hash_set.h"
#include "pas_range.h"

PAS_BEGIN_EXTERN_C;

/* Internal FilC runtime header, defining how the FilC runtime maintains its state.
 
   Currently, including this header is the only way to perform FFI to FilC code, and the API for
   that is too low-level for comfort. That's probably fine, since the FilC ABI is going to
   change the moment I start giving a shit about performance.

   This runtime is engineered under the following principles:

   - It's based on libpas, so we get fully correct isoheap semantics and the perf of allocation in
     those heaps is as good as what I can come up with after ~5 years of effort. The isoheap code
     has been thoroughly battle-tested and we can trust it.

   - Coding standards have to be extremely high, and assert usage should be full-ass
     belt-and-suspenders. The goal of this code is to achieve memory safety under the FilC
     "Bounded P^I" type system. It's fine to take extra cycles or bytes to achieve that goal.

   - There are no optimizations yet, but the structure of this code is such that when I choose to 
     go into optimization mode, I will be able to wreak havoc I'm just holding back from going
     there, for now. Lots of running code is better than a small amount of fast code. */

struct filc_alloca_stack;
struct filc_constant_relocation;
struct filc_constexpr_node;
struct filc_global_initialization_context;
struct filc_initialization_entry;
struct filc_origin;
struct filc_ptr;
struct filc_type;
struct filc_type_template;
struct pas_basic_heap_runtime_config;
struct pas_stream;
typedef struct filc_alloca_stack filc_alloca_stack;
typedef struct filc_constant_relocation filc_constant_relocation;
typedef struct filc_constexpr_node filc_constexpr_node;
typedef struct filc_global_initialization_context filc_global_initialization_context;
typedef struct filc_initialization_entry filc_initialization_entry;
typedef struct filc_origin filc_origin;
typedef struct filc_ptr filc_ptr;
typedef struct filc_type filc_type;
typedef struct filc_type_template filc_type_template;
typedef struct pas_basic_heap_runtime_config pas_basic_heap_runtime_config;
typedef struct pas_stream pas_stream;

typedef uint8_t filc_word_type;

/* Note that any statement like "128-bit word" below needs to be understood with the caveat that ptr
   access also checks bounds. So, a pointer might say it has type "128-bit int" but bounds that say
   "1 byte", in which case you get the intersection: "8-bit int". */
#define FILC_WORD_TYPE_OFF_LIMITS        ((uint8_t)0)     /* 128-bit that cannot be accessed. */
#define FILC_WORD_TYPE_INT               ((uint8_t)1)     /* 128-bit word that contains ints.
                                                               Primitive ptrs may have bounds that
                                                               are less than 16 bytes and may not have
                                                               16 byte alignment. */
#define FILC_WORD_TYPE_PTR_SIDECAR       ((uint8_t)2)     /* 128-bit word that contains the sidecar
                                                               of a wide ptr. */
#define FILC_WORD_TYPE_PTR_CAPABILITY    ((uint8_t)3)     /* 128-bit word that contains the capability
                                                               of a wide ptr. */

#define FILC_WORD_SIZE sizeof(pas_uint128)

/* Helper for emitting the ptr word types */
#define FILC_WORD_TYPES_PTR \
    FILC_WORD_TYPE_PTR_SIDECAR, \
    FILC_WORD_TYPE_PTR_CAPABILITY

/* The filc pointer, represented using its heap format. Let's call this the rest format. Pointers must
   have this format when they are in any memory location of pointer type (i.e. the pair of SIDECAR and
   CAPABILITY word types).
   
   It would be smart to someday optimize both the compiler and the runtime for the filc pointer's flight
   format: ptr, lower, upper, and type. That would be an awesome optimization, but I haven't done it yet,
   because I am not yet interested in FilC's performance so long as I don't have a lot of running code.
   
   Instead, we pass around filc_ptrs, but to do anything with them, we have to call getters to get the
   ptr/lower/upper/type. That's the current idiom even though it is surely inefficient and confusing for
   debugging, since we don't clear invalid sidecars - we just lazily ignore them in the lower getter. */
struct PAS_ALIGNED(FILC_WORD_SIZE) filc_ptr {
    /* The sidecar is optional. If it doesn't match the capability, we ignore it. In some cases, we just
       store 0 in it. If the ptr = lower, then the sidecar is totally ignored. */
    pas_uint128 sidecar;

    /* The capabiltiy is mandatory. It stores the ptr itself, the upper bound, the type, and some data
       that's useful for inferring more accurate bounds. */
    pas_uint128 capability;
};

enum filc_capability_kind {
    /* The ptr is exactly at lower, so ignore the sidecar. This avoids capability widening across
       allocation boundaries. */
    filc_capability_at_lower,

    /* The ptr is inside an array allocation. If it's in the flex array allocation, the type will
       be the trailing array type. */
    filc_capability_in_array,

    /* The ptr is inside the base of a flex. The capability's upper refers to the upper bounds of
       the flex base. This has a special sidecar (the flex_upper sidecar). */
    filc_capability_flex_base,

    /* The ptr is out of bounds. The capability does not have an upper or type. This has a special
       sidecar (the oob_capability sidecar). */
    filc_capability_oob,
};

typedef enum filc_capability_kind filc_capability_kind;

enum filc_sidecar_kind {
    /* Normal sidecar, which tells the lower bounds. */
    filc_sidecar_lower,

    /* Sidecar used for flex_base capabilities, which tells the true upper bounds of the flex
       allocation. */
    filc_sidecar_flex_upper,

    /* Sidecar used for oob capabilities, which contains the full capability but not the pointer.
       I.e. the sidecar has lower, upper, and type. In this case, the sidecar's lower/upper/type
       are combined with the capability's ptr. If that fails, then the ptr becomes a boxed int. */
    filc_sidecar_oob_capability
};

typedef enum filc_sidecar_kind filc_sidecar_kind;

/* Zero-word types are unique; they are only equal by pointer equality. They may or may not have
   a size. If they have a size, they must have an alignment, and they may be allocated. Zero-word
   types have a pas_heap_runtime_config* instead of a trailing_array.
   
   Non-zero-word types are equal if they are structurally the same. They must have a size that
   matches num_words, as in: (size + 15) / 16 == num_words. They must have an alignment. And, they
   have a trailing_array instead of a runtime_config. */
struct filc_type {
    unsigned index;
    size_t size;
    size_t alignment;
    size_t num_words;
    union {
        const filc_type* trailing_array;
        pas_basic_heap_runtime_config* runtime_config;
    } u;
    filc_word_type word_types[];
};

/* Used to describe a non-opaque type to the runtime. For this to be useful, you need to pass it to
   filc_get_type().

   There is no need for the layout of this to match filc_type. It cannot completely match it,
   since type has extra fields that this doesn't need to have. */
struct filc_type_template {
    size_t size;
    size_t alignment;
    const filc_type_template* trailing_array;
    filc_word_type word_types[];
};

struct filc_origin {
    const char* function;
    const char* filename;
    unsigned line;
    unsigned column;
};

struct filc_initialization_entry {
    pas_uint128* pizlonated_gptr;
    pas_uint128 ptr_capability;
};

struct filc_global_initialization_context {
    size_t ref_count;
    pas_ptr_hash_set seen;
    filc_initialization_entry* entries;
    size_t num_entries;
    size_t entries_capacity;
};

enum filc_constant_kind {
    /* The target is the actual function. */
    filc_function_constant,

    /* The target is a getter that returns a pointer to the global. */
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

struct filc_alloca_stack {
    void** array;
    size_t size;
    size_t capacity;
};

#define FILC_ALLOCA_STACK_INITIALIZER { \
        .array = NULL, \
        .size = 0, \
        .capacity = 0 \
    }

/* This exist only so that compiler-generated templates that need the int type for trailing arrays can
   refer to it. */
extern const filc_type_template filc_int_type_template;

extern const filc_type filc_int_type;
extern const filc_type filc_one_ptr_type;
extern const filc_type filc_int_ptr_type;
extern const filc_type filc_function_type;
extern const filc_type filc_type_type;

extern const filc_type** filc_type_array;
extern unsigned filc_type_array_size;
extern unsigned filc_type_array_capacity;

#define FILC_INVALID_TYPE_INDEX              0u
#define FILC_INT_TYPE_INDEX                  1u
#define FILC_ONE_PTR_TYPE_INDEX              2u
#define FILC_INT_PTR_TYPE_INDEX              3u
#define FILC_FUNCTION_TYPE_INDEX             4u
#define FILC_TYPE_TYPE_INDEX                 5u
#define FILC_MUSL_PASSWD_TYPE_INDEX          6u
#define FILC_MUSL_SIGACTION_TYPE_INDEX       7u
#define FILC_THREAD_TYPE_INDEX               8u
#define FILC_MUSL_ADDRINFO_TYPE_INDEX        9u
#define FILC_DIRSTREAM_TYPE_INDEX            10u
#define FILC_TYPE_ARRAY_INITIAL_SIZE         11u
#define FILC_TYPE_ARRAY_INITIAL_CAPACITY     100u
#define FILC_TYPE_MAX_INDEX                  0x3fffffffu
#define FILC_TYPE_INDEX_MASK                 0x3fffffffu
#define FILC_TYPE_ARRAY_MAX_SIZE             0x40000000u

extern pas_lock_free_read_ptr_ptr_hashtable filc_fast_type_table;
extern pas_lock_free_read_ptr_ptr_hashtable filc_fast_heap_table;
extern pas_lock_free_read_ptr_ptr_hashtable filc_fast_hard_heap_table;

PAS_DECLARE_LOCK(filc_type);
PAS_DECLARE_LOCK(filc_type_ops);
PAS_DECLARE_LOCK(filc_global_initialization);

void filc_panic(const filc_origin* origin, const char* format, ...);

#define FILC_CHECK(exp, origin, ...) do {     \
        if ((exp)) \
            break; \
        filc_panic(origin, __VA_ARGS__); \
    } while (0)

/* Ideally, all FILC_ASSERTs would be turned into FILC_CHECKs.
 
   Also, some FILC_ASSERTs are asserting things that cannot happen unless the filc runtime or
   compiler are broken or memory safey was violated some other way; it would be great to better
   distinguish those. Most of them aren't FILC_TESTING_ASSERTs. */
#define FILC_ASSERT(exp, origin) do { \
        if ((exp)) \
            break; \
        filc_panic( \
            origin, \
            "%s:%d: %s: safety assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

#define FILC_TESTING_ASSERT(exp, origin) do { \
        if (!PAS_ENABLE_TESTING) \
            break; \
        if ((exp)) \
            break; \
        filc_panic( \
            origin, "%s:%d: %s: testing assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

#define PIZLONATED_SIGNATURE \
    void* pizlonated_arg_ptr, void* pizlonated_arg_upper, const filc_type* pizlonated_arg_type, \
    void* pizlonated_ret_ptr, void* pizlonated_ret_upper, const filc_type *pizlonated_ret_type

#define PIZLONATED_ARGS \
    filc_ptr_forge(pizlonated_arg_ptr, pizlonated_arg_ptr, pizlonated_arg_upper, pizlonated_arg_type)
#define PIZLONATED_RETS \
    filc_ptr_forge(pizlonated_ret_ptr, pizlonated_ret_ptr, pizlonated_ret_upper, pizlonated_ret_type)

#define PIZLONATED_DELETE_ARGS() do { \
        filc_deallocate(pizlonated_arg_ptr); \
        PAS_UNUSED_PARAM(pizlonated_arg_upper); \
        PAS_UNUSED_PARAM(pizlonated_arg_type); \
        PAS_UNUSED_PARAM(pizlonated_ret_ptr); \
        PAS_UNUSED_PARAM(pizlonated_ret_upper); \
        PAS_UNUSED_PARAM(pizlonated_ret_type); \
    } while (false)

/* Must be called from CRT before any FilC happens. If we ever allow FilC dylibs to be loaded 
   into non-FilC code, then we'll have to call it from compiler-generated initializers, too. It's
   fine to call this more than once. */
PAS_API void filc_initialize(void);

/* Begin execution in Fil-C. Executing Fil-C comes with the promise that you'll periodically do
   a pollcheck and that all signals will be deferred to pollchecks.
   
   In the future, this might mean acquiring heap access (if Fil-C ever gets a GC). */
PAS_API void filc_enter(void);

/* End execution in Fil-C. Call this before doing anything that might block or anything to
   affect signal masks.
   
   You can exit and then reenter as much as you like. It'll be super cheap eventually. */
PAS_API void filc_exit(void);

/* Check if there's a pending signal, and if so, run its handler.
   
   This mechanism allows us to have signal handlers that allocate even though the allocator uses
   locks. It also means that signal handlers can call into almost all stdfil API and all
   compiler-facing runtime API.

   Only call this inside Fil-C execution and never after exiting. */
PAS_API void filc_pollcheck(void);

void filc_origin_dump(const filc_origin* origin, pas_stream* stream);

static inline const filc_type* filc_type_lookup(unsigned index)
{
    PAS_TESTING_ASSERT(index < filc_type_array_size);
    return filc_type_array[index];
}

static inline size_t filc_type_template_num_words(const filc_type_template* type)
{
    return pas_round_up_to_power_of_2(type->size, FILC_WORD_SIZE) / FILC_WORD_SIZE;
}

PAS_API void filc_type_template_dump(const filc_type_template* type, pas_stream* stream);

/* Hash-conses the type template to give you a type instance. If that exact type_template ptr has been
   used for filc_get_type before, then this is lock-free. Otherwise, it's still O(1) but may need
   some locks. You're expected to never free the type_template. */
PAS_API const filc_type* filc_get_type(const filc_type_template* type_template);

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
void filc_validate_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                              const filc_origin* origin);

static inline void filc_validate_ptr(filc_ptr ptr, const filc_origin* origin)
{
    filc_validate_ptr_impl(ptr.sidecar, ptr.capability, origin);
}

static inline void filc_testing_validate_ptr(filc_ptr ptr)
{
    if (PAS_ENABLE_TESTING)
        filc_validate_ptr(ptr, NULL);
}

static inline unsigned filc_ptr_capability_type_index(filc_ptr ptr)
{
    return (unsigned)(ptr.capability >> (pas_uint128)96) & FILC_TYPE_INDEX_MASK;
}

static inline filc_capability_kind filc_ptr_capability_kind(filc_ptr ptr)
{
    return (filc_capability_kind)(ptr.capability >> (pas_uint128)126);
}

static inline bool filc_ptr_is_boxed_int(filc_ptr ptr)
{
    return !filc_ptr_capability_type_index(ptr)
        && filc_ptr_capability_kind(ptr) != filc_capability_oob;
}

static inline bool filc_ptr_capability_has_things(filc_ptr ptr)
{
    return filc_ptr_capability_type_index(ptr);
}

static inline void* filc_ptr_ptr(filc_ptr ptr)
{
    if (filc_ptr_is_boxed_int(ptr))
        return (void*)(uintptr_t)ptr.capability;
    return (void*)((uintptr_t)ptr.capability & PAS_ADDRESS_MASK);
}

void* filc_ptr_ptr_impl(pas_uint128 sidecar, pas_uint128 capability);

static inline void* filc_ptr_capability_upper(filc_ptr ptr)
{
    PAS_TESTING_ASSERT(filc_ptr_capability_has_things(ptr));
    return (void*)((uintptr_t)(ptr.capability >> (pas_uint128)48) & PAS_ADDRESS_MASK);
}

/* Not meant to be called directly; this is here so that we can implement filc_ptr_type. */
static inline const filc_type* filc_ptr_capability_type(filc_ptr ptr)
{
    return filc_type_lookup(filc_ptr_capability_type_index(ptr));
}

/* Sidecar methods aren't meant to be called directly; they're here so that we can implement
   filc_ptr_lower. */
static inline void* filc_ptr_sidecar_ptr_or_upper(filc_ptr ptr)
{
    return (void*)((uintptr_t)ptr.sidecar & PAS_ADDRESS_MASK);
}

static inline void* filc_ptr_sidecar_lower_or_upper(filc_ptr ptr)
{
    return (void*)((uintptr_t)(ptr.sidecar >> (pas_uint128)48) & PAS_ADDRESS_MASK);
}

static inline unsigned filc_ptr_sidecar_type_index(filc_ptr ptr)
{
    return (unsigned)(ptr.sidecar >> (pas_uint128)96) & FILC_TYPE_INDEX_MASK;
}

static inline bool filc_ptr_sidecar_is_blank(filc_ptr ptr)
{
    return !filc_ptr_sidecar_type_index(ptr);
}

static inline filc_sidecar_kind filc_ptr_sidecar_kind(filc_ptr ptr)
{
    return (filc_sidecar_kind)(ptr.sidecar >> (pas_uint128)126);
}

static inline void* filc_ptr_sidecar_ptr(filc_ptr ptr)
{
    PAS_TESTING_ASSERT(filc_ptr_sidecar_kind(ptr) == filc_sidecar_lower
                       || filc_ptr_sidecar_kind(ptr) == filc_sidecar_flex_upper);
    return filc_ptr_sidecar_ptr_or_upper(ptr);
}

static inline void* filc_ptr_sidecar_lower(filc_ptr ptr)
{
    PAS_TESTING_ASSERT(filc_ptr_sidecar_kind(ptr) == filc_sidecar_lower
                       || filc_ptr_sidecar_kind(ptr) == filc_sidecar_oob_capability);
    return filc_ptr_sidecar_lower_or_upper(ptr);
}

static inline void* filc_ptr_sidecar_upper(filc_ptr ptr)
{
    PAS_TESTING_ASSERT(filc_ptr_sidecar_kind(ptr) == filc_sidecar_flex_upper
                       || filc_ptr_sidecar_kind(ptr) == filc_sidecar_oob_capability);
    if (filc_ptr_sidecar_kind(ptr) == filc_sidecar_oob_capability)
        return filc_ptr_sidecar_ptr_or_upper(ptr);
    return filc_ptr_sidecar_lower_or_upper(ptr);
}

static inline const filc_type* filc_ptr_sidecar_type(filc_ptr ptr)
{
    return filc_type_lookup(filc_ptr_sidecar_type_index(ptr));
}

static inline bool filc_ptr_sidecar_is_relevant(filc_ptr ptr)
{
    const filc_type* capability_type;
    const filc_type* sidecar_type;
    if (filc_ptr_is_boxed_int(ptr))
        return false;
    switch (filc_ptr_capability_kind(ptr)) {
    case filc_capability_at_lower:
        return false;
    case filc_capability_in_array:
        if (filc_ptr_sidecar_kind(ptr) != filc_sidecar_lower)
            return false;
        break;
    case filc_capability_flex_base:
        if (filc_ptr_sidecar_kind(ptr) != filc_sidecar_flex_upper)
            return false;
        break;
    case filc_capability_oob:
        return filc_ptr_sidecar_kind(ptr) == filc_sidecar_oob_capability;
    }
    if (filc_ptr_sidecar_ptr(ptr) != filc_ptr_ptr(ptr))
        return false;
    capability_type = filc_ptr_capability_type(ptr);
    sidecar_type = filc_ptr_sidecar_type(ptr);
    if (sidecar_type == capability_type)
        return true;
    PAS_TESTING_ASSERT(filc_ptr_capability_kind(ptr) != filc_capability_flex_base);
    PAS_TESTING_ASSERT(!filc_ptr_sidecar_is_blank(ptr));
    if (!sidecar_type->num_words)
        return false;
    return sidecar_type->u.trailing_array == capability_type;
}

static inline const filc_type* filc_ptr_type(filc_ptr ptr)
{
    if (filc_ptr_sidecar_is_relevant(ptr))
        return filc_ptr_sidecar_type(ptr);
    return filc_ptr_capability_type(ptr);
}

static inline void* filc_ptr_upper(filc_ptr ptr)
{
    if (filc_ptr_is_boxed_int(ptr))
        return NULL;

    switch (filc_ptr_capability_kind(ptr)) {
    case filc_capability_at_lower:
    case filc_capability_in_array:
        return filc_ptr_capability_upper(ptr);

    case filc_capability_flex_base:
        if (!filc_ptr_sidecar_is_relevant(ptr))
            return filc_ptr_capability_upper(ptr);
        
        return filc_ptr_sidecar_upper(ptr);

    case filc_capability_oob:
        if (filc_ptr_sidecar_kind(ptr) == filc_sidecar_oob_capability)
            return filc_ptr_sidecar_upper(ptr);
        
        return NULL;
    }

    PAS_ASSERT(!"Should not be reached");
    return NULL;
}

static inline void* filc_ptr_lower(filc_ptr ptr)
{
    static const bool verbose = false;
    
    uintptr_t distance_to_upper;
    uintptr_t distance_to_upper_of_element;

    if (filc_ptr_is_boxed_int(ptr))
        return NULL;

    PAS_TESTING_ASSERT(filc_ptr_capability_type(ptr)
                       || filc_ptr_capability_kind(ptr) == filc_capability_oob);

    if (verbose)
        pas_log("capability kind = %u\n", (unsigned)filc_ptr_capability_kind(ptr));

    if (filc_ptr_capability_kind(ptr) == filc_capability_at_lower)
        return filc_ptr_ptr(ptr);

    if (filc_ptr_sidecar_is_relevant(ptr)
        && filc_ptr_capability_kind(ptr) != filc_capability_flex_base) {
        PAS_TESTING_ASSERT(filc_ptr_sidecar_lower(ptr) <= filc_ptr_upper(ptr));
        return filc_ptr_sidecar_lower(ptr);
    }

    if (filc_ptr_capability_kind(ptr) == filc_capability_oob)
        return NULL;

    PAS_TESTING_ASSERT(filc_ptr_ptr(ptr) <= filc_ptr_upper(ptr));
    if (filc_ptr_ptr(ptr) >= filc_ptr_upper(ptr))
        return filc_ptr_upper(ptr);

    distance_to_upper = (char*)filc_ptr_capability_upper(ptr) - (char*)filc_ptr_ptr(ptr);
    PAS_TESTING_ASSERT(filc_ptr_capability_kind(ptr) == filc_capability_in_array
                       || filc_ptr_capability_kind(ptr) == filc_capability_flex_base);
    if (filc_ptr_capability_kind(ptr) == filc_capability_flex_base) {
        PAS_TESTING_ASSERT(distance_to_upper);
        PAS_TESTING_ASSERT(distance_to_upper < filc_ptr_capability_type(ptr)->size);
    }
    distance_to_upper_of_element = distance_to_upper % filc_ptr_type(ptr)->size;
    if (!distance_to_upper_of_element)
        return filc_ptr_ptr(ptr);
    
    return (char*)filc_ptr_ptr(ptr)
        + distance_to_upper_of_element - filc_ptr_capability_type(ptr)->size;
}

static inline uintptr_t filc_ptr_offset(filc_ptr ptr)
{
    return (char*)filc_ptr_ptr(ptr) - (char*)filc_ptr_lower(ptr);
}

static inline uintptr_t filc_ptr_available(filc_ptr ptr)
{
    return (char*)filc_ptr_upper(ptr) - (char*)filc_ptr_ptr(ptr);
}

static inline filc_ptr filc_ptr_create(pas_uint128 sidecar, pas_uint128 capability)
{
    filc_ptr result;
    result.sidecar = sidecar;
    result.capability = capability;
    filc_testing_validate_ptr(result);
    return result;
}

PAS_API filc_ptr filc_ptr_forge(void* ptr, void* lower, void* upper, const filc_type* type);

static inline filc_ptr filc_ptr_forge_with_size(void* ptr, size_t size, const filc_type* type)
{
    return filc_ptr_forge(ptr, ptr, (char*)ptr + size, type);
}

static inline filc_ptr filc_ptr_forge_byte(void* ptr, const filc_type* type)
{
    return filc_ptr_forge_with_size(ptr, 1, type);
}

static inline filc_ptr filc_ptr_forge_invalid(void* ptr)
{
    return filc_ptr_forge(ptr, NULL, NULL, NULL);
}

static inline filc_ptr filc_ptr_with_ptr(filc_ptr ptr, void* new_ptr)
{
    return filc_ptr_forge(
        new_ptr, filc_ptr_lower(ptr), filc_ptr_upper(ptr), filc_ptr_type(ptr));
}

static inline filc_ptr filc_ptr_with_offset(filc_ptr ptr, uintptr_t offset)
{
    return filc_ptr_with_ptr(ptr, (char*)filc_ptr_ptr(ptr) + offset);
}

static inline bool filc_ptr_is_totally_equal(filc_ptr a, filc_ptr b)
{
    return a.sidecar == b.sidecar
        && a.capability == b.capability;
}

static inline bool filc_ptr_is_totally_null(filc_ptr ptr)
{
    return filc_ptr_is_totally_equal(ptr, filc_ptr_forge_invalid(NULL));
}

static inline filc_ptr filc_ptr_load(filc_ptr* ptr)
{
    filc_ptr result;
    result.sidecar = __c11_atomic_load((_Atomic pas_uint128*)&ptr->sidecar, __ATOMIC_RELAXED);
    result.capability = __c11_atomic_load((_Atomic pas_uint128*)&ptr->capability, __ATOMIC_RELAXED);
    return result;
}

static inline void filc_ptr_store(filc_ptr* ptr, filc_ptr value)
{
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, value.sidecar, __ATOMIC_RELAXED);
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->capability, value.capability, __ATOMIC_RELAXED);
}

static inline bool filc_ptr_unfenced_weak_cas(
    filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    /* This is optional; it's legal to do it or not do it, from the standpoint of FilC soundness.
       If whoever reads the ptr sees a sidecar that doesn't match the capability, then it'll be
       rejected anyway. Nuking the sidecar turns a rarely-occurring bad case into a deterministic
       bad case, so code has to always deal with it. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

static inline filc_ptr filc_ptr_unfenced_strong_cas(
    filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    return filc_ptr_create(0, expected.capability);
}

static inline bool filc_ptr_weak_cas(filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline filc_ptr filc_ptr_strong_cas(filc_ptr* ptr, filc_ptr expected, filc_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return filc_ptr_create(0, expected.capability);
}

static inline filc_ptr filc_ptr_unfenced_xchg(filc_ptr* ptr, filc_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return filc_ptr_create(
        0,
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->capability, new_value.capability, __ATOMIC_RELAXED));
}

static inline filc_ptr filc_ptr_xchg(filc_ptr* ptr, filc_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return filc_ptr_create(
        0,
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->capability, new_value.capability, __ATOMIC_SEQ_CST));
}

PAS_API void filc_ptr_dump(filc_ptr ptr, pas_stream* stream);
PAS_API char* filc_ptr_to_new_string(filc_ptr ptr); /* WARNING: this is different from
                                                           zptr_to_new_string. This uses the
                                                           utility heap - WHICH MUST NEVER LEAK TO
                                                           PIZLONATED CODE - whereas the other one
                                                           uses the int heap. */

/* Returns true if this type has no trailing array and is not opaque. */
static inline bool filc_type_is_normal(const filc_type* type)
{
    return type->num_words && !type->u.trailing_array;
}

static inline const filc_type* filc_type_get_trailing_array(const filc_type* type)
{
    if (type->num_words)
        return type->u.trailing_array;
    return NULL;
}

static inline filc_word_type filc_type_get_word_type(const filc_type* type,
                                                         uintptr_t word_type_index)
{
    PAS_TESTING_ASSERT(type->num_words);
    
    if (type->u.trailing_array) {
        if (word_type_index >= type->num_words) {
            word_type_index -= type->num_words;
            type = type->u.trailing_array;
        }
    }
    return type->word_types[word_type_index % type->num_words];
}

/* Run assertions on the type itself. The runtime isn't guaranteed to ever run this check. The
   compiler is expected to only produce types that pass this check, and we provide no path for
   the user to create types (unless they write unsafe code). */
void filc_validate_type(const filc_type* type, const filc_origin* origin);

static inline bool filc_type_is_equal(const filc_type* a, const filc_type* b)
{
    return a == b;
}

static inline unsigned filc_type_hash(const filc_type* type)
{
    return pas_hash_ptr(type);
}

PAS_API bool filc_type_template_is_equal(const filc_type_template* a,
                                           const filc_type_template* b);
PAS_API unsigned filc_type_template_hash(const filc_type_template* type);

static inline size_t filc_type_representation_size(const filc_type* type)
{
    return PAS_OFFSETOF(filc_type, word_types)
        + sizeof(filc_word_type) * type->num_words;
}
    
static inline size_t filc_type_as_heap_type_get_type_size(const pas_heap_type* heap_type)
{
    const filc_type* type = (const filc_type*)heap_type;
    PAS_TESTING_ASSERT(type->size);
    PAS_TESTING_ASSERT(type->alignment);
    return pas_round_up_to_power_of_2(type->size, type->alignment);
}

static inline size_t filc_type_as_heap_type_get_type_alignment(const pas_heap_type* heap_type)
{
    const filc_type* type = (const filc_type*)heap_type;
    PAS_TESTING_ASSERT(type->size);
    PAS_TESTING_ASSERT(type->alignment);
    return type->alignment;
}

PAS_API pas_heap_runtime_config* filc_type_as_heap_type_get_runtime_config(
    const pas_heap_type* type, pas_heap_runtime_config* config);
PAS_API pas_heap_runtime_config* filc_type_as_heap_type_assert_default_runtime_config(
    const pas_heap_type* type, pas_heap_runtime_config* config);

PAS_API void filc_word_type_dump(filc_word_type type, pas_stream* stream);
PAS_API char* filc_word_type_to_new_string(filc_word_type type);

PAS_API void filc_type_dump(const filc_type* type, pas_stream* stream);
PAS_API void filc_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream);

char* filc_type_to_new_string(const filc_type* type); /* WARNING: this is different from
                                                             ztype_to_new_string. This uses the
                                                             utility heap - WHICH MUST NEVER LEAK TO
                                                             PIZLONATED CODE - whereas the other one
                                                             uses the int heap. */

PAS_API const filc_type* filc_type_slice(const filc_type* type, pas_range range,
                                             const filc_origin* origin);
PAS_API const filc_type* filc_type_cat(const filc_type* a, size_t a_size,
                                           const filc_type* b, size_t b_size,
                                           const filc_origin* origin);

/* This is basically va_arg, but it doesn't check that the type matches. That's fine if the consumer
   of the pointer is code compiled by FilC, since that will check on every access. That's not fine if
   the consumer is someone writing legacy C code against some FilC flight API. */
static inline filc_ptr filc_ptr_get_next_bytes(
    filc_ptr* ptr, size_t size, size_t alignment)
{
    filc_ptr ptr_value;
    uintptr_t ptr_as_int;
    filc_ptr result;

    ptr_value = filc_ptr_load(ptr);
    ptr_as_int = (uintptr_t)filc_ptr_ptr(ptr_value);
    ptr_as_int = pas_round_up_to_power_of_2(ptr_as_int, alignment);

    result = filc_ptr_with_ptr(ptr_value, (void*)ptr_as_int);

    filc_ptr_store(ptr, filc_ptr_with_ptr(ptr_value, (char*)ptr_as_int + size));

    return result;
}

/* Gives you a heap for the given type. This looks up the heap based on the structural equality
   of the type, so equal types get the same heap.

   It's correct to call this every time you do a memory allocation. It's not the greatest idea, but
   it will be lock-free (and basically fence-free) in the common case.

   The type you pass here must be immortal. The runtime may refer to it forever. (That's not
   strictly necessary so we could change that if we needed to.) */
pas_heap_ref* filc_get_heap(const filc_type* type);

void* filc_allocate_int(size_t size, size_t cout);
void* filc_allocate_int_with_alignment(size_t size, size_t count, size_t alignment);

void* filc_allocate_one(pas_heap_ref* ref);
void* filc_allocate_opaque(pas_heap_ref* ref);
void* filc_allocate_many(pas_heap_ref* ref, size_t count);
void* filc_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment);

void* filc_allocate_int_flex(size_t base_size, size_t element_size, size_t count);
void* filc_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                            size_t alignment);
void* filc_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count);
void* filc_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                        size_t count, size_t alignment);

/* This can allocate any type (ints or not), but it's considerably slower than the other allocation
   entrypoints. The compiler avoids this in most cases. */
void* filc_allocate_with_type(const filc_type* type, size_t size);

/* FIXME: It would be great if this had a separate freeing function, and filc_deallocate asserted
   if it detected that it was used to free a utility allocation.
   
   Right now, we just rely on the fact that zfree (and friends) do a bunch of checks that effectively
   prevent freeing utility allocations. */
void* filc_allocate_utility(size_t size);

void* filc_reallocate_int_impl(pas_uint128 sidecar, pas_uint128 capability, size_t size, size_t count,
                               const filc_origin* origin);
void* filc_reallocate_int_with_alignment_impl(pas_uint128 sidecar, pas_uint128 capability,
                                              size_t size, size_t count, size_t alignment,
                                              const filc_origin* origin);

void* filc_reallocate_impl(pas_uint128 sidecar, pas_uint128 capability,
                           pas_heap_ref* ref, size_t count, const filc_origin* origin);

/* Deallocation usually checks if the thread is in_filc. But, some deallocations happen during
   thread destruction, when we've blocked all signals. Deallocations that can happen at that time
   call the yolo version, which has no such check. This is a testing-only check, so the yolo version
   is equivalent to the normal one in production. */
void filc_deallocate_yolo(const void* ptr);

/* Deallocate without the Fil-C type system checks. Super dangerous to use except for pointers
   allocated by the Fil-C runtime! */
void filc_deallocate(const void* ptr);

/* Deallocate with all of the checks! This is equivalent to filc_check_deallocate and then
   filc_deallocate. */
void filc_deallocate_safe(filc_ptr ptr);

void pizlonated_f_zfree(PIZLONATED_SIGNATURE);
void pizlonated_f_zgetallocsize(PIZLONATED_SIGNATURE);

void pizlonated_f_zcalloc_multiply(PIZLONATED_SIGNATURE);

pas_heap_ref* filc_get_hard_heap(const filc_type* type);

void* filc_hard_allocate_int(size_t size, size_t count);
void* filc_hard_allocate_int_with_alignment(size_t size, size_t count, size_t alignment);
void* filc_hard_allocate_one(pas_heap_ref* ref);
void* filc_hard_allocate_many(pas_heap_ref* ref, size_t count);
void* filc_hard_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment);
void* filc_hard_allocate_int_flex(size_t base_size, size_t element_size, size_t count);
void* filc_hard_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                                   size_t alignment);
void* filc_hard_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count);
void* filc_hard_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                               size_t count, size_t alignment);
void* filc_hard_reallocate_int_impl(pas_uint128 sidecar, pas_uint128 capability, size_t size, size_t count,
                                    const filc_origin* origin);
void* filc_hard_reallocate_int_with_alignment_impl(pas_uint128 sidecar, pas_uint128 capability,
                                                   size_t size, size_t count, size_t alignment,
                                                   const filc_origin* origin);

void* filc_hard_reallocate(pas_uint128 sidecar, pas_uint128 capability, pas_heap_ref* ref, size_t count,
                           const filc_origin* origin);
void filc_hard_deallocate(const void* ptr);

/* This is super gross but it's fine for now. When the compiler allocates, it emits three calls.
   First it calls the allocator. Then it calls new_capability. Then it calls new_sidecar.
   
   That's dumb!
   
   Why do I do it for now?
   
   - I also want to call all of these allocation functions from C code in the runtime, and most of
     the time, that code just wants a void*. So if I wanted allocation functions that returned
     a wide pointer, or even just the pointer's capability, then I'd have to have duplicate
     entrypoints. I don't feel like writing that code right now, but I'll almost certainly do it
     eventually.
   
   - It's super annoying to have code that returns a filc_ptr called from compiler-generated code,
     since at the point I'm at in LLVM, the returns for structs may (or may not!) have been lowered
     to some kind of pointer-passing convention. I can make LLVM return a struct like filc_ptr in
     registers, but this is C code, and so these functions will have C's calling convention, which is
     different from LLVM's when it comes to struct return! There are many ways around this,
     especially since I control the compiler (I am not above having a separate hacked clang just for
     compiling libfilc, or rewriting all of this in assembly, or in LLVM IR), but I don't feel
     like doing that right now.
   
   Therfore, I have functions to build the capability and sidecar separately. Gross but at least it
   gets me there.

   Also, it's not actually necessary to know the size to construct the sidecar, but whatever, doing
   it this way makes it more obviously correct. */
pas_uint128 filc_new_capability(void* ptr, size_t size, const filc_type* type);
pas_uint128 filc_new_sidecar(void* ptr, size_t size, const filc_type* type);

void filc_log_allocation(filc_ptr ptr, const filc_origin* origin);
void filc_log_allocation_impl(pas_uint128 sidecar, pas_uint128 capability,
                                const filc_origin* origin);

void filc_check_forge(
    void* ptr, size_t size, size_t count, const filc_type* type, const filc_origin* origin);

void pizlonated_f_zhard_free(PIZLONATED_SIGNATURE);
void pizlonated_f_zhard_getallocsize(PIZLONATED_SIGNATURE);

void pizlonated_f_zgetlower(PIZLONATED_SIGNATURE);
void pizlonated_f_zgetupper(PIZLONATED_SIGNATURE);
void pizlonated_f_zgettype(PIZLONATED_SIGNATURE);
void pizlonated_f_zisint(PIZLONATED_SIGNATURE);
void pizlonated_f_zptrphase(PIZLONATED_SIGNATURE);

void pizlonated_f_zslicetype(PIZLONATED_SIGNATURE);
void pizlonated_f_zgettypeslice(PIZLONATED_SIGNATURE);
void pizlonated_f_zcattype(PIZLONATED_SIGNATURE);
void pizlonated_f_zalloc_with_type(PIZLONATED_SIGNATURE);

void pizlonated_f_ztype_to_new_string(PIZLONATED_SIGNATURE);
void pizlonated_f_zptr_to_new_string(PIZLONATED_SIGNATURE);

/* The compiler uses these for now because it makes LLVM codegen ridiculously easy to get right,
   but it's totally the wrong way to do it from a perf standpoint. */
pas_uint128 filc_update_sidecar(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr);
pas_uint128 filc_update_capability(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr);

void filc_check_deallocate_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const filc_origin* origin);
static inline void filc_check_deallocate(filc_ptr ptr, const filc_origin* origin)
{
    filc_check_deallocate_impl(ptr.sidecar, ptr.capability, origin);
}

void filc_check_access_int_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  uintptr_t bytes, const filc_origin* origin);
void filc_check_access_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const filc_origin* origin);

static inline void filc_check_access_int(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    filc_check_access_int_impl(ptr.sidecar, ptr.capability, bytes, origin);
}
static inline void filc_check_access_ptr(filc_ptr ptr, const filc_origin* origin)
{
    filc_check_access_ptr_impl(ptr.sidecar, ptr.capability, origin);
}

void filc_check_function_call_impl(pas_uint128 sidecar, pas_uint128 capability,
                                     const filc_origin* origin);

static inline void filc_check_function_call(filc_ptr ptr, const filc_origin* origin)
{
    filc_check_function_call_impl(ptr.sidecar, ptr.capability, origin);
}

void filc_check_access_opaque(
    filc_ptr ptr, const filc_type* expected_type, const filc_origin* origin);

/* You can call these if you know that you're copying pointers (or possible pointers) and you've
   already proved that it's safe and the pointer/size are aligned.
   
   These also happen to be used as the implementation of filc_memset_impl/filc_memcpy_impl/
   filc_memmove_impl in those cases where pointer copying is detected. Those also do all the
   checks needed to ensure memory safety. So, usually, you want those, not these.

   The number of bytes must be a multiple of 16. */
void filc_low_level_ptr_safe_bzero(void* ptr, size_t bytes);
void filc_low_level_ptr_safe_memcpy(void* dst, void* src, size_t bytes);
void filc_low_level_ptr_safe_memmove(void* dst, void* src, size_t bytes);

void filc_memset_impl(pas_uint128 sidecar, pas_uint128 capability,
                        unsigned value, size_t count, const filc_origin* origin);
void filc_memcpy_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                        pas_uint128 src_sidecar, pas_uint128 src_capability,
                        size_t count, const filc_origin* origin);
void filc_memmove_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                         pas_uint128 src_sidecar, pas_uint128 src_capability,
                         size_t count, const filc_origin* origin);

static inline void filc_memset(filc_ptr ptr, unsigned value, size_t count, const filc_origin* origin)
{
    filc_memset_impl(ptr.sidecar, ptr.capability, value, count, origin);
}

static inline void filc_memcpy(filc_ptr dst, filc_ptr src, size_t count, const filc_origin* origin)
{
    filc_memcpy_impl(dst.sidecar, dst.capability, src.sidecar, src.capability, count, origin);
}

static inline void filc_memmove(filc_ptr dst, filc_ptr src, size_t count, const filc_origin* origin)
{
    filc_memmove_impl(dst.sidecar, dst.capability, src.sidecar, src.capability, count, origin);
}

/* Correct uses of this should always pass new_upper = ptr + K * new_type->size. This function does
   not have to check that you did that. This property is ensured by the compiler and the fact that
   the user-visible API (zrestrict) takes a count. */
void filc_check_restrict(pas_uint128 sidecar, pas_uint128 capability,
                         void* new_upper, const filc_type* new_type, const filc_origin* origin);

static inline filc_ptr filc_restrict(filc_ptr ptr, size_t count, const filc_type* new_type,
                                     const filc_origin* origin)
{
    void* new_upper;
    new_upper = (char*)filc_ptr_ptr(ptr) + count * new_type->size;
    filc_check_restrict(ptr.sidecar, ptr.capability, new_upper, new_type, origin);
    return filc_ptr_forge(filc_ptr_ptr(ptr), filc_ptr_ptr(ptr), new_upper, new_type);
}

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
const char* filc_check_and_get_new_str(filc_ptr ptr, const filc_origin* origin);

const char* filc_check_and_get_new_str_or_null(filc_ptr ptr, const filc_origin* origin);

filc_ptr filc_strdup(const char* str);

/* This is basically va_arg. Whatever kind of API we expose to native C code to interact with FilC
   code will have to use this kind of API to parse the flights. */
static inline filc_ptr filc_ptr_get_next(
    filc_ptr* ptr, size_t count, size_t alignment, const filc_type* type,
    const filc_origin* origin)
{
    return filc_restrict(filc_ptr_get_next_bytes(
                               ptr, count * type->size, alignment),
                           count, type, origin);
}

/* NOTE: It's tempting to add a macro that takes a type and does get_next, but I don't see how
   that would handle pointers correctly. */

static inline filc_ptr filc_ptr_get_next_ptr(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(filc_ptr), alignof(filc_ptr));
    filc_check_access_ptr(slot_ptr, origin);
    return filc_ptr_load((filc_ptr*)filc_ptr_ptr(slot_ptr));
}

static inline int filc_ptr_get_next_int(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(int), alignof(int));
    filc_check_access_int(slot_ptr, sizeof(int), origin);
    return *(int*)filc_ptr_ptr(slot_ptr);
}

static inline unsigned filc_ptr_get_next_unsigned(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(unsigned), alignof(unsigned));
    filc_check_access_int(slot_ptr, sizeof(unsigned), origin);
    return *(unsigned*)filc_ptr_ptr(slot_ptr);
}

static inline long filc_ptr_get_next_long(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(long), alignof(long));
    filc_check_access_int(slot_ptr, sizeof(long), origin);
    return *(long*)filc_ptr_ptr(slot_ptr);
}

static inline unsigned long filc_ptr_get_next_unsigned_long(filc_ptr* ptr,
                                                              const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(unsigned long), alignof(unsigned long));
    filc_check_access_int(slot_ptr, sizeof(unsigned long), origin);
    return *(unsigned long*)filc_ptr_ptr(slot_ptr);
}

static inline size_t filc_ptr_get_next_size_t(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(size_t), alignof(size_t));
    filc_check_access_int(slot_ptr, sizeof(size_t), origin);
    return *(size_t*)filc_ptr_ptr(slot_ptr);
}

static inline double filc_ptr_get_next_double(filc_ptr* ptr, const filc_origin* origin)
{
    filc_ptr slot_ptr;
    slot_ptr = filc_ptr_get_next_bytes(ptr, sizeof(double), alignof(double));
    filc_check_access_int(slot_ptr, sizeof(double), origin);
    return *(double*)filc_ptr_ptr(slot_ptr);
}

/* Given a va_list ptr (so a ptr to a ptr), this:
   
   - checks that it is indeed a ptr to a ptr
   - uses that ptr to va_arg according to the filc_ptr_get_next_bytes protocol
   - checks that the resulting ptr can be restricted to count*type
   - if all that goes well, returns a ptr you can load/store from safely according to that type */
void* filc_va_arg_impl(
    pas_uint128 va_list_sidecar, pas_uint128 va_list_capability,
    size_t count, size_t alignment, const filc_type* type, const filc_origin* origin);

/* If parent is not NULL, increases its ref count and returns it. Otherwise, creates a new context. */
filc_global_initialization_context* filc_global_initialization_context_create(
    filc_global_initialization_context* parent);
/* Attempts to add the given global to be initialized.
   
   If it's already in the set then this returns false.
   
   If it's not in the set, then it's added to the set and true is returned. */
bool filc_global_initialization_context_add(
    filc_global_initialization_context* context, pas_uint128* pizlonated_gptr, pas_uint128 ptr_capability);
/* Derefs the context. If the refcount reaches zero, it gets destroyed.
 
   Destroying the set means storing all known ptr_capabilities into their corresponding pizlonated_gptrs
   atomically. */
void filc_global_initialization_context_destroy(filc_global_initialization_context* context);

void filc_alloca_stack_push(filc_alloca_stack* stack, void* alloca);
static inline size_t filc_alloca_stack_save(filc_alloca_stack* stack) { return stack->size; }
void filc_alloca_stack_restore(filc_alloca_stack* stack, size_t size);
void filc_alloca_stack_destroy(filc_alloca_stack* stack);

void filc_execute_constant_relocations(
    void* constant, filc_constant_relocation* relocations, size_t num_relocations,
    filc_global_initialization_context* context);

void filc_defer_or_run_global_ctor(void (*global_ctor)(PIZLONATED_SIGNATURE));
void filc_run_deferred_global_ctors(void); /* Important safety property: libc must call this before
                                                letting the user start threads. But it's OK if the
                                                deferred constructors that this calls start threads,
                                                as far as safety goes. */
void pizlonated_f_zrun_deferred_global_ctors(PIZLONATED_SIGNATURE);

void filc_error(const char* reason, const filc_origin* origin);

void pizlonated_f_zprint(PIZLONATED_SIGNATURE);
void pizlonated_f_zprint_long(PIZLONATED_SIGNATURE);
void pizlonated_f_zprint_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zerror(PIZLONATED_SIGNATURE);
void pizlonated_f_zstrlen(PIZLONATED_SIGNATURE);
void pizlonated_f_zisdigit(PIZLONATED_SIGNATURE);

void pizlonated_f_zfence(PIZLONATED_SIGNATURE);
void pizlonated_f_zstore_store_fence(PIZLONATED_SIGNATURE);
void pizlonated_f_zcompiler_fence(PIZLONATED_SIGNATURE);
void pizlonated_f_zunfenced_weak_cas_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zweak_cas_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zunfenced_strong_cas_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zstrong_cas_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zunfenced_xchg_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zxchg_ptr(PIZLONATED_SIGNATURE);

void pizlonated_f_zis_runtime_testing_enabled(PIZLONATED_SIGNATURE);
void pizlonated_f_zborkedptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zvalidate_ptr(PIZLONATED_SIGNATURE);
void pizlonated_f_zscavenger_suspend(PIZLONATED_SIGNATURE);
void pizlonated_f_zscavenger_resume(PIZLONATED_SIGNATURE);

/* Amusingly, the order of these functions tell the story of me porting musl to filc. */
void pizlonated_f_zregister_sys_errno_handler(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_ioctl(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_writev(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_read(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_readv(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_write(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_close(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_lseek(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_exit(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_geteuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getgid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getegid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_open(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getpid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_clock_gettime(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_fstatat(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_fstat(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_fcntl(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getpwuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_sigaction(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_isatty(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_pipe(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_select(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_sched_yield(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_socket(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setsockopt(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_bind(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getaddrinfo(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_connect(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getsockname(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getsockopt(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getpeername(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_sendto(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_recvfrom(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getrlimit(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_umask(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_uname(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getitimer(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setitimer(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_pause(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_pselect(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getpeereid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_kill(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_raise(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_dup(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_dup2(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_sigprocmask(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getpwnam(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setgroups(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_opendir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_fdopendir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_closedir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_readdir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_rewinddir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_seekdir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_telldir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_closelog(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_openlog(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setlogmask(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_syslog(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_chdir(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_fork(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_waitpid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_listen(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_accept(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_socketpair(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setsid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_execve(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getppid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_chroot(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_seteuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setreuid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setgid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setegid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_setregid(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_nanosleep(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getgroups(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_getgrouplist(PIZLONATED_SIGNATURE);
void pizlonated_f_zsys_readlink(PIZLONATED_SIGNATURE);

void pizlonated_f_zthread_self(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_get_id(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_get_cookie(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_set_self_cookie(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_create(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_join(PIZLONATED_SIGNATURE);
void pizlonated_f_zthread_detach(PIZLONATED_SIGNATURE);

void pizlonated_f_zpark_if(PIZLONATED_SIGNATURE);
void pizlonated_f_zunpark_one(PIZLONATED_SIGNATURE);
void pizlonated_f_zunpark_all(PIZLONATED_SIGNATURE);

PAS_END_EXTERN_C;

#endif /* FILC_RUNTIME_H */

