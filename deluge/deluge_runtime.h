#ifndef DELUGE_RUNTIME_H
#define DELUGE_RUNTIME_H

#include <inttypes.h>

/* Internal Deluge runtime header, defining how the Deluge runtime maintains its state. */

struct deluge_heap_ref;
struct deluge_opaque_heap;
struct deluge_ptr;
struct deluge_type;
typedef struct deluge_heap_ref deluge_heap_ref;
typedef struct deluge_opaque_heap deluge_opaque_heap;
typedef struct deluge_ptr deluge_ptr;
typedef struct deluge_type deluge_type;

typedef uint8_t deluge_word_type;

#define DELUGE_WORD_TYPE_INVALID     ((uint8_t)0)
#define DELUGE_WORD_TYPE_INT         ((uint8_t)1)
#define DELUGE_WORD_TYPE_PTR_PART1   ((uint8_t)2)
#define DELUGE_WORD_TYPE_PTR_PART2   ((uint8_t)3)
#define DELUGE_WORD_TYPE_PTR_PART3   ((uint8_t)4)
#define DELUGE_WORD_TYPE_PTR_PART4   ((uint8_t)5)

struct deluge_ptr {
    void* ptr;
    void* lower;
    void* upper;
    deluge_type* type;
};

struct deluge_type {
    size_t size;
    size_t alignment;
    deluge_type* trailing_array;
    deluge_word_type word_types[1];
};

/* FIXME: There should be a subtype of deluge_type that has no word_types but that describes
   function pointers. */

/* This struct aligns exactly with pas_heap_ref. */
struct deluge_heap_ref {
    deluge_type* type;
    deluge_opaque_heap* opaque_heap; /* owned by libpas, initialize to NULL. */
    unsigned allocator_index; /* owned by libpas, initialize to 0. */
};

/* Run assertions on the type itself. The runtime isn't guaranteed to ever run this check. The
   compiler is expected to only produce types that pass this check, and we provide no path for
   the user to create types (unless they write unsafe code). */
void deluge_validate_type(deluge_type* type);

bool deluge_type_is_equal(deluge_type* a, deluge_type* b);
unsigned deluge_type_hash(deluge_type* type);

/* Gives you a heap for the given type. This looks up the heap based on the structural equality
   of the type, so equal types get the same heap.

   It's correct to call this every type you do a memory allocation, but it's not a super great
   idea. */
deluge_heap_ref* deluge_get_heap(deluge_type* type);

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
void deluge_validate_ptr_impl(void* ptr, void* lower, void* upper, deluge_type* type);

static inline void deluge_validate_ptr(deluge_ptr ptr)
{
    deluge_validate_ptr_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type);
}

void deluge_check_access_int_impl(void* ptr, void* lower, void* upper, deluge_type* type, uintptr_t bytes);
void deluge_check_access_ptr_impl(void* ptr, void* lower, void* upper, deluge_type* type);

static inline void deluge_check_access_int(deluge_ptr ptr, uintptr_t bytes)
{
    deluge_check_access_int_impl(ptr.ptr, ptr.lower, ptr.upper, ptr.type, bytes);
}
static inline void deluge_check_access_ptr(deluge_ptr ptr)
{
    deluge_check_access_ptr(ptr.ptr, ptr.lower, ptr.upper, ptr.type);
}

void deluge_memset_impl(void* ptr, void* lower, void* upper, deluge_type* type,
                        unsigned value, size_t count);
void deluge_memcpy_impl(void* dst_ptr, void* dst_lower, void* dst_upper, deluge_type* dst_type,
                        void *src_ptr, void* src_lower, void* src_upper, deluge_type* src_type,
                        size_t count);
void deluge_memmove_impl(void* dst_ptr, void* dst_lower, void* dst_upper, deluge_type* dst_type,
                         void *src_ptr, void* src_lower, void* src_upper, deluge_type* src_type,
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

void deluge_error(void);

#endif /* DELUGE_RUNTIME_H */

