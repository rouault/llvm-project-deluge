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
     go into optimization mode, I will be able to wreak havoc and take many prisoners. I pity the
     fool who bets against my ability to make this hella fucking fast. I'm just holding back from
     going there, for now. */

struct deluge_alloca_stack;
struct deluge_global_initialization_context;
struct deluge_origin;
struct deluge_ptr;
struct deluge_type;
struct pas_basic_heap_runtime_config;
struct pas_stream;
typedef struct deluge_alloca_stack deluge_alloca_stack;
typedef struct deluge_global_initialization_context deluge_global_initialization_context;
typedef struct deluge_origin deluge_origin;
typedef struct deluge_ptr deluge_ptr;
typedef struct deluge_type deluge_type;
typedef struct pas_basic_heap_runtime_config pas_basic_heap_runtime_config;
typedef struct pas_stream pas_stream;

typedef uint8_t deluge_word_type;

/* Note that any statement like "64-bit word" below needs to be understood with the caveat that ptr
   access also checks bounds. So, a pointer might say it has type "64-bit int" but bounds that say
   "1 byte", in which case you get the intersection: "1 byte int". */
#define DELUGE_WORD_TYPE_OFF_LIMITS        ((uint8_t)0)     /* 64-bit that cannot be accessed. */
#define DELUGE_WORD_TYPE_INT               ((uint8_t)1)     /* 64-bit word that contains ints.
                                                               Primitive ptrs may have bounds that
                                                               are less than 8 bytes and may not have
                                                               8 byte alignment. */
#define DELUGE_WORD_TYPE_PTR_PART1         ((uint8_t)2)     /* 64-bit word that contains the first part
                                                               of a wide ptr (the ptr). */
#define DELUGE_WORD_TYPE_PTR_PART2         ((uint8_t)3)     /* 64-bit word that contains the second
                                                               part of a wide ptr (the lower). */
#define DELUGE_WORD_TYPE_PTR_PART3         ((uint8_t)4)     /* 64-bit word that contains the third part
                                                               of a wide ptr (the upper). */
#define DELUGE_WORD_TYPE_PTR_PART4         ((uint8_t)5)     /* 64-bit word that contains the fourth
                                                               part of a wide ptr (the type). */

struct deluge_ptr {
    void* ptr;
    void* lower;
    void* upper;
    const deluge_type* type;
};

/* Zero-word types are unique; they are only equal by pointer equality. They may or may not have
   a size. If they have a size, they must have an alignment, and they may be allocated. Zero-word
   types have a pas_heap_runtime_config* instead of a trailing_array.
   
   Non-zero-word types are equal if they are structurally the same. They must have a size that
   matches num_words, as in: (size + 7) / 8 == num_words. They must have an alignment. And, they
   have a trailing_array instead of a runtime_config. */
struct deluge_type {
    size_t size;
    size_t alignment;
    size_t num_words;
    union {
        const deluge_type* trailing_array;
        pas_basic_heap_runtime_config* runtime_config;
    } u;
    deluge_word_type word_types[];
};

struct deluge_origin {
    const char* function;
    const char* filename;
    unsigned line;
    unsigned column;
};

struct deluge_global_initialization_context {
    void* global_getter; /* This is a function pointer, but we cannot give it a signature in C. */
    deluge_ptr ptr;
    deluge_global_initialization_context* outer;
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

extern const deluge_type deluge_int_type;
extern const deluge_type deluge_ptr_type;
extern const deluge_type deluge_function_type;
extern const deluge_type deluge_type_type;

extern pas_lock_free_read_ptr_ptr_hashtable deluge_fast_type_table;

PAS_DECLARE_LOCK(deluge_type);
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

static inline deluge_ptr deluge_ptr_forge(void* ptr, void* lower, void* upper, const deluge_type* type)
{
    deluge_ptr result;
    result.ptr = ptr;
    result.lower = lower;
    result.upper = upper;
    result.type = type;
    return result;
}

static inline deluge_ptr deluge_ptr_with_ptr(deluge_ptr ptr, void* new_ptr)
{
    return deluge_ptr_forge(new_ptr, ptr.lower, ptr.upper, ptr.type);
}

static inline deluge_ptr deluge_ptr_with_offset(deluge_ptr ptr, uintptr_t offset)
{
    return deluge_ptr_with_ptr(ptr, (char*)ptr.ptr + offset);
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

bool deluge_type_is_equal(const deluge_type* a, const deluge_type* b);
unsigned deluge_type_hash(const deluge_type* type);

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
    return type->size;
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

PAS_API void deluge_word_type_dump(deluge_word_type type, pas_stream* stream);

PAS_API void deluge_type_dump(const deluge_type* type, pas_stream* stream);
PAS_API void deluge_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream);
char* deluge_type_to_new_string(const deluge_type* type);

/* This is basically va_arg, but it doesn't check that the type matches. That's fine if the consumer
   of the pointer is code compiled by Deluge, since that will check on every access. That's not fine if
   the consumer is someone writing legacy C code against some Deluge flight API. */
static inline deluge_ptr deluge_ptr_get_next_bytes(
    deluge_ptr* ptr, size_t size, size_t alignment)
{
    uintptr_t ptr_as_int;
    deluge_ptr result;

    ptr_as_int = (uintptr_t)ptr->ptr;
    ptr_as_int = pas_round_up_to_power_of_2(ptr_as_int, alignment);

    result = *ptr;
    result.ptr = (void*)ptr_as_int;

    ptr->ptr = (char*)ptr_as_int + size;

    return result;
}

/* Gives you a heap for the given type. This looks up the heap based on the structural equality
   of the type, so equal types get the same heap.

   It's correct to call this every time you do a memory allocation. It's not the greatest idea, but
   it will be lock-free (and basically fence-free) in the common case.

   The type you pass here must be immortal. The runtime may refer to it forever. (That's not
   strictly necessary so we could change that if we needed to.) */
pas_heap_ref* deluge_get_heap(const deluge_type* type);

void* deluge_try_allocate_int(size_t size);
void* deluge_try_allocate_int_with_alignment(size_t size, size_t alignment);
void* deluge_allocate_int(size_t size);
void* deluge_allocate_int_with_alignment(size_t size, size_t alignment);

void* deluge_try_allocate_one(pas_heap_ref* ref);
void* deluge_allocate_one(pas_heap_ref* ref);
void* deluge_try_allocate_many(pas_heap_ref* ref, size_t count);
void* deluge_allocate_many(pas_heap_ref* ref, size_t count);
void* deluge_try_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment);

/* This can allocate any type (ints or not), but it's considerably slower than the other allocation
   entrypoints. The compiler avoids this in most cases. */
void* deluge_try_allocate_with_type(const deluge_type* type, size_t size);
void* deluge_allocate_with_type(const deluge_type* type, size_t size);

void* deluge_allocate_utility(size_t size);

void* deluge_try_reallocate_int(void* ptr, size_t size);
void* deluge_try_reallocate_int_with_alignment(void* ptr, size_t size, size_t alignment);

void* deluge_try_reallocate(void* ptr, pas_heap_ref* ref, size_t count);

void deluge_deallocate(void* ptr);
void deluded_f_zfree(DELUDED_SIGNATURE);

void deluded_f_zcalloc_multiply(DELUDED_SIGNATURE);

void deluded_f_zgetlower(DELUDED_SIGNATURE);
void deluded_f_zgetupper(DELUDED_SIGNATURE);
void deluded_f_zgettype(DELUDED_SIGNATURE);

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
void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                              const deluge_origin* origin);

static inline void deluge_validate_ptr(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_validate_ptr_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, origin);
}

void deluge_check_access_int_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                                  uintptr_t bytes, const deluge_origin* origin);
void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                                  const deluge_origin* origin);

static inline void deluge_check_access_int(deluge_ptr ptr, uintptr_t bytes, const deluge_origin* origin)
{
    deluge_check_access_int_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, bytes, origin);
}
static inline void deluge_check_access_ptr(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_check_access_ptr_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, origin);
}

void deluge_check_function_call_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                                     const deluge_origin* origin);

static inline void deluge_check_function_call(deluge_ptr ptr, const deluge_origin* origin)
{
    deluge_check_function_call_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, origin);
}

void deluge_check_access_opaque(
    deluge_ptr ptr, const deluge_type* expected_type, const deluge_origin* origin);

void deluge_memset_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                        unsigned value, size_t count, const deluge_origin* origin);
void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                        size_t count, const deluge_origin* origin);
void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                         size_t count, const deluge_origin* origin);

static inline void deluge_memset(deluge_ptr ptr, unsigned value, size_t count, const deluge_origin* origin)
{
    deluge_memset_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, value, count, origin);
}

static inline void deluge_memcpy(deluge_ptr dst, deluge_ptr src, size_t count, const deluge_origin* origin)
{
    deluge_memcpy_impl(dst.ptr, dst.lower, dst.upper, dst.type,
                       src.ptr, src.lower, src.upper, src.type,
                       count, origin);
}

static inline void deluge_memmove(deluge_ptr dst, deluge_ptr src, size_t count, const deluge_origin* origin)
{
    deluge_memmove_impl(dst.ptr, dst.lower, dst.upper, dst.type,
                        src.ptr, src.lower, src.upper, src.type,
                        count, origin);
}

/* Correct uses of this should always pass new_upper = ptr + K * new_type->size. This function does
   not have to check that you did that. This property is ensured by the compiler and the fact that
   the user-visible API (zrestrict) takes a count. */
void deluge_check_restrict(void* ptr, void* lower, void* upper, const deluge_type* type,
                           void* new_upper, const deluge_type* new_type, const deluge_origin* origin);

static inline deluge_ptr deluge_restrict(deluge_ptr ptr, size_t count, const deluge_type* new_type,
                                         const deluge_origin* origin)
{
    void* new_upper;
    new_upper = (char*)ptr.ptr + count * new_type->size;
    deluge_check_restrict(ptr.ptr, ptr.lower, ptr.upper, ptr.type, new_upper, new_type, origin);
    ptr.lower = ptr.ptr;
    ptr.upper = new_upper;
    ptr.type = new_type;
    return ptr;
}

/* Checks that the ptr points at a valid C string. That is, there is a null terminator before we
   get to the upper bound.
   
   It's safe to call legacy C string functions on strings returned from this, since if they lacked
   an in-bounds terminator, then this would have trapped. */
const char* deluge_check_and_get_str(deluge_ptr ptr, const deluge_origin* origin);

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
    slot_ptr = deluge_ptr_get_next_bytes(ptr, 32, 8);
    deluge_check_access_ptr(slot_ptr, origin);
    return *(deluge_ptr*)slot_ptr.ptr;
}

static inline int deluge_ptr_get_next_int(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(int), alignof(int));
    deluge_check_access_int(slot_ptr, sizeof(int), origin);
    return *(int*)slot_ptr.ptr;
}

static inline long deluge_ptr_get_next_long(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(long), alignof(long));
    deluge_check_access_int(slot_ptr, sizeof(long), origin);
    return *(long*)slot_ptr.ptr;
}

static inline unsigned long deluge_ptr_get_next_unsigned_long(deluge_ptr* ptr,
                                                              const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(unsigned long), alignof(unsigned long));
    deluge_check_access_int(slot_ptr, sizeof(unsigned long), origin);
    return *(unsigned long*)slot_ptr.ptr;
}

static inline size_t deluge_ptr_get_next_size_t(deluge_ptr* ptr, const deluge_origin* origin)
{
    deluge_ptr slot_ptr;
    slot_ptr = deluge_ptr_get_next_bytes(ptr, sizeof(size_t), alignof(size_t));
    deluge_check_access_int(slot_ptr, sizeof(size_t), origin);
    return *(size_t*)slot_ptr.ptr;
}

/* Given a va_list ptr (so a ptr to a ptr), this:
   
   - checks that it is indeed a ptr to a ptr
   - uses that ptr to va_arg according to the deluge_ptr_get_next_bytes protocol
   - checks that the resulting ptr can be restructed to count*type
   - of all that goes well, returns a ptr you can load/store from safely according to that type */
void* deluge_va_arg_impl(
    void* va_list_ptr, void* va_list_lower, void* va_list_upper, const deluge_type* va_list_type,
    size_t count, size_t alignment, const deluge_type* type, const deluge_origin* origin);

deluge_global_initialization_context* deluge_global_initialization_context_lock_and_find(
    deluge_global_initialization_context* context, void* global_getter);
void deluge_global_initialization_context_unlock(deluge_global_initialization_context* context);

void deluge_alloca_stack_push(deluge_alloca_stack* stack, void* alloca);
static inline size_t deluge_alloca_stack_save(deluge_alloca_stack* stack) { return stack->size; }
void deluge_alloca_stack_restore(deluge_alloca_stack* stack, size_t size);
void deluge_alloca_stack_destroy(deluge_alloca_stack* stack);

void deluge_defer_or_run_global_ctor(void (*global_ctor)(DELUDED_SIGNATURE));
void deluge_run_deferred_global_ctors(void); /* Important safety property: libc must call this before
                                                letting the user start threads. But it's OK if the
                                                deferred constructors that this calls start threads,
                                                as far as safety goes. */
void deluded_f_zrun_deferred_global_ctors(DELUDED_SIGNATURE);

void deluge_error(const deluge_origin* origin);

void deluded_f_zprint(DELUDED_SIGNATURE);
void deluded_f_zprint_long(DELUDED_SIGNATURE);
void deluded_f_zprint_ptr(DELUDED_SIGNATURE);
void deluded_f_zerror(DELUDED_SIGNATURE);
void deluded_f_zstrlen(DELUDED_SIGNATURE);
void deluded_f_zstrchr(DELUDED_SIGNATURE);
void deluded_f_zisdigit(DELUDED_SIGNATURE);

void deluded_f_zfence(DELUDED_SIGNATURE);

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

void deluded_f_zthread_key_create(DELUDED_SIGNATURE);
void deluded_f_zthread_key_delete(DELUDED_SIGNATURE);
void deluded_f_zthread_setspecific(DELUDED_SIGNATURE);
void deluded_f_zthread_getspecific(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_create(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_delete(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_rdlock(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_tryrdlock(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_wrlock(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_trywrlock(DELUDED_SIGNATURE);
void deluded_f_zthread_rwlock_unlock(DELUDED_SIGNATURE);

PAS_END_EXTERN_C;

#endif /* DELUGE_RUNTIME_H */

