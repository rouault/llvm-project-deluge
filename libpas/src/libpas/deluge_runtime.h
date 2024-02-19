#ifndef DELUGE_RUNTIME_H
#define DELUGE_RUNTIME_H

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

/* Internal Deluge runtime header, defining how the Deluge runtime maintains its state.
 
   Currently, including this header is the only way to perform FFI to Deluge code, and the API for
   that is too low-level for comfort. That's probably fine, since the Deluge ABI is going to
   change the moment I start giving a shit about performance.

   This runtime is engineered under the following principles:

   - It's based on libpas, so we get fully correct isoheap semantics and the perf of allocation in
     those heaps is as good as what I can come up with after ~5 years of effort. The isoheap code
     has been thoroughly battle-tested and we can trust it.

   - Coding standards have to be extremely high, and assert usage should be full-ass
     belt-and-suspenders. The goal of this code is to achieve memory safety under the Deluge
     "Bounded P^I" type system. It's fine to take extra cycles or bytes to achieve that goal.

   - There are no optimizations yet, but the structure of this code is such that when I choose to 
     go into optimization mode, I will be able to wreak havoc I'm just holding back from going
     there, for now. Lots of running code is better than a small amount of fast code. */

struct deluge_alloca_stack;
struct deluge_constant_relocation;
struct deluge_constexpr_node;
struct deluge_global_initialization_context;
struct deluge_initialization_entry;
struct deluge_origin;
struct deluge_ptr;
struct deluge_type;
struct deluge_type_template;
struct pas_basic_heap_runtime_config;
struct pas_stream;
typedef struct deluge_alloca_stack deluge_alloca_stack;
typedef struct deluge_constant_relocation deluge_constant_relocation;
typedef struct deluge_constexpr_node deluge_constexpr_node;
typedef struct deluge_global_initialization_context deluge_global_initialization_context;
typedef struct deluge_initialization_entry deluge_initialization_entry;
typedef struct deluge_origin deluge_origin;
typedef struct deluge_ptr deluge_ptr;
typedef struct deluge_type deluge_type;
typedef struct deluge_type_template deluge_type_template;
typedef struct pas_basic_heap_runtime_config pas_basic_heap_runtime_config;
typedef struct pas_stream pas_stream;

typedef uint8_t deluge_word_type;

/* Note that any statement like "128-bit word" below needs to be understood with the caveat that ptr
   access also checks bounds. So, a pointer might say it has type "128-bit int" but bounds that say
   "1 byte", in which case you get the intersection: "8-bit int". */
#define DELUGE_WORD_TYPE_OFF_LIMITS        ((uint8_t)0)     /* 128-bit that cannot be accessed. */
#define DELUGE_WORD_TYPE_INT               ((uint8_t)1)     /* 128-bit word that contains ints.
                                                               Primitive ptrs may have bounds that
                                                               are less than 16 bytes and may not have
                                                               16 byte alignment. */
#define DELUGE_WORD_TYPE_PTR_SIDECAR       ((uint8_t)2)     /* 128-bit word that contains the sidecar
                                                               of a wide ptr. */
#define DELUGE_WORD_TYPE_PTR_CAPABILITY    ((uint8_t)3)     /* 128-bit word that contains the capability
                                                               of a wide ptr. */

#define DELUGE_WORD_SIZE sizeof(pas_uint128)

/* Helper for emitting the ptr word types */
#define DELUGE_WORD_TYPES_PTR \
    DELUGE_WORD_TYPE_PTR_SIDECAR, \
    DELUGE_WORD_TYPE_PTR_CAPABILITY

/* The deluge pointer, represented using its heap format. Let's call this the rest format. Pointers must
   have this format when they are in any memory location of pointer type (i.e. the pair of SIDECAR and
   CAPABILITY word types).
   
   It would be smart to someday optimize both the compiler and the runtime for the deluge pointer's flight
   format: ptr, lower, upper, and type. That would be an awesome optimization, but I haven't done it yet,
   because I am not yet interested in Deluge's performance so long as I don't have a lot of running code.
   
   Instead, we pass around deluge_ptrs, but to do anything with them, we have to call getters to get the
   ptr/lower/upper/type. That's the current idiom even though it is surely inefficient and confusing for
   debugging, since we don't clear invalid sidecars - we just lazily ignore them in the lower getter. */
struct PAS_ALIGNED(DELUGE_WORD_SIZE) deluge_ptr {
    /* The sidecar is optional. If it doesn't match the capability, we ignore it. In some cases, we just
       store 0 in it. If the ptr = lower, then the sidecar is totally ignored. */
    pas_uint128 sidecar;

    /* The capabiltiy is mandatory. It stores the ptr itself, the upper bound, the type, and some data
       that's useful for inferring more accurate bounds. */
    pas_uint128 capability;
};

enum deluge_capability_kind {
    /* The ptr is exactly at lower, so ignore the sidecar. This avoids capability widening across
       allocation boundaries. */
    deluge_capability_at_lower,

    /* The ptr is inside an array allocation. If it's in the flex array allocation, the type will
       be the trailing array type. */
    deluge_capability_in_array,

    /* The ptr is inside the base of a flex. The capability's upper refers to the upper bounds of
       the flex base. This has a special sidecar (the flex_upper sidecar). */
    deluge_capability_flex_base,

    /* The ptr is out of bounds. The capability does not have an upper or type. This has a special
       sidecar (the oob_capability sidecar). */
    deluge_capability_oob,
};

typedef enum deluge_capability_kind deluge_capability_kind;

enum deluge_sidecar_kind {
    /* Normal sidecar, which tells the lower bounds. */
    deluge_sidecar_lower,

    /* Sidecar used for flex_base capabilities, which tells the true upper bounds of the flex
       allocation. */
    deluge_sidecar_flex_upper,

    /* Sidecar used for oob capabilities, which contains the full capability but not the pointer.
       I.e. the sidecar has lower, upper, and type. In this case, the sidecar's lower/upper/type
       are combined with the capability's ptr. If that fails, then the ptr becomes a boxed int. */
    deluge_sidecar_oob_capability
};

typedef enum deluge_sidecar_kind deluge_sidecar_kind;

/* Zero-word types are unique; they are only equal by pointer equality. They may or may not have
   a size. If they have a size, they must have an alignment, and they may be allocated. Zero-word
   types have a pas_heap_runtime_config* instead of a trailing_array.
   
   Non-zero-word types are equal if they are structurally the same. They must have a size that
   matches num_words, as in: (size + 15) / 16 == num_words. They must have an alignment. And, they
   have a trailing_array instead of a runtime_config. */
struct deluge_type {
    unsigned index;
    size_t size;
    size_t alignment;
    size_t num_words;
    union {
        const deluge_type* trailing_array;
        pas_basic_heap_runtime_config* runtime_config;
    } u;
    deluge_word_type word_types[];
};

/* Used to describe a non-opaque type to the runtime. For this to be useful, you need to pass it to
   deluge_get_type().

   There is no need for the layout of this to match deluge_type. It cannot completely match it,
   since type has extra fields that this doesn't need to have. */
struct deluge_type_template {
    size_t size;
    size_t alignment;
    const deluge_type_template* trailing_array;
    deluge_word_type word_types[];
};

struct deluge_origin {
    const char* function;
    const char* filename;
    unsigned line;
    unsigned column;
};

struct deluge_initialization_entry {
    pas_uint128* deluded_gptr;
    pas_uint128 ptr_capability;
};

struct deluge_global_initialization_context {
    size_t ref_count;
    pas_ptr_hash_set seen;
    deluge_initialization_entry* entries;
    size_t num_entries;
    size_t entries_capacity;
};

enum deluge_constant_kind {
    /* The target is the actual function. */
    deluge_function_constant,

    /* The target is a getter that returns a pointer to the global. */
    deluge_global_constant,

    /* The target is a constexpr node. */
    deluge_expr_constant
};

typedef enum deluge_constant_kind deluge_constant_kind;

enum deluge_constexpr_opcode {
    deluge_constexpr_add_ptr_immediate
};

typedef enum deluge_constexpr_opcode deluge_constexpr_opcode;

struct deluge_constexpr_node {
    deluge_constexpr_opcode opcode;

    /* This will eventually be an operand union, I guess? */
    deluge_constant_kind left_kind;
    void* left_target;
    uintptr_t right_value;
};

struct deluge_constant_relocation {
    size_t offset;
    deluge_constant_kind kind;
    void* target;
};

struct deluge_alloca_stack {
    void** array;
    size_t size;
    size_t capacity;
};

#define DELUGE_ALLOCA_STACK_INITIALIZER { \
        .array = NULL, \
        .size = 0, \
        .capacity = 0 \
    }

/* This exist only so that compiler-generated templates that need the int type for trailing arrays can
   refer to it. */
extern const deluge_type_template deluge_int_type_template;

extern const deluge_type deluge_int_type;
extern const deluge_type deluge_one_ptr_type;
extern const deluge_type deluge_int_ptr_type;
extern const deluge_type deluge_function_type;
extern const deluge_type deluge_type_type;

extern const deluge_type** deluge_type_array;
extern unsigned deluge_type_array_size;
extern unsigned deluge_type_array_capacity;

#define DELUGE_INVALID_TYPE_INDEX              0u
#define DELUGE_INT_TYPE_INDEX                  1u
#define DELUGE_ONE_PTR_TYPE_INDEX              2u
#define DELUGE_INT_PTR_TYPE_INDEX              3u
#define DELUGE_FUNCTION_TYPE_INDEX             4u
#define DELUGE_TYPE_TYPE_INDEX                 5u
#define DELUGE_MUSL_PASSWD_TYPE_INDEX          6u
#define DELUGE_MUSL_SIGACTION_TYPE_INDEX       7u
#define DELUGE_THREAD_TYPE_INDEX               8u
#define DELUGE_MUSL_ADDRINFO_TYPE_INDEX        9u
#define DELUGE_TYPE_ARRAY_INITIAL_SIZE         10u
#define DELUGE_TYPE_ARRAY_INITIAL_CAPACITY     100u
#define DELUGE_TYPE_MAX_INDEX                  0x3fffffffu
#define DELUGE_TYPE_INDEX_MASK                 0x3fffffffu
#define DELUGE_TYPE_ARRAY_MAX_SIZE             0x40000000u

extern pas_lock_free_read_ptr_ptr_hashtable deluge_fast_type_table;
extern pas_lock_free_read_ptr_ptr_hashtable deluge_fast_heap_table;
extern pas_lock_free_read_ptr_ptr_hashtable deluge_fast_hard_heap_table;

PAS_DECLARE_LOCK(deluge_type);
PAS_DECLARE_LOCK(deluge_type_ops);
PAS_DECLARE_LOCK(deluge_global_initialization);

void deluge_panic(const deluge_origin* origin, const char* format, ...);

#define DELUGE_CHECK(exp, origin, ...) do {     \
        if ((exp)) \
            break; \
        deluge_panic(origin, __VA_ARGS__); \
    } while (0)

/* Ideally, all DELUGE_ASSERTs would be turned into DELUGE_CHECKs.
 
   Also, some DELUGE_ASSERTs are asserting things that cannot happen unless the deluge runtime or
   compiler are broken or memory safey was violated some other way; it would be great to better
   distinguish those. Most of them aren't DELUGE_TESTING_ASSERTs. */
#define DELUGE_ASSERT(exp, origin) do { \
        if ((exp)) \
            break; \
        deluge_panic( \
            origin, \
            "%s:%d: %s: safety assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

#define DELUGE_TESTING_ASSERT(exp, origin) do { \
        if (!PAS_ENABLE_TESTING) \
            break; \
        if ((exp)) \
            break; \
        deluge_panic( \
            origin, "%s:%d: %s: testing assertion %s failed.", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

#define DELUDED_SIGNATURE \
    void* deluded_arg_ptr, void* deluded_arg_upper, const deluge_type* deluded_arg_type, \
    void* deluded_ret_ptr, void* deluded_ret_upper, const deluge_type *deluded_ret_type

#define DELUDED_ARGS \
    deluge_ptr_forge(deluded_arg_ptr, deluded_arg_ptr, deluded_arg_upper, deluded_arg_type)
#define DELUDED_RETS \
    deluge_ptr_forge(deluded_ret_ptr, deluded_ret_ptr, deluded_ret_upper, deluded_ret_type)

#define DELUDED_DELETE_ARGS() do { \
        deluge_deallocate(deluded_arg_ptr); \
        PAS_UNUSED_PARAM(deluded_arg_upper); \
        PAS_UNUSED_PARAM(deluded_arg_type); \
        PAS_UNUSED_PARAM(deluded_ret_ptr); \
        PAS_UNUSED_PARAM(deluded_ret_upper); \
        PAS_UNUSED_PARAM(deluded_ret_type); \
    } while (false)

/* Must be called from CRT before any Deluge happens. If we ever allow Deluge dylibs to be loaded 
   into non-Deluge code, then we'll have to call it from compiler-generated initializers, too. It's
   fine to call this more than once. */
PAS_API void deluge_initialize(void);

void deluge_origin_dump(const deluge_origin* origin, pas_stream* stream);

static inline const deluge_type* deluge_type_lookup(unsigned index)
{
    PAS_TESTING_ASSERT(index < deluge_type_array_size);
    return deluge_type_array[index];
}

static inline size_t deluge_type_template_num_words(const deluge_type_template* type)
{
    return pas_round_up_to_power_of_2(type->size, DELUGE_WORD_SIZE) / DELUGE_WORD_SIZE;
}

PAS_API void deluge_type_template_dump(const deluge_type_template* type, pas_stream* stream);

/* Hash-conses the type template to give you a type instance. If that exact type_template ptr has been
   used for deluge_get_type before, then this is lock-free. Otherwise, it's still O(1) but may need
   some locks. You're expected to never free the type_template. */
PAS_API const deluge_type* deluge_get_type(const deluge_type_template* type_template);

/* Run assertions on the ptr itself. The runtime isn't guaranteed to ever run this check. Pointers
   are expected to be valid by construction. This asserts properties that are going to be true
   even for user-forged pointers using unsafe API, so the only way to break these asserts is if there
   is a bug in deluge itself (compiler or runtime), or by unsafely forging a pointer and then using
   that to corrupt the bits of a pointer.
   
   One example invariant: lower/upper must be aligned on the type's required alignment.
   Another example invariant: !((upper - lower) % type->size)
   
   There may be others.
   
   This does not check if the pointer is in bounds or that it's pointing at something that has any
   particular type. This isn't the actual Deluge check that the compiler uses to achieve memory
   safety! */
void deluge_validate_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                              const deluge_origin* origin);

static inline void deluge_validate_ptr(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_validate_ptr_impl(ptr.sidecar, ptr.capability, origin);
}

static inline void deluge_testing_validate_ptr(deluge_ptr ptr)
{
    if (PAS_ENABLE_TESTING)
        deluge_validate_ptr(ptr, NULL);
}

static inline unsigned deluge_ptr_capability_type_index(deluge_ptr ptr)
{
    return (unsigned)(ptr.capability >> (pas_uint128)96) & DELUGE_TYPE_INDEX_MASK;
}

static inline deluge_capability_kind deluge_ptr_capability_kind(deluge_ptr ptr)
{
    return (deluge_capability_kind)(ptr.capability >> (pas_uint128)126);
}

static inline bool deluge_ptr_is_boxed_int(deluge_ptr ptr)
{
    return !deluge_ptr_capability_type_index(ptr)
        && deluge_ptr_capability_kind(ptr) != deluge_capability_oob;
}

static inline bool deluge_ptr_capability_has_things(deluge_ptr ptr)
{
    return deluge_ptr_capability_type_index(ptr);
}

static inline void* deluge_ptr_ptr(deluge_ptr ptr)
{
    if (deluge_ptr_is_boxed_int(ptr))
        return (void*)(uintptr_t)ptr.capability;
    return (void*)((uintptr_t)ptr.capability & PAS_ADDRESS_MASK);
}

void* deluge_ptr_ptr_impl(pas_uint128 sidecar, pas_uint128 capability);

static inline void* deluge_ptr_capability_upper(deluge_ptr ptr)
{
    PAS_TESTING_ASSERT(deluge_ptr_capability_has_things(ptr));
    return (void*)((uintptr_t)(ptr.capability >> (pas_uint128)48) & PAS_ADDRESS_MASK);
}

/* Not meant to be called directly; this is here so that we can implement deluge_ptr_type. */
static inline const deluge_type* deluge_ptr_capability_type(deluge_ptr ptr)
{
    return deluge_type_lookup(deluge_ptr_capability_type_index(ptr));
}

/* Sidecar methods aren't meant to be called directly; they're here so that we can implement
   deluge_ptr_lower. */
static inline void* deluge_ptr_sidecar_ptr_or_upper(deluge_ptr ptr)
{
    return (void*)((uintptr_t)ptr.sidecar & PAS_ADDRESS_MASK);
}

static inline void* deluge_ptr_sidecar_lower_or_upper(deluge_ptr ptr)
{
    return (void*)((uintptr_t)(ptr.sidecar >> (pas_uint128)48) & PAS_ADDRESS_MASK);
}

static inline unsigned deluge_ptr_sidecar_type_index(deluge_ptr ptr)
{
    return (unsigned)(ptr.sidecar >> (pas_uint128)96) & DELUGE_TYPE_INDEX_MASK;
}

static inline bool deluge_ptr_sidecar_is_blank(deluge_ptr ptr)
{
    return !deluge_ptr_sidecar_type_index(ptr);
}

static inline deluge_sidecar_kind deluge_ptr_sidecar_kind(deluge_ptr ptr)
{
    return (deluge_sidecar_kind)(ptr.sidecar >> (pas_uint128)126);
}

static inline void* deluge_ptr_sidecar_ptr(deluge_ptr ptr)
{
    PAS_TESTING_ASSERT(deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_lower
                       || deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_flex_upper);
    return deluge_ptr_sidecar_ptr_or_upper(ptr);
}

static inline void* deluge_ptr_sidecar_lower(deluge_ptr ptr)
{
    PAS_TESTING_ASSERT(deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_lower
                       || deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_oob_capability);
    return deluge_ptr_sidecar_lower_or_upper(ptr);
}

static inline void* deluge_ptr_sidecar_upper(deluge_ptr ptr)
{
    PAS_TESTING_ASSERT(deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_flex_upper
                       || deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_oob_capability);
    if (deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_oob_capability)
        return deluge_ptr_sidecar_ptr_or_upper(ptr);
    return deluge_ptr_sidecar_lower_or_upper(ptr);
}

static inline const deluge_type* deluge_ptr_sidecar_type(deluge_ptr ptr)
{
    return deluge_type_lookup(deluge_ptr_sidecar_type_index(ptr));
}

static inline bool deluge_ptr_sidecar_is_relevant(deluge_ptr ptr)
{
    const deluge_type* capability_type;
    const deluge_type* sidecar_type;
    if (deluge_ptr_is_boxed_int(ptr))
        return false;
    switch (deluge_ptr_capability_kind(ptr)) {
    case deluge_capability_at_lower:
        return false;
    case deluge_capability_in_array:
        if (deluge_ptr_sidecar_kind(ptr) != deluge_sidecar_lower)
            return false;
        break;
    case deluge_capability_flex_base:
        if (deluge_ptr_sidecar_kind(ptr) != deluge_sidecar_flex_upper)
            return false;
        break;
    case deluge_capability_oob:
        return deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_oob_capability;
    }
    if (deluge_ptr_sidecar_ptr(ptr) != deluge_ptr_ptr(ptr))
        return false;
    capability_type = deluge_ptr_capability_type(ptr);
    sidecar_type = deluge_ptr_sidecar_type(ptr);
    if (sidecar_type == capability_type)
        return true;
    PAS_TESTING_ASSERT(deluge_ptr_capability_kind(ptr) != deluge_capability_flex_base);
    PAS_TESTING_ASSERT(!deluge_ptr_sidecar_is_blank(ptr));
    if (!sidecar_type->num_words)
        return false;
    return sidecar_type->u.trailing_array == capability_type;
}

static inline const deluge_type* deluge_ptr_type(deluge_ptr ptr)
{
    if (deluge_ptr_sidecar_is_relevant(ptr))
        return deluge_ptr_sidecar_type(ptr);
    return deluge_ptr_capability_type(ptr);
}

static inline void* deluge_ptr_upper(deluge_ptr ptr)
{
    if (deluge_ptr_is_boxed_int(ptr))
        return NULL;

    switch (deluge_ptr_capability_kind(ptr)) {
    case deluge_capability_at_lower:
    case deluge_capability_in_array:
        return deluge_ptr_capability_upper(ptr);

    case deluge_capability_flex_base:
        if (!deluge_ptr_sidecar_is_relevant(ptr))
            return deluge_ptr_capability_upper(ptr);
        
        return deluge_ptr_sidecar_upper(ptr);

    case deluge_capability_oob:
        if (deluge_ptr_sidecar_kind(ptr) == deluge_sidecar_oob_capability)
            return deluge_ptr_sidecar_upper(ptr);
        
        return NULL;
    }

    PAS_ASSERT(!"Should not be reached");
    return NULL;
}

static inline void* deluge_ptr_lower(deluge_ptr ptr)
{
    static const bool verbose = false;
    
    uintptr_t distance_to_upper;
    uintptr_t distance_to_upper_of_element;

    if (deluge_ptr_is_boxed_int(ptr))
        return NULL;

    PAS_TESTING_ASSERT(deluge_ptr_capability_type(ptr)
                       || deluge_ptr_capability_kind(ptr) == deluge_capability_oob);

    if (verbose)
        pas_log("capability kind = %u\n", (unsigned)deluge_ptr_capability_kind(ptr));

    if (deluge_ptr_capability_kind(ptr) == deluge_capability_at_lower)
        return deluge_ptr_ptr(ptr);

    if (deluge_ptr_sidecar_is_relevant(ptr)
        && deluge_ptr_capability_kind(ptr) != deluge_capability_flex_base) {
        PAS_TESTING_ASSERT(deluge_ptr_sidecar_lower(ptr) <= deluge_ptr_upper(ptr));
        return deluge_ptr_sidecar_lower(ptr);
    }

    if (deluge_ptr_capability_kind(ptr) == deluge_capability_oob)
        return NULL;

    PAS_TESTING_ASSERT(deluge_ptr_ptr(ptr) <= deluge_ptr_upper(ptr));
    if (deluge_ptr_ptr(ptr) >= deluge_ptr_upper(ptr))
        return deluge_ptr_upper(ptr);

    distance_to_upper = (char*)deluge_ptr_capability_upper(ptr) - (char*)deluge_ptr_ptr(ptr);
    PAS_TESTING_ASSERT(deluge_ptr_capability_kind(ptr) == deluge_capability_in_array
                       || deluge_ptr_capability_kind(ptr) == deluge_capability_flex_base);
    if (deluge_ptr_capability_kind(ptr) == deluge_capability_flex_base) {
        PAS_TESTING_ASSERT(distance_to_upper);
        PAS_TESTING_ASSERT(distance_to_upper < deluge_ptr_capability_type(ptr)->size);
    }
    distance_to_upper_of_element = distance_to_upper % deluge_ptr_type(ptr)->size;
    if (!distance_to_upper_of_element)
        return deluge_ptr_ptr(ptr);
    
    return (char*)deluge_ptr_ptr(ptr)
        + distance_to_upper_of_element - deluge_ptr_capability_type(ptr)->size;
}

static inline uintptr_t deluge_ptr_offset(deluge_ptr ptr)
{
    return (char*)deluge_ptr_ptr(ptr) - (char*)deluge_ptr_lower(ptr);
}

static inline uintptr_t deluge_ptr_available(deluge_ptr ptr)
{
    return (char*)deluge_ptr_upper(ptr) - (char*)deluge_ptr_ptr(ptr);
}

static inline deluge_ptr deluge_ptr_create(pas_uint128 sidecar, pas_uint128 capability)
{
    deluge_ptr result;
    result.sidecar = sidecar;
    result.capability = capability;
    deluge_testing_validate_ptr(result);
    return result;
}

PAS_API deluge_ptr deluge_ptr_forge(void* ptr, void* lower, void* upper, const deluge_type* type);

static inline deluge_ptr deluge_ptr_forge_with_size(void* ptr, size_t size, const deluge_type* type)
{
    return deluge_ptr_forge(ptr, ptr, (char*)ptr + size, type);
}

static inline deluge_ptr deluge_ptr_forge_byte(void* ptr, const deluge_type* type)
{
    return deluge_ptr_forge_with_size(ptr, 1, type);
}

static inline deluge_ptr deluge_ptr_forge_invalid(void* ptr)
{
    return deluge_ptr_forge(ptr, NULL, NULL, NULL);
}

static inline deluge_ptr deluge_ptr_with_ptr(deluge_ptr ptr, void* new_ptr)
{
    return deluge_ptr_forge(
        new_ptr, deluge_ptr_lower(ptr), deluge_ptr_upper(ptr), deluge_ptr_type(ptr));
}

static inline deluge_ptr deluge_ptr_with_offset(deluge_ptr ptr, uintptr_t offset)
{
    return deluge_ptr_with_ptr(ptr, (char*)deluge_ptr_ptr(ptr) + offset);
}

static inline bool deluge_ptr_is_totally_equal(deluge_ptr a, deluge_ptr b)
{
    return a.sidecar == b.sidecar
        && a.capability == b.capability;
}

static inline bool deluge_ptr_is_totally_null(deluge_ptr ptr)
{
    return deluge_ptr_is_totally_equal(ptr, deluge_ptr_forge_invalid(NULL));
}

static inline deluge_ptr deluge_ptr_load(deluge_ptr* ptr)
{
    deluge_ptr result;
    result.sidecar = __c11_atomic_load((_Atomic pas_uint128*)&ptr->sidecar, __ATOMIC_RELAXED);
    result.capability = __c11_atomic_load((_Atomic pas_uint128*)&ptr->capability, __ATOMIC_RELAXED);
    return result;
}

static inline void deluge_ptr_store(deluge_ptr* ptr, deluge_ptr value)
{
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, value.sidecar, __ATOMIC_RELAXED);
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->capability, value.capability, __ATOMIC_RELAXED);
}

static inline bool deluge_ptr_unfenced_weak_cas(
    deluge_ptr* ptr, deluge_ptr expected, deluge_ptr new_value)
{
    /* This is optional; it's legal to do it or not do it, from the standpoint of Deluge soundness.
       If whoever reads the ptr sees a sidecar that doesn't match the capability, then it'll be
       rejected anyway. Nuking the sidecar turns a rarely-occurring bad case into a deterministic
       bad case, so code has to always deal with it. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

static inline deluge_ptr deluge_ptr_unfenced_strong_cas(
    deluge_ptr* ptr, deluge_ptr expected, deluge_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
    return deluge_ptr_create(0, expected.capability);
}

static inline bool deluge_ptr_weak_cas(deluge_ptr* ptr, deluge_ptr expected, deluge_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return __c11_atomic_compare_exchange_weak(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline deluge_ptr deluge_ptr_strong_cas(deluge_ptr* ptr, deluge_ptr expected, deluge_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    __c11_atomic_compare_exchange_strong(
        (_Atomic pas_uint128*)&ptr->capability, &expected.capability, new_value.capability,
        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return deluge_ptr_create(0, expected.capability);
}

static inline deluge_ptr deluge_ptr_unfenced_xchg(deluge_ptr* ptr, deluge_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return deluge_ptr_create(
        0,
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->capability, new_value.capability, __ATOMIC_RELAXED));
}

static inline deluge_ptr deluge_ptr_xchg(deluge_ptr* ptr, deluge_ptr new_value)
{
    /* See above. */
    __c11_atomic_store((_Atomic pas_uint128*)&ptr->sidecar, 0, __ATOMIC_RELAXED);
    return deluge_ptr_create(
        0,
        __c11_atomic_exchange(
            (_Atomic pas_uint128*)&ptr->capability, new_value.capability, __ATOMIC_SEQ_CST));
}

PAS_API void deluge_ptr_dump(deluge_ptr ptr, pas_stream* stream);
PAS_API char* deluge_ptr_to_new_string(deluge_ptr ptr); /* WARNING: this is different from
                                                           zptr_to_new_string. This uses the
                                                           utility heap - WHICH MUST NEVER LEAK TO
                                                           DELUDED CODE - whereas the other one
                                                           uses the int heap. */

/* Returns true if this type has no trailing array and is not opaque. */
static inline bool deluge_type_is_normal(const deluge_type* type)
{
    return type->num_words && !type->u.trailing_array;
}

static inline const deluge_type* deluge_type_get_trailing_array(const deluge_type* type)
{
    if (type->num_words)
        return type->u.trailing_array;
    return NULL;
}

static inline deluge_word_type deluge_type_get_word_type(const deluge_type* type,
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
void deluge_validate_type(const deluge_type* type, const deluge_origin* origin);

static inline bool deluge_type_is_equal(const deluge_type* a, const deluge_type* b)
{
    return a == b;
}

static inline unsigned deluge_type_hash(const deluge_type* type)
{
    return pas_hash_ptr(type);
}

PAS_API bool deluge_type_template_is_equal(const deluge_type_template* a,
                                           const deluge_type_template* b);
PAS_API unsigned deluge_type_template_hash(const deluge_type_template* type);

static inline size_t deluge_type_representation_size(const deluge_type* type)
{
    return PAS_OFFSETOF(deluge_type, word_types)
        + sizeof(deluge_word_type) * type->num_words;
}
    
static inline size_t deluge_type_as_heap_type_get_type_size(const pas_heap_type* heap_type)
{
    const deluge_type* type = (const deluge_type*)heap_type;
    PAS_TESTING_ASSERT(type->size);
    PAS_TESTING_ASSERT(type->alignment);
    return pas_round_up_to_power_of_2(type->size, type->alignment);
}

static inline size_t deluge_type_as_heap_type_get_type_alignment(const pas_heap_type* heap_type)
{
    const deluge_type* type = (const deluge_type*)heap_type;
    PAS_TESTING_ASSERT(type->size);
    PAS_TESTING_ASSERT(type->alignment);
    return type->alignment;
}

PAS_API pas_heap_runtime_config* deluge_type_as_heap_type_get_runtime_config(
    const pas_heap_type* type, pas_heap_runtime_config* config);
PAS_API pas_heap_runtime_config* deluge_type_as_heap_type_assert_default_runtime_config(
    const pas_heap_type* type, pas_heap_runtime_config* config);

PAS_API void deluge_word_type_dump(deluge_word_type type, pas_stream* stream);
PAS_API char* deluge_word_type_to_new_string(deluge_word_type type);

PAS_API void deluge_type_dump(const deluge_type* type, pas_stream* stream);
PAS_API void deluge_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream);

char* deluge_type_to_new_string(const deluge_type* type); /* WARNING: this is different from
                                                             ztype_to_new_string. This uses the
                                                             utility heap - WHICH MUST NEVER LEAK TO
                                                             DELUDED CODE - whereas the other one
                                                             uses the int heap. */

PAS_API const deluge_type* deluge_type_slice(const deluge_type* type, pas_range range,
                                             const deluge_origin* origin);
PAS_API const deluge_type* deluge_type_cat(const deluge_type* a, size_t a_size,
                                           const deluge_type* b, size_t b_size,
                                           const deluge_origin* origin);

/* This is basically va_arg, but it doesn't check that the type matches. That's fine if the consumer
   of the pointer is code compiled by Deluge, since that will check on every access. That's not fine if
   the consumer is someone writing legacy C code against some Deluge flight API. */
static inline deluge_ptr deluge_ptr_get_next_bytes(
    deluge_ptr* ptr, size_t size, size_t alignment)
{
    deluge_ptr ptr_value;
    uintptr_t ptr_as_int;
    deluge_ptr result;

    ptr_value = deluge_ptr_load(ptr);
    ptr_as_int = (uintptr_t)deluge_ptr_ptr(ptr_value);
    ptr_as_int = pas_round_up_to_power_of_2(ptr_as_int, alignment);

    result = deluge_ptr_with_ptr(ptr_value, (void*)ptr_as_int);

    deluge_ptr_store(ptr, deluge_ptr_with_ptr(ptr_value, (char*)ptr_as_int + size));

    return result;
}

/* Gives you a heap for the given type. This looks up the heap based on the structural equality
   of the type, so equal types get the same heap.

   It's correct to call this every time you do a memory allocation. It's not the greatest idea, but
   it will be lock-free (and basically fence-free) in the common case.

   The type you pass here must be immortal. The runtime may refer to it forever. (That's not
   strictly necessary so we could change that if we needed to.) */
pas_heap_ref* deluge_get_heap(const deluge_type* type);

void* deluge_allocate_int(size_t size, size_t cout);
void* deluge_allocate_int_with_alignment(size_t size, size_t count, size_t alignment);

void* deluge_allocate_one(pas_heap_ref* ref);
void* deluge_allocate_opaque(pas_heap_ref* ref);
void* deluge_allocate_many(pas_heap_ref* ref, size_t count);
void* deluge_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment);

void* deluge_allocate_int_flex(size_t base_size, size_t element_size, size_t count);
void* deluge_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                              size_t alignment);
void* deluge_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count);
void* deluge_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                          size_t count, size_t alignment);

/* This can allocate any type (ints or not), but it's considerably slower than the other allocation
   entrypoints. The compiler avoids this in most cases. */
void* deluge_allocate_with_type(const deluge_type* type, size_t size);

/* FIXME: It would be great if this had a separate freeing function, and deluge_deallocate asserted
   if it detected that it was used to free a utility allocation.
   
   Right now, we just rely on the fact that zfree (and friends) do a bunch of checks that effectively
   prevent freeing utility allocations. */
void* deluge_allocate_utility(size_t size);

void* deluge_reallocate_int(void* ptr, size_t size, size_t count);
void* deluge_reallocate_int_with_alignment(void* ptr, size_t size, size_t count, size_t alignment);

void* deluge_reallocate(void* ptr, pas_heap_ref* ref, size_t count);

void deluge_deallocate(const void* ptr);
void deluge_deallocate_safe(deluge_ptr ptr);
void deluded_f_zfree(DELUDED_SIGNATURE);
void deluded_f_zgetallocsize(DELUDED_SIGNATURE);

void deluded_f_zcalloc_multiply(DELUDED_SIGNATURE);

pas_heap_ref* deluge_get_hard_heap(const deluge_type* type);

void* deluge_hard_allocate_int(size_t size, size_t count);
void* deluge_hard_allocate_int_with_alignment(size_t size, size_t count, size_t alignment);
void* deluge_hard_allocate_one(pas_heap_ref* ref);
void* deluge_hard_allocate_many(pas_heap_ref* ref, size_t count);
void* deluge_hard_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment);
void* deluge_hard_allocate_int_flex(size_t base_size, size_t element_size, size_t count);
void* deluge_hard_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                                   size_t alignment);
void* deluge_hard_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count);
void* deluge_hard_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                               size_t count, size_t alignment);
void* deluge_hard_reallocate_int(void* ptr, size_t size, size_t count);
void* deluge_hard_reallocate_int_with_alignment(void* ptr, size_t size, size_t count,
                                                size_t alignment);

void* deluge_hard_reallocate(void* ptr, pas_heap_ref* ref, size_t count);
void deluge_hard_deallocate(void* ptr);

/* This is super gross but it's fine for now. When the compiler allocates, it emits three calls.
   First it calls the allocator. Then it calls new_capability. Then it calls new_sidecar.
   
   That's dumb!
   
   Why do I do it for now?
   
   - I also want to call all of these allocation functions from C code in the runtime, and most of
     the time, that code just wants a void*. So if I wanted allocation functions that returned
     a wide pointer, or even just the pointer's capability, then I'd have to have duplicate
     entrypoints. I don't feel like writing that code right now, but I'll almost certainly do it
     eventually.
   
   - It's super annoying to have code that returns a deluge_ptr called from compiler-generated code,
     since at the point I'm at in LLVM, the returns for structs may (or may not!) have been lowered
     to some kind of pointer-passing convention. I can make LLVM return a struct like deluge_ptr in
     registers, but this is C code, and so these functions will have C's calling convention, which is
     different from LLVM's when it comes to struct return! There are many ways around this,
     especially since I control the compiler (I am not above having a separate hacked clang just for
     compiling libdeluge, or rewriting all of this in assembly, or in LLVM IR), but I don't feel
     like doing that right now.
   
   Therfore, I have functions to build the capability and sidecar separately. Gross but at least it
   gets me there.

   Also, it's not actually necessary to know the size to construct the sidecar, but whatever, doing
   it this way makes it more obviously correct. */
pas_uint128 deluge_new_capability(void* ptr, size_t size, const deluge_type* type);
pas_uint128 deluge_new_sidecar(void* ptr, size_t size, const deluge_type* type);

void deluge_log_allocation(deluge_ptr ptr, const deluge_origin* origin);
void deluge_log_allocation_impl(pas_uint128 sidecar, pas_uint128 capability,
                                const deluge_origin* origin);

void deluge_check_forge(
    void* ptr, size_t size, size_t count, const deluge_type* type, const deluge_origin* origin);

void deluded_f_zhard_free(DELUDED_SIGNATURE);
void deluded_f_zhard_getallocsize(DELUDED_SIGNATURE);

void deluded_f_zgetlower(DELUDED_SIGNATURE);
void deluded_f_zgetupper(DELUDED_SIGNATURE);
void deluded_f_zgettype(DELUDED_SIGNATURE);

void deluded_f_zslicetype(DELUDED_SIGNATURE);
void deluded_f_zgettypeslice(DELUDED_SIGNATURE);
void deluded_f_zcattype(DELUDED_SIGNATURE);
void deluded_f_zalloc_with_type(DELUDED_SIGNATURE);

void deluded_f_ztype_to_new_string(DELUDED_SIGNATURE);
void deluded_f_zptr_to_new_string(DELUDED_SIGNATURE);

/* The compiler uses these for now because it makes LLVM codegen ridiculously easy to get right,
   but it's totally the wrong way to do it from a perf standpoint. */
pas_uint128 deluge_update_sidecar(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr);
pas_uint128 deluge_update_capability(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr);

void deluge_check_deallocate_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const deluge_origin* origin);
static inline void deluge_check_deallocate(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_check_deallocate_impl(ptr.sidecar, ptr.capability, origin);
}

void deluge_check_access_int_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  uintptr_t bytes, const deluge_origin* origin);
void deluge_check_access_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const deluge_origin* origin);

static inline void deluge_check_access_int(deluge_ptr ptr, uintptr_t bytes, const deluge_origin* origin)
{
    deluge_check_access_int_impl(ptr.sidecar, ptr.capability, bytes, origin);
}
static inline void deluge_check_access_ptr(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_check_access_ptr_impl(ptr.sidecar, ptr.capability, origin);
}

void deluge_check_function_call_impl(pas_uint128 sidecar, pas_uint128 capability,
                                     const deluge_origin* origin);

static inline void deluge_check_function_call(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_check_function_call_impl(ptr.sidecar, ptr.capability, origin);
}

void deluge_check_access_opaque(
    deluge_ptr ptr, const deluge_type* expected_type, const deluge_origin* origin);

/* You can call these if you know that you're copying pointers (or possible pointers) and you've
   already proved that it's safe and the pointer/size are aligned.
   
   These also happen to be used as the implementation of deluge_memset_impl/deluge_memcpy_impl/
   deluge_memmove_impl in those cases where pointer copying is detected. Those also do all the
   checks needed to ensure memory safety. So, usually, you want those, not these.

   The number of bytes must be a multiple of 16. */
void deluge_low_level_ptr_safe_bzero(void* ptr, size_t bytes);
void deluge_low_level_ptr_safe_memcpy(void* dst, void* src, size_t bytes);
void deluge_low_level_ptr_safe_memmove(void* dst, void* src, size_t bytes);

void deluge_memset_impl(pas_uint128 sidecar, pas_uint128 capability,
                        unsigned value, size_t count, const deluge_origin* origin);
void deluge_memcpy_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                        pas_uint128 src_sidecar, pas_uint128 src_capability,
                        size_t count, const deluge_origin* origin);
void deluge_memmove_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                         pas_uint128 src_sidecar, pas_uint128 src_capability,
                         size_t count, const deluge_origin* origin);

static inline void deluge_memset(deluge_ptr ptr, unsigned value, size_t count, const deluge_origin* origin)
{
    deluge_memset_impl(ptr.sidecar, ptr.capability, value, count, origin);
}

static inline void deluge_memcpy(deluge_ptr dst, deluge_ptr src, size_t count, const deluge_origin* origin)
{
    deluge_memcpy_impl(dst.sidecar, dst.capability, src.sidecar, src.capability, count, origin);
}

static inline void deluge_memmove(deluge_ptr dst, deluge_ptr src, size_t count, const deluge_origin* origin)
{
    deluge_memmove_impl(dst.sidecar, dst.capability, src.sidecar, src.capability, count, origin);
}

/* Correct uses of this should always pass new_upper = ptr + K * new_type->size. This function does
   not have to check that you did that. This property is ensured by the compiler and the fact that
   the user-visible API (zrestrict) takes a count. */
void deluge_check_restrict(pas_uint128 sidecar, pas_uint128 capability,
                           void* new_upper, const deluge_type* new_type, const deluge_origin* origin);

static inline deluge_ptr deluge_restrict(deluge_ptr ptr, size_t count, const deluge_type* new_type,
                                         const deluge_origin* origin)
{
    void* new_upper;
    new_upper = (char*)deluge_ptr_ptr(ptr) + count * new_type->size;
    deluge_check_restrict(ptr.sidecar, ptr.capability, new_upper, new_type, origin);
    return deluge_ptr_forge(deluge_ptr_ptr(ptr), deluge_ptr_ptr(ptr), new_upper, new_type);
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
const char* deluge_check_and_get_new_str(deluge_ptr ptr, const deluge_origin* origin);

const char* deluge_check_and_get_new_str_or_null(deluge_ptr ptr, const deluge_origin* origin);

deluge_ptr deluge_strdup(const char* str);

/* This is basically va_arg. Whatever kind of API we expose to native C code to interact with Deluge
   code will have to use this kind of API to parse the flights. */
static inline deluge_ptr deluge_ptr_get_next(
    deluge_ptr* ptr, size_t count, size_t alignment, const deluge_type* type,
    const deluge_origin* origin)
{
    return deluge_restrict(deluge_ptr_get_next_bytes(
                               ptr, count * type->size, alignment),
                           count, type, origin);
}

/* NOTE: It's tempting to add a macro that takes a type and does get_next, but I don't see how
   that would handle pointers correctly. */

static inline deluge_ptr deluge_ptr_get_next_ptr(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(deluge_ptr), alignof(deluge_ptr));
    deluge_check_access_ptr(slot_ptr, origin);
    return deluge_ptr_load((deluge_ptr*)deluge_ptr_ptr(slot_ptr));
}

static inline int deluge_ptr_get_next_int(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(int), alignof(int));
    deluge_check_access_int(slot_ptr, sizeof(int), origin);
    return *(int*)deluge_ptr_ptr(slot_ptr);
}

static inline unsigned deluge_ptr_get_next_unsigned(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(unsigned), alignof(unsigned));
    deluge_check_access_int(slot_ptr, sizeof(unsigned), origin);
    return *(unsigned*)deluge_ptr_ptr(slot_ptr);
}

static inline long deluge_ptr_get_next_long(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(long), alignof(long));
    deluge_check_access_int(slot_ptr, sizeof(long), origin);
    return *(long*)deluge_ptr_ptr(slot_ptr);
}

static inline unsigned long deluge_ptr_get_next_unsigned_long(deluge_ptr* ptr,
                                                              const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(unsigned long), alignof(unsigned long));
    deluge_check_access_int(slot_ptr, sizeof(unsigned long), origin);
    return *(unsigned long*)deluge_ptr_ptr(slot_ptr);
}

static inline size_t deluge_ptr_get_next_size_t(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(size_t), alignof(size_t));
    deluge_check_access_int(slot_ptr, sizeof(size_t), origin);
    return *(size_t*)deluge_ptr_ptr(slot_ptr);
}

static inline double deluge_ptr_get_next_double(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(double), alignof(double));
    deluge_check_access_int(slot_ptr, sizeof(double), origin);
    return *(double*)deluge_ptr_ptr(slot_ptr);
}

/* Given a va_list ptr (so a ptr to a ptr), this:
   
   - checks that it is indeed a ptr to a ptr
   - uses that ptr to va_arg according to the deluge_ptr_get_next_bytes protocol
   - checks that the resulting ptr can be restricted to count*type
   - if all that goes well, returns a ptr you can load/store from safely according to that type */
void* deluge_va_arg_impl(
    pas_uint128 va_list_sidecar, pas_uint128 va_list_capability,
    size_t count, size_t alignment, const deluge_type* type, const deluge_origin* origin);

/* If parent is not NULL, increases its ref count and returns it. Otherwise, creates a new context. */
deluge_global_initialization_context* deluge_global_initialization_context_create(
    deluge_global_initialization_context* parent);
/* Attempts to add the given global to be initialized.
   
   If it's already in the set then this returns false.
   
   If it's not in the set, then it's added to the set and true is returned. */
bool deluge_global_initialization_context_add(
    deluge_global_initialization_context* context, pas_uint128* deluded_gptr, pas_uint128 ptr_capability);
/* Derefs the context. If the refcount reaches zero, it gets destroyed.
 
   Destroying the set means storing all known ptr_capabilities into their corresponding deluded_gptrs
   atomically. */
void deluge_global_initialization_context_destroy(deluge_global_initialization_context* context);

void deluge_alloca_stack_push(deluge_alloca_stack* stack, void* alloca);
static inline size_t deluge_alloca_stack_save(deluge_alloca_stack* stack) { return stack->size; }
void deluge_alloca_stack_restore(deluge_alloca_stack* stack, size_t size);
void deluge_alloca_stack_destroy(deluge_alloca_stack* stack);

void deluge_execute_constant_relocations(
    void* constant, deluge_constant_relocation* relocations, size_t num_relocations,
    deluge_global_initialization_context* context);

void deluge_defer_or_run_global_ctor(void (*global_ctor)(DELUDED_SIGNATURE));
void deluge_run_deferred_global_ctors(void); /* Important safety property: libc must call this before
                                                letting the user start threads. But it's OK if the
                                                deferred constructors that this calls start threads,
                                                as far as safety goes. */
void deluded_f_zrun_deferred_global_ctors(DELUDED_SIGNATURE);

void deluge_error(const char* reason, const deluge_origin* origin);

void deluded_f_zprint(DELUDED_SIGNATURE);
void deluded_f_zprint_long(DELUDED_SIGNATURE);
void deluded_f_zprint_ptr(DELUDED_SIGNATURE);
void deluded_f_zerror(DELUDED_SIGNATURE);
void deluded_f_zstrlen(DELUDED_SIGNATURE);
void deluded_f_zisdigit(DELUDED_SIGNATURE);

void deluded_f_zfence(DELUDED_SIGNATURE);
void deluded_f_zstore_store_fence(DELUDED_SIGNATURE);
void deluded_f_zcompiler_fence(DELUDED_SIGNATURE);
void deluded_f_zunfenced_weak_cas_ptr(DELUDED_SIGNATURE);
void deluded_f_zweak_cas_ptr(DELUDED_SIGNATURE);
void deluded_f_zunfenced_strong_cas_ptr(DELUDED_SIGNATURE);
void deluded_f_zstrong_cas_ptr(DELUDED_SIGNATURE);
void deluded_f_zunfenced_xchg_ptr(DELUDED_SIGNATURE);
void deluded_f_zxchg_ptr(DELUDED_SIGNATURE);

void deluded_f_zis_runtime_testing_enabled(DELUDED_SIGNATURE);
void deluded_f_zborkedptr(DELUDED_SIGNATURE);
void deluded_f_zvalidate_ptr(DELUDED_SIGNATURE);
void deluded_f_zscavenger_suspend(DELUDED_SIGNATURE);
void deluded_f_zscavenger_resume(DELUDED_SIGNATURE);

/* Amusingly, the order of these functions tell the story of me porting musl to deluge. */
void deluded_f_zregister_sys_errno_handler(DELUDED_SIGNATURE);
void deluded_f_zsys_ioctl(DELUDED_SIGNATURE);
void deluded_f_zsys_writev(DELUDED_SIGNATURE);
void deluded_f_zsys_read(DELUDED_SIGNATURE);
void deluded_f_zsys_readv(DELUDED_SIGNATURE);
void deluded_f_zsys_write(DELUDED_SIGNATURE);
void deluded_f_zsys_close(DELUDED_SIGNATURE);
void deluded_f_zsys_lseek(DELUDED_SIGNATURE);
void deluded_f_zsys_exit(DELUDED_SIGNATURE);
void deluded_f_zsys_signal(DELUDED_SIGNATURE);
void deluded_f_zsys_getuid(DELUDED_SIGNATURE);
void deluded_f_zsys_geteuid(DELUDED_SIGNATURE);
void deluded_f_zsys_getgid(DELUDED_SIGNATURE);
void deluded_f_zsys_getegid(DELUDED_SIGNATURE);
void deluded_f_zsys_open(DELUDED_SIGNATURE);
void deluded_f_zsys_getpid(DELUDED_SIGNATURE);
void deluded_f_zsys_clock_gettime(DELUDED_SIGNATURE);
void deluded_f_zsys_fstatat(DELUDED_SIGNATURE);
void deluded_f_zsys_fstat(DELUDED_SIGNATURE);
void deluded_f_zsys_fcntl(DELUDED_SIGNATURE);
void deluded_f_zsys_getpwuid(DELUDED_SIGNATURE);
void deluded_f_zsys_sigaction(DELUDED_SIGNATURE);
void deluded_f_zsys_isatty(DELUDED_SIGNATURE);
void deluded_f_zsys_pipe(DELUDED_SIGNATURE);
void deluded_f_zsys_select(DELUDED_SIGNATURE);
void deluded_f_zsys_sched_yield(DELUDED_SIGNATURE);
void deluded_f_zsys_socket(DELUDED_SIGNATURE);
void deluded_f_zsys_setsockopt(DELUDED_SIGNATURE);
void deluded_f_zsys_bind(DELUDED_SIGNATURE);
void deluded_f_zsys_getaddrinfo(DELUDED_SIGNATURE);
void deluded_f_zsys_connect(DELUDED_SIGNATURE);
void deluded_f_zsys_getsockname(DELUDED_SIGNATURE);
void deluded_f_zsys_getsockopt(DELUDED_SIGNATURE);
void deluded_f_zsys_getpeername(DELUDED_SIGNATURE);
void deluded_f_zsys_sendto(DELUDED_SIGNATURE);
void deluded_f_zsys_recvfrom(DELUDED_SIGNATURE);
void deluded_f_zsys_getrlimit(DELUDED_SIGNATURE);

void deluded_f_zthread_self(DELUDED_SIGNATURE);
void deluded_f_zthread_get_id(DELUDED_SIGNATURE);
void deluded_f_zthread_get_cookie(DELUDED_SIGNATURE);
void deluded_f_zthread_set_self_cookie(DELUDED_SIGNATURE);
void deluded_f_zthread_create(DELUDED_SIGNATURE);
void deluded_f_zthread_join(DELUDED_SIGNATURE);
void deluded_f_zthread_detach(DELUDED_SIGNATURE);

void deluded_f_zpark_if(DELUDED_SIGNATURE);
void deluded_f_zunpark_one(DELUDED_SIGNATURE);
void deluded_f_zunpark_all(DELUDED_SIGNATURE);

PAS_END_EXTERN_C;

#endif /* DELUGE_RUNTIME_H */

