#ifndef DELUGE_STDFIL_H
#define DELUGE_STDFIL_H

/* ztype: Opaque representation of the Deluge type. It's not possible to access the contents of a
   Deluge type except through whatever APIs we provide. Deluge types are tricky, since they are
   orthogonal to bounds. Every pointer has a bounds separate from the type, and the type's job is
   to provide additional information. The only requirement is that a pointer's total range is a
   multiple of whatever type it uses, or that it makes the math of flexes work out (where there is
   an array of some type and a header).

   Deluge types can tell you the layout of memory if you also give them a slice, or span - a tuple
   of begin and end, in bytes.

   Deluge types by themselves know a size for the purpose of informing the internal algorithm. It
   should not be taken to mean the actual size of something. In particular, say you have a struct
   like:

       struct dejavu {
           T a;
           U b;
           T c;
           U d
       };
   
   Observe that this type is the same as if we had an array of exactly two entries of this struct:
   
       struct pair {
           T a;
           U b;
       };
   
   It's valid for a deluge pointer to struct dejavu to describe itself in either of the following
   ways:
   
       1. We could say that the pointer points to one dejavu.
       2. We could say that the pointer points to two pairs.
   
   The statements are equivalent under the Deluge type system, so the runtime and compiler are free
   to pick whichever representation is most convenient. The type checks will work out the same
   either way, because every type check (and every compiler-generated type introspection and all
   type introspection in the runtime itself) always starts with a type and a slice, and the slice
   has offsets in bytes.
   
   Things get even weirder when you consider that ztypes can represent flexes - objects with trailing
   arrays. Flexes secretly also know the type of the array and where it starts. Hence, a flex can only
   tell you memory layout if you only give it a slice since otherwise we don't know how many elements
   the array has.
   
   This is quite different from C types. The APIs found here sometimes deal with C types and
   sometimes with ztypes. APIs that take a C type do not need a slice, since the C type is both a
   layout and a size. But anytime we have an API that takes a ztype - so anytime we want to deal with
   types at runtime - then to emulate the behavior of a C type, we need at least a ztype and a size.
   This denotes a slice starting at zero and ending at size. Those APIs may require that the size is
   a multiple of the type's size and that the type not be a flex (like zalloc_with_type). Or, they
   may allow the size to be any anything so long as 8 byte alignment is maintained for anything with
   pointers. For example, slicing dejavu with [0, sizeof(dejavu) * 1.5) would create a type that is
   equivalent to three pairs (i.e. it might even return the pair type). Basically, going "out of
   bounds" of the type works as if we were asking for the layout of an array of that type, or the
   layout of a flex with the appropriate array length to fit the slice.

   This fact about ztypes can be traced to the fact that *every* pointer access in Deluge must
   perform a bounds check based on that pointer's lower and upper, and the lower/upper pair can't be
   faked or tricked. So, the ztype's job is to give you information that is orthogonal to the bounds.
   Therefore, ztypes are not in the business of bounds checking and always assume that your slice
   makes sense. The business of bounds checking typically happens before the ztype even gets
   consulted. */
struct ztype;
typedef struct ztype ztype;

/* Don't call these _impls directly. Any uses that aren't exactly like the ones in the #defines may 
   crash the compiler or produce a program that traps extra hard. */
void* zunsafe_forge_impl(const void* ptr, void* type_like, __SIZE_TYPE__ count);
void* zrestrict_impl(const void* ptr, void* type_type, __SIZE_TYPE__ count);
void* zalloc_impl(void* type_like, __SIZE_TYPE__ count);
void* zalloc_flex_impl(void* type_like, __SIZE_TYPE__ offset,
                       void* trailing_type_like, __SIZE_TYPE__ count);
void* zaligned_alloc_impl(void* type_like, __SIZE_TYPE__ alignment, __SIZE_TYPE__ count);
void* zrealloc_impl(void* old_ptr, void* type_like, __SIZE_TYPE__ count);
void* zhard_alloc_impl(void* type_like, __SIZE_TYPE__ count);
void* zhard_alloc_flex_impl(void* type_like, __SIZE_TYPE__ offset,
                           void* trailing_type_like, __SIZE_TYPE__ count);
void* zhard_aligned_alloc_impl(void* type_like, __SIZE_TYPE__ alignment, __SIZE_TYPE__ count);
void* zhard_realloc_impl(void* old_ptr, void* type_like, __SIZE_TYPE__ count);
_Bool zcalloc_multiply(__SIZE_TYPE__ left, __SIZE_TYPE__ right, __SIZE_TYPE__ *result);
ztype* ztypeof_impl(void* type_like);

/* Unsafely creates a pointer that will claim to point at count repetitions of the given type.
   
   It's super awesome to never use this. So, far the only uses of this are in the test suite, just
   to make sure it works at all.
   
   This is the only escape hatch (other than writing legacy C code and linking it to your Deluge code).
   
   ptr can be anything castable to const void*. type is a type expression. count must be __SIZE_TYPE__
   ish. */
#define zunsafe_forge(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zunsafe_forge_impl((const void*)(ptr), &__d_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Safely restricts the capability of the incoming pointer. If the given pointer cannot be treated as
   the given type and size, trap.
   
   ptr must be a valid pointer, but can be of any type. type is a type expression. count must be
   __SIZE_TYPE__ish. */
#define zrestrict(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zrestrict_impl(ptr, &__d_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Allocates count repetitions of the given type from virtual memory that has never been pointed at
   by pointers that view it as anything other than count or more repetitions of the given type.
   
   Misuse of zalloc/zfree may cause logic errors where zalloc will return the same pointer as it had
   previously returned, so pointers that you expected to different objects will end up pointing at the
   same object.
   
   It's not possible to misuse zalloc/zfree to cause type confusion under the Deluge P^I type system.
   
   type is a type expression. count must be __SIZE_TYPE__ish. */
#define zalloc(type, count) ({ \
        type __d_temporary; \
        (type*)zalloc_impl(&__d_temporary, (__SIZE_TYPE__)(count)); \
    })

#define zalloc_zero(type, count) ({ \
        type __d_temporary; \
        __SIZE_TYPE__ __d_count = (__SIZE_TYPE__)(count); \
        type* __d_result = (type*)zalloc_impl(&__d_temporary, __d_count); \
        if (__d_result) \
            __builtin_memset(__d_result, 0, sizeof(type) * __d_count); \
        __d_result; \
    })

#define zcalloc(type, something_to_multiply, another_thing_to_multiply) ({ \
        __SIZE_TYPE__ __d_size; \
        _Bool __d_mul = zcalloc_multiply((something_to_multiply), (another_thing_to_multiply), &__d_size); \
        __d_mul ? zalloc_zero(type, __d_size) : NULL; \
    })

#define zaligned_alloc(type, alignment, count) ({ \
        type __d_temporary; \
        (type*)zaligned_alloc_impl(&__d_temporary, (__SIZE_TYPE__)(alignment), (__SIZE_TYPE__)(count)); \
    })

/* Allocates a flex object - that is, an object with a trailing array - with the given array length.

   struct_type is a type expression; this must be a struct (or a typedef for a struct). field is the
   name of a field in struct_type; this field is taken to be the trailing array. count must be
   __SIZE_TYPE__ ish.

   Misuse of this API may cause logic errors, or more likely, a situation where the returned pointer
   has a type that is incompatible with all subsequent accesses, leading to deluge thwarting your
   program's execution. */
#define zalloc_flex(struct_type, field, count) ({ \
        struct_type __d_temporary; \
        __typeof__(__d_temporary.field[0]) __d_trailing_temporary; \
        (struct_type*)zalloc_flex_impl( \
            &__d_temporary, __builtin_offsetof(struct_type, field), \
            &__d_trailing_temporary, (__SIZE_TYPE__)(count)); \
    })

#define zalloc_flex_zero(struct_type, field, count) ({ \
        struct_type __d_temporary; \
        __typeof__(__d_temporary.field[0]) __d_trailing_temporary; \
        __SIZE_TYPE__ __d_count = (__SIZE_TYPE__)(count); \
        struct_type* __d_result = (struct_type*)zalloc_flex_impl( \
            &__d_temporary, __builtin_offsetof(struct_type, field), \
            &__d_trailing_temporary, __d_count); \
        if (__d_result) { \
            __builtin_memset( \
                __d_result, 0, \
                __builtin_offsetof(struct_type, field) \
                + sizeof(__d_trailing_temporary) * __d_count); \
        } \
        __d_result; \
    })

/* Allocates count repetitions of the given type from virtual memory that has never been pointed at
   by pointers that view it as anything other than count or more repetitions of the given type.
   The new memory is populated with a copy of the passed-in pointer. If the pointer points at more
   than count repetitions of the type, then only the first count are copied. If the pointer points
   at fewer than count repetitions of the type, then we only copy whatever it has.
   
   The pointer must point at an allocation of the same type as the one we're requesting, else this
   traps.
   
   Misuse of zrealloc/zalloc/zfree may cause logic errors where zalloc/realloc will return the same
   pointer as it had previously returned.
   
   It's not possible to misuse zrealloc to cause type confusion under the Deluge P^I type system.
   
   ptr is a pointer of the given type, type is a type expression, count must be __SIZE_TYPE__ ish. */
#define zrealloc(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zrealloc_impl(ptr, &__d_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Free the object starting at the given pointer.
   
   If you pass NULL, this silently returns.
   
   If you pass a pointer where lower is NULL, this thwarts your program's stupidity by panicking.
   
   If you pass a pointer that is offset from lower (i.e. ptr != lower), this panicks your dumb program.
   
   If you pass a pointer that has an opaque type (like the type type, the function type, or deluge
   runtime types like the ones used to implement zthread APIs), this kills the shit out of your
   program.
   
   If you pass a pointer that is already freed or was never allocated, then this might trap either now
   or during any future call to zfree or zalloc. Note that the state where a trap is coming but hasn't
   happened is not a weird state for the allocator; it's just buffering up the part where it will free
   the object, and when that happens, the allocator will deterministically and well-defindedly either
   identify an object it had allocated that it will now free, or it will kill the shit out of your
   program.
   
   If you pass a pointer to the middle of an allocated object (which is still possible by zrestricting
   and then zfreeing), then this might either free the whole object, or trap now, or trap at any future
   call to zfree or zalloc. Basically, for interior pointers the allocator might first round them down
   to some minalign as a byproduct of the algorithm's bitvector implementation. So if this works:
   
       zfree(ptr);
   
   Then this will also work and be equivalent in practice for right now, assuming that at ptr+1 there's
   an int byte:
   
       zfree(zrestrict((char*)ptr + 1, char, 1));
   
   It works and is safe because the allocator happens to not mind whatever's in the low 4 bits. The
   zrestrict call is necessary to get around the ptr=lower check.
   
   If you free an object and then use it again, then that might be fine. But, free memory may be
   decommitted at any time (so many start to trap or suddenly become all-zero). Memory that becomes
   zero invalidates ptrs even when they straddle pages.

   This has to be a synonym for free(), and it's up to libc to make this be the case by implementing
   free() as a call to zfree(). If the libc author feels like doing extra checking (like if they
   don't want it to be OK to free NULL), then whatever. */
void zfree(void* ptr);

/* If the allocator owns this pointer, returns the number of bytes allocated. This can be more than
   the number of bytes in the pointer's capability, so you may not actually be able to use all of
   the memory. The number of bytes allocated is always greater than zero.

   If the allocator does not own this pointer, returns zero. */
__SIZE_TYPE__ zgetallocsize(void* ptr);

/* Allocate memory in the hard heap.
   
   The hard heap is intended for storing cryptographic secrets that cannot be written to swap, core
   dumped, or kept in memory longer than necessary.
   
   This heap is incompatible with zfree(); you have to use zhard_free() to free memory from
   zhard_alloc().
   
   The memory backing this heap is always given guard pages, is always locked (cannot be swapped),
   and has DONT_DUMP (if the OS supports it). Additinally, both allocation and free always zero the
   whole allocation. All underlying page regions allocated into this allocator have guard pages
   around them. Automatic decommit still works. If the heap becomes empty, all of the pages will
   be zeroed, unlocked, decommitted, and protected (PROT_NONE).

   And, of course, the hard heap is totally isoheaped, hence an API that looks like normal zalloc
   and has all of the guarantees as zalloc in addition to the mlocking B.S. */
#define zhard_alloc(type, count) ({ \
        type __d_temporary; \
        (type*)zhard_alloc_impl(&__d_temporary, (__SIZE_TYPE__)(count)); \
    })
#define zhard_calloc(type, something_to_multiply, another_thing_to_multiply) ({ \
        __SIZE_TYPE__ __d_size; \
        _Bool __d_mul = zcalloc_multiply((something_to_multiply), (another_thing_to_multiply), &__d_size); \
        __d_mul ? zhard_alloc(type, __d_size) : NULL; \
    })
#define zhard_aligned_alloc(type, alignment, count) ({ \
        type __d_temporary; \
        (type*)zhard_aligned_alloc_impl( \
            &__d_temporary, (__SIZE_TYPE__)(alignment), (__SIZE_TYPE__)(count)); \
    })
#define zhard_alloc_flex(struct_type, field, count) ({ \
        struct_type __d_temporary; \
        __typeof__(__d_temporary.field[0]) __d_trailing_temporary; \
        (struct_type*)zhard_alloc_flex_impl( \
            &__d_temporary, __builtin_offsetof(struct_type, field), \
            &__d_trailing_temporary, (__SIZE_TYPE__)(count)); \
    })
#define zhard_realloc(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zhard_realloc_impl(ptr, &__d_temporary, (__SIZE_TYPE__)(count)); \
    })
void zhard_free(void* ptr);
__SIZE_TYPE__ zhard_getallocsize(void* ptr);

/* Accessors for the bounds and type.
 
   The lower and upper bounds have the same capability as the incoming ptr. So, if you know that a
   ptr points into the middle of struct foo and you want to get to the base of struct foo, you can
   totally do:

       struct foo* foo = (struct foo*)zgetlower(ptr);

   Or if you know that ptr points to an array of struct foos, and you want to get a pointer to the
   last one:
 
       struct foo* foo = (struct foo*)zgetupper(ptr) - 1;
       
   In both cases, the pointer is usable provided that the bounds are big enough for struct foo and
   that the type is compatible with struct foo.
   
   On the other hand, zgettype returns a very special kind of pointer. Anytime stdfil API says it
   returns a ztype*, you'll get a pointer that knows that it points at the type type. It's not
   possible to access such a pointer, but it is possible to pass it to APIs that take ztype*. APIs
   that take ztype* validate the integrity of the type pointer. If you add anything to a ztype*,
   then the capability will reject your pointer, so any uses will fail. So, it's not possible to
   turn one ztype* into another via arithmetic. */
void* zgetlower(void* ptr);
void* zgetupper(void* ptr);
ztype* zgettype(void* ptr);

/* Get the pointer's array length, which is the distance to upper in units of the ptr's static type. */
#define zlength(ptr) ({ \
        __typeof__(ptr) __d_ptr = (ptr); \
        (__typeof__(ptr))zgetupper(__d_ptr) - __d_ptr; \
    })

/* Tells if the pointer is in bounds of lower/upper. This is not a guarantee that accesses will
   succeed, since this does not check type. For example, valid function pointers are zinbounds but
   cannot be "accessed" regardless of type (can only be called if in bounds). */
_Bool zinbounds(void* ptr);

/* Given a type and a range, returns a type that describes that range. Note that it's valid to ask
   for a range bigger than the type. It's also valid for this to return a type whose size is smaller
   than the range, if we can optimize repetitions (for example, InP1P2P3P4 is the same as
   InP1P2P3P4InP1P2P3P4, so the algorithm could return either, unless the range is too small for
   InP1P2P3P4InP1P2P3P4). That said, don't expect that this function will definitely do any such
   optimizations. In particular, if the range is large, then this might (try to) allocate a
   ginormous type. Since this never returns NULL, failure in those allocations gives you the
   ultimate security mitigation (i.e. it kills the shit out of your program). */
ztype* zslicetype(ztype* type, __SIZE_TYPE__ begin, __SIZE_TYPE__ end);

/* Gets a type that describes the first bytes of memory starting at where ptr points.
 
   Note that you could implement this using zgetlower, zgetupper, zgettype, and zslicetype, but it
   would be gross. That's what happens behind the scenes, more or less. */
ztype* zgettypeslice(void* ptr, __SIZE_TYPE__ bytes);

/* Gets the ztype of the given C type. */
#define ztypeof(type) ({ \
        type __d_temporary; \
        ztypeof_impl(&__d_temporary); \
    })

/* Concatenates the two type slices to create a new type. As with zslicetype, if the type looks like
   an array of some simpler type, then the simpler type might get returned. That's also why you must
   pass a size. */
ztype* zcattype(ztype* a, __SIZE_TYPE__ asize, ztype* b, __SIZE_TYPE__ bsize);

/* Allocate a bytes-size array of the given type, which is given dynamically. The size must be a
   multiple of the type's size.

   Here's an example use case. You can allocate some memory that can be used for memcpy'ing some bytes
   out of an object and back again:

       void* ptr2 = zalloc_with_type(zgettypeslice(ptr, bytes), bytes);

   Or, to clone a whole object, you can do shenanigans like this:
   
       void* ptr2 = zalloc_with_type(zgettype(ptr), (char*)zgetupper(ptr) - (char*)zgetlower(ptr));
       memcpy(ptr2, zgetlower(ptr), (char*)zgetupper(ptr) - (char*)zgetlower(ptr));
       ptr2 = (char*)ptr2 + ((char*)ptr - (char*)zgetlower(ptr));
   
   This is a memory-safe escape hatch for situations where you don't know the type of the data that
   you're dealing with, that data may have pointers, and you just want to be able to carry that data
   around. Other than allowing you to specify a type dynamically (which implies picking the right
   isoheap dynamically), it's exactly the same as zalloc. */
void* zalloc_with_type(ztype* type, __SIZE_TYPE__ size);

/* Allocates a new string (with zalloc(char, strlen+1)) and prints a dump of the type to that string.
   Returns that string. You have to zfree the string when you're done with it.

   This is exposed as %T in the zprintf family of functions. */
char* ztype_to_new_string(ztype* type);

/* Allocates a new string (with zalloc(char, strlen+1)) and prints a dump of the ptr to that string.
   Returns that string. You have to zfree the string when you're done with it.

   This is exposed as %P in the zprintf family of functions. */
char* zptr_to_new_string(const void* ptr);

/* Low-level printing functions. These might die someday. They are useful for Deluge's own tests. They
   print directly to stdout using write(). They are safe (passing an invalid ptr to zprint() will trap
   for sure, and it will never print out of bounds even if there is no null terminator). */
void zprint(const char* str);
void zprint_long(long x);
void zprint_ptr(const void* ptr);

/* Low-level functions that should be provided by libc, which should live above this. For now they are
   here because we don't have that libc. */
__SIZE_TYPE__ zstrlen(const char* str);
char* zstrchr(const char* str, int chr);
void* zmemchr(const void* str, int chr, __SIZE_TYPE__ length);
int zisdigit(int chr);

/* This is almost like sprintf, but because Deluge knows the upper bounds of buf, this actually ends
   up working exactly like snprintf where the size is upper-ptr. Hence, in Deluge, it's preferable
   to call zsprintf instead of zsnprintf.

   In libc, sprintf (without the z) behaves kinda like zsprintf, but traps on OOB.

   The main difference from the libc sprintf is that it uses a different implementation under the hood.
   This is based on the samba snprintf, origindally by Patrick Powell, but it uses the zstrlen/zisdigit/etc
   functions rather than the libc ones, and it has some additional features:

       - '%P', which prints the full deluge_ptr (i.e. 0xptr,0xlower,0xupper,type{thingy}).
       - '%T', which prints the deluge_type or traps if you give it anything but a ztype.

   It's not obvious that this code will do the right thing for floating point formats. But this code is
   deluded, so if it goes wrong, at least it'll stop your program from causing any more damage. */
int zvsprintf(char* buf, const char* format, __builtin_va_list args);
int zsprintf(char* buf, const char* format, ...);

int zvsnprintf(char* buf, __SIZE_TYPE__ size, const char* format, __builtin_va_list args);
int zsnprintf(char* buf, __SIZE_TYPE__ size, const char* format, ...);

/* This is like asprintf, but instead of super annoyingly returning the string in an out argument,
   it just fucking returns it in the return value like a fucking sensible function. */
char* zvasprintf(const char* format, __builtin_va_list args);
char* zasprintf(const char* format, ...);

/* This is mostly just like printf, but does only per-call buffering. In particular, this relies on
   zvasprintf under the hood and then prints the entire string in one write(2) call (unless write
   demands that we call it again).

   Note that the main reason why you might want to use this for debugging over printf is that it supports:

       - '%P', which prints the full deluge_ptr (i.e. 0xptr,0xlower,0xupper,type{thingy}).
       - '%T', which prints the deluge_type or traps if you give it anything but a ztype.

   But if you want to debug floating point, you should maybe go with printf. */
void zvprintf(const char* format, __builtin_va_list args);
void zprintf(const char* format, ...);

/* This prints the given message and then shuts down the program using the same shutdown codepath
   used for memory safety violatins (i.e. it's designed to really kill the shit out of the process). */
void zerror(const char* str);
void zerrorf(const char* str, ...);

/* Definitely assert something. This is not some kind of optional assert that you can compile out.
   It's gonna be there and do its thing no matter what, even in production, like a real assert
   should. */
#define ZASSERT(exp) do { \
        if ((exp)) \
            break; \
        zerrorf("%s:%d: %s: assertion %s failed.", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    } while (0)

void zfence(void);
void zstore_store_fence(void);
void zcompiler_fence(void);

/* Currently, the compiler builtins for ptr CAS don't work for silly clang reasons. So, Deluge
   offers these functions instead.

   But pointers are 32 bytes, you say! How can they be atomic, you say! What the fuck is going on!

   Deluge pointers comprise a sidecar, which is optional, and a capability, which is the source of
   truth. A missing sidecar causes the pointer's lower bound to snap up to the pointer itself.
   Anytime that a pointer has a sidecar that doesn't match the capability (points at the wrong
   place or has the wrong type), then the sidecar is ignored, as if it wasn't there.
   
   Every possible outcome is memory-safe under Deluge Bounded P^I rules. But, some races on
   pointers will trap in Deluge even though they would have worked in Legacy C.
   
   Atomic ops on pointers always just clear the sidecar.
   
   Pointer stores and loads will store and load the sidecar, but races may cause the sidecar to be
   mismatched and treated as missing.
   
   Both sidecar and capability are always accessed using 128-bit atomics.
   
   The result:
   
   - Non-racing pointer loads and stores always get a valid sidecar, so pointers loaded sans races
     always know their full lower bound, so you can subtract stuff from them and dereference them
     so long as you stay within those bounds.
   
   - Racing pointer loads and stores are likely to lose their sidecar. Pointers loaded under races
     still always know their upper bound and where they point. They also know the type of the
     memory between where they point and their upper bound. But, the lower bound of a pointer
     loaded under race will sometimes be higher than the pointer that was originally stored. So,
     subtracting from pointers loaded under races, and then dereferencing those downward-traveling
     pointers sometimes works and sometimes doesn't.
   
   - Pointers used for atomics are guaranteed to lose their sidecar, so their lower bound will
     always snap up to where they point. Those pointers still know their full value and upper
     bound.
   
   Pointers that lose their sidecar are still usable for most of what C programmers use pointers
   for. Most of the time, you load a pointer so that you can add to it (for example to perform an
   array or field access - both of which are additions under the hood). That always works, even
   under races and atomics.
   
   You will notice the missing sidecar behavior if you race or CAS on a pointer that is used for
   stuff like:
   
   - Downward-traveling iterator pointer. Say you want to write a lock-free or racy concurrent
     iteration protocol based on pointers that you subtract from. That won't work in Deluge
     unless you put a lock around it.
   
   - Some cases of intrusive structs. This one is funny. A missing sidecar means that the lower
     bound is inferred from the capability's ptr, type, and upper. But Deluge requires that:

         upper - lower = K * type->size

     When recovering the sidecar, we keep upper unchanged, so the lower that we infer will often
     be below the ptr - it'll be far enough below it so that the lower is on type boundary.

     This means that an intrusive linked list can still be lock-free and racy! Linked list means
     pointers, and pointers mean nontrivial type, and nontrivial type means that the type knows
     the size. So, a pointer into the middle of a single object (not array) with pointers will
     never seem to lose its sidecar, since the sidecar contains purely redundant information in
     that case (the lower bound can always be inferred from the capability).
     
     However, intrusive pointers into integer-only structs won't work (the type is int in that
     case, and int->size = 1). Also, intrusive pointers into arrays of structs with pointers
     will appear to lose their sidecar in the sense that the pointer after race will no longer
     know the accurate base of the array, but will instead approximate it as the base of
     whatever array element you're pointing at.

   In summary, Deluge's 32 byte wide pointers are Atomic Enough (TM) for most uses of pointers.
   If you have code that wants to CAS pointers or race on them then go right ahead!

   Eventually, I'll even fix the clang bugs and then you'll be able to just use whatever your
   favorite compiler builtin for CAS is, instead of this junk. And those builtins will have
   identical semantics to these functions.

   If you want to CAS primitives, then just use the builtins, those work today.

   I have simplified the memory ordering approach based on pretty good data that only a tiny
   fraction of algorithms ever benefit from unfenced CAS on modern CPUs, and the fact that CPUs
   usually only give you either one or two variants. The "unfenced" variants are like RELAXED
   in the C model. The not-"unfenced" ones are like SEQ_CST. Strong CAS just returns the old
   value rather than both a bool and the old value, since emprically, relying on the bit that
   the CAS instruction returns for brancing on the CAS is never any faster than branching on
   a comparison of your expected value and the old value returned by CAS.

   I may add more ptr atomic functions as I find a need for them. */
_Bool zunfenced_weak_cas_ptr(void** ptr, void* expected, void* new_value);
_Bool zweak_cas_ptr(void** ptr, void* expected, void* new_value);
void* zunfenced_strong_cas_ptr(void** ptr, void* expected, void* new_value);
void* zstrong_cas_ptr(void** ptr, void* expected, void* new_value);
void* zunfenced_xchg_ptr(void** ptr, void* new_value);
void* zxchg_ptr(void** ptr, void* new_value);

/* Returns true if running in the build of the runtime that has extra (super expensive) testing
   checks.

   This is here so that the test suite can assert that it runs with testing asserts enabled. */
_Bool zis_runtime_testing_enabled(void);

/* ------------------ All APIs below here are intended for libc consumption ------------------------- */

/* These APIs are memory-safe, so you won't escape the deluge by using them. But it's not clear to me to
   what extent I give a shit about maintaining compatible semantics for these calls. They're meant as a
   replacement for musl's syscall layer, and they're very much tailored to my hacks to musl. */

void zrun_deferred_global_ctors(void);

void zregister_sys_errno_handler(void (*errno_handler)(int errno_value));
int zsys_ioctl(int fd, unsigned long request, ...);
long zsys_writev(int fd, const void* iov, int iovcnt);
long zsys_read(int fd, void* buf, __SIZE_TYPE__ size);
long zsys_readv(int fd, const void* iov, int iovcnt);
long zsys_write(int fd, const void* buf, __SIZE_TYPE__ size);
int zsys_close(int fd);
long zsys_lseek(int fd, long offset, int whence);
void zsys_exit(int return_code);
void* zsys_signal(int signum, void* sighandler);
unsigned zsys_getuid(void);
unsigned zsys_geteuid(void);
unsigned zsys_getgid(void);
unsigned zsys_getegid(void);
int zsys_open(const char* path, int flags, ...);
int zsys_getpid(void);
int zsys_clock_gettime(int clock_id, unsigned long long* timespec_ptr);
int zsys_fstatat(int fd, const char* path, void* buf, int flag);
int zsys_fstat(int fd, void* buf);
int zsys_fcntl(int fd, int cmd, ...);
void* zsys_getpwuid(unsigned uid);
int zsys_sigaction(int signum, const void* act, void* oact);
int zsys_isatty(int fd);
int zsys_pipe(int fds[2]);
int zsys_select(int nfds, void* reafds, void* writefds, void* errorfds, void* timeout);
void zsys_sched_yield(void);

/* Functions that return bool: they return true on success, false on error. All of these set errno
   on error. */
void* zthread_self(void);
unsigned zthread_get_id(void* thread);
unsigned zthread_self_id(void);
void* zthread_get_cookie(void* thread);
void* zthread_self_cookie(void);
void zthread_set_self_cookie(void* cookie);
void* zthread_create(void* (*callback)(void* arg), void* arg);
_Bool zthread_join(void* thread, void** result);
_Bool zthread_detach(void* thread);

/* Parks the thread in a queue associated with the given address, which cannot be null. The
   parking only succeeds if the condition function returns true while the queue lock is held.
  
   If condition returns false, it will unlock the internal parking queue and then it will
   return false.
  
   If condition returns true, it will enqueue the thread, unlock the parking queue lock, call
   the before_sleep function, and then it will sleep so long as the thread continues to be on the
   queue and the timeout hasn't fired. Finally, this returns true if we actually got unparked or
   false if the timeout was hit.
  
   Note that before_sleep is called with no locks held, so it's OK to do pretty much anything so
   long as you don't recursively call zpark_if(). You can call zunpark_one()/zunpark_all()
   though. It's useful to do that in before_sleep() for implementing condition variables. If you
   do call into the zpark_if recursively, you'll get a trap.
   
   Crucially, when zpark_if calls your callbacks, it is only holding the queue lock associated
   with the address, and not any other locks that the Deluge runtime uses.

   The timeout is according to the REALTIME clock on POSIX, but formatted as a double because
   this is a civilized API.

   Errors are reported by killing the shit out of your program. */
_Bool zpark_if(const void* address,
               _Bool (*condition)(void* arg),
               void (*before_sleep)(void* arg),
               void* arg,
               double absolute_timeout_in_milliseconds);

/* Simplified version of zpark_if. If the address is int-aligned, then this does a zpark_if with
   a condition that returns true if the address contains the expected value. Does nothing on
   before_sleep.
   
   This function has adorable behavior when address is misaligned. In that case, the address
   passed to zpark_if is the original misaligned address, but the rounded-down address is used for
   the comparison. This lets you use an atomic int as four notification channels.

   This matches the basic futex API except futexes would error on misaligned.

   Note that while this expects you to use an int, zpark_if has no such restriction. You could use
   any atomic word there (or words, if you're fancy). */
_Bool zcompare_and_park(const int* address, int expected_value,
                        double absolute_timeout_in_milliseconds);

/* Unparks one thread from the queue associated with the given address, and calls the given
   callback while the address is locked. Reports to the callback whether any thread got
   unparked and whether there may be any other threads still on the queue. */
void zunpark_one(const void* address,
                 void (*callback)(_Bool did_unpark_thread, _Bool may_have_more_threads, void* arg),
                 void* arg);

/* Unparks up to count threads from the queue associated with the given address, which cannot
   be null. Returns the number of threads unparked. */
unsigned zunpark(const void* address, unsigned count);

#endif /* DELUGE_STDFIL_H */

