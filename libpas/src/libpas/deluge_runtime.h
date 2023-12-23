#ifndef DELUGE_RUNTIME_H
#define DELUGE_RUNTIME_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pas_allocation_config.h"
#include "pas_hashtable.h"
#include "pas_heap_ref.h"
#include "pas_lock.h"

PAS_BEGIN_EXTERN_C;

/* Internal Deluge runtime header, defining how the Deluge runtime maintains its state. */

struct deluge_ptr;
struct deluge_type;
struct pas_stream;
typedef struct deluge_ptr deluge_ptr;
typedef struct deluge_type deluge_type;
typedef struct pas_stream pas_stream;

typedef uint8_t deluge_word_type;

#define DELUGE_WORD_TYPE_OFF_LIMITS  ((uint8_t)0)
#define DELUGE_WORD_TYPE_INT         ((uint8_t)1)
#define DELUGE_WORD_TYPE_PTR_PART1   ((uint8_t)2)
#define DELUGE_WORD_TYPE_PTR_PART2   ((uint8_t)3)
#define DELUGE_WORD_TYPE_PTR_PART3   ((uint8_t)4)
#define DELUGE_WORD_TYPE_PTR_PART4   ((uint8_t)5)
#define DELUGE_WORD_TYPE_FUNCTION    ((uint8_t)6)

struct deluge_ptr {
    void* ptr;
    void* lower;
    void* upper;
    const deluge_type* type;
};

struct deluge_type {
    size_t size;
    size_t alignment;
    const deluge_type* trailing_array;
    deluge_word_type word_types[1];
};

PAS_API extern const deluge_type deluge_int_type;
PAS_API extern const deluge_type deluge_function_type;

PAS_DECLARE_LOCK(deluge);

/* FIXME: There should be a subtype of deluge_type that has no word_types but that describes
   function pointers. */

static inline size_t deluge_type_num_words(const deluge_type* type)
{
    return (type->size + 7) / 8;
}

static inline size_t deluge_type_num_words_exact(const deluge_type* type)
{
    PAS_TESTING_ASSERT(!(type->size % 8));
    return type->size / 8;
}

static inline deluge_word_type deluge_type_get_word_type(const deluge_type* type,
                                                         uintptr_t word_type_index)
{
    if (type->trailing_array) {
        uintptr_t num_words = deluge_type_num_words_exact(type);
        if (word_type_index >= num_words) {
            word_type_index -= num_words;
            type = type->trailing_array;
        }
    }
    return type->word_types[word_type_index % deluge_type_num_words(type)];
}

/* Run assertions on the type itself. The runtime isn't guaranteed to ever run this check. The
   compiler is expected to only produce types that pass this check, and we provide no path for
   the user to create types (unless they write unsafe code). */
void deluge_validate_type(const deluge_type* type);

bool deluge_type_is_equal(const deluge_type* a, const deluge_type* b);
unsigned deluge_type_hash(const deluge_type* type);

static inline size_t deluge_type_representation_size(const deluge_type* type)
{
    return PAS_OFFSETOF(deluge_type, word_types)
        + sizeof(deluge_word_type) * deluge_type_num_words(type);
}
    
static inline size_t deluge_type_as_heap_type_get_type_size(const pas_heap_type* type)
{
    return ((const deluge_type*)type)->size;
}

static inline size_t deluge_type_as_heap_type_get_type_alignment(const pas_heap_type* type)
{
    return ((const deluge_type*)type)->alignment;
}

PAS_API void deluge_word_type_dump(deluge_word_type type, pas_stream* stream);

PAS_API void deluge_type_dump(const deluge_type* type, pas_stream* stream);
PAS_API void deluge_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream);

/* Gives you a heap for the given type. This looks up the heap based on the structural equality
   of the type, so equal types get the same heap.

   It's correct to call this every type you do a memory allocation, but it's not a super great
   idea. Currently it grabs global locks.

   The type you pass here must be immortal. The runtime may refer to it forever. (That's not
   strictly necessary so we could change that if we needed to.) */
pas_heap_ref* deluge_get_heap(const deluge_type* type);

void* deluge_try_allocate_int(size_t size);

void* deluge_try_allocate_one(pas_heap_ref* ref);
void* deluge_try_allocate_many(pas_heap_ref* ref, size_t count);

void* deluge_allocate_utility(size_t size);

void deluge_deallocate(void* ptr);
void deluded_zfree(void* ptr, void* lower, void* upper, const deluge_type* type);

/* Run assertions on the ptr itself. The runtime isn't guaranteed to ever run this check. Pointers
   are expected to be valid by construction. This asserts properties that are going to be true
   even for user-forged pointers using unsafe API, so the only way to break these asserts is to
   forge the pointer by hand using the struct and then to get some invariant wrong.
   
   One example invariant: lower/upper must be aligned on the type's required alignment.
   Another example invariant: !((upper - lower) % type->size)
   
   There may be others.
   
   This does not check if the pointer is in bounds or that it's pointing at something that has any
   particular type. This isn't the actual Deluge check that the compiler uses to achieve memory
   safety! */
void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type);

static inline void deluge_validate_ptr(deluge_ptr ptr)
{
    deluge_validate_ptr_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type);
}

void deluge_check_access_int_impl(void* ptr, void* lower, void* upper, const deluge_type* type, uintptr_t bytes);
void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, const deluge_type* type);

static inline void deluge_check_access_int(deluge_ptr ptr, uintptr_t bytes)
{
    deluge_check_access_int_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, bytes);
}
static inline void deluge_check_access_ptr(deluge_ptr ptr)
{
    deluge_check_access_ptr_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type);
}

void deluge_memset_impl(void* ptr, void* lower, void* upper, const deluge_type* type,
                        unsigned value, size_t count);
void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                        size_t count);
void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, const deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, const deluge_type* src_type,
                         size_t count);

static inline void deluge_memset(deluge_ptr ptr, unsigned value, size_t count)
{
    deluge_memset_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, value, count);
}

static inline void deluge_memcpy(deluge_ptr dst, deluge_ptr src, size_t count)
{
    deluge_memcpy_impl(dst.ptr, dst.lower, dst.upper, dst.type,
                       src.ptr, src.lower, src.upper, src.type,
                       count);
}

static inline void deluge_memmove(deluge_ptr dst, deluge_ptr src, size_t count)
{
    deluge_memmove_impl(dst.ptr, dst.lower, dst.upper, dst.type,
                        src.ptr, src.lower, src.upper, src.type,
                        count);
}

/* Correct uses of this should always pass new_upper = ptr + K * new_type->size. This function does
   not have to check that you did that. This property is ensured by the compiler and the fact that
   the user-visible API (zrestrict) takes a count. */
void deluge_check_restrict(void* ptr, void* lower, void* upper, const deluge_type* type,
                           void* new_upper, const deluge_type* new_type);

static inline deluge_ptr deluge_restrict(deluge_ptr ptr, size_t count, const deluge_type* new_type)
{
    void* new_upper;
    new_upper = (char*)ptr.ptr + count * new_type->size;
    deluge_check_restrict(ptr.ptr, ptr.lower, ptr.upper, ptr.type, new_upper, new_type);
    ptr.lower = ptr.ptr;
    ptr.upper = new_upper;
    ptr.type = new_type;
    return ptr;
}

void deluge_error(void);

PAS_END_EXTERN_C;

#endif /* DELUGE_RUNTIME_H */

