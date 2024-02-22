#ifndef FILC_STDFIL_H
#define FILC_STDFIL_H

/* ztype: Opaque representation of the Fil-C type. It's not possible to access the contents of a
   Fil-C type except through whatever APIs we provide. Fil-C types are tricky, since they are
   orthogonal to bounds. Every pointer has a bounds separate from the type, and the type's job is
   to provide additional information. The only requirement is that a pointer's total range is a
   multiple of whatever type it uses, or that it makes the math of flexes work out (where there is
   an array of some type and a header).

   Fil-C types can tell you the layout of memory if you also give them a slice, or span - a tuple
   of begin and end, in bytes.

   Fil-C types by themselves know a size for the purpose of informing the internal algorithm. It
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
   
   It's valid for a filc pointer to struct dejavu to describe itself in either of the following
   ways:
   
       1. We could say that the pointer points to one dejavu.
       2. We could say that the pointer points to two pairs.
   
   The statements are equivalent under the Fil-C type system, so the runtime and compiler are free
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

   This fact about ztypes can be traced to the fact that *every* pointer access in Fil-C must
   perform a bounds check based on that pointer's lower and upper, and the lower/upper pair can't be
   faked or tricked. So, the ztype's job is to give you information that is orthogonal to the bounds.
   Therefore, ztypes are not in the business of bounds checking and always assume that your slice
   makes sense. The business of bounds checking typically happens before the ztype even gets
   consulted. */
struct ztype;
typedef struct ztype ztype;

/* Don't call these _impls directly. Any uses that aren't exactly like the ones in the #defines may 
   crash the compiler or produce a program that traps extra hard. */
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
__SIZE_TYPE__ zcalloc_multiply(__SIZE_TYPE__ left, __SIZE_TYPE__ right);
ztype* ztypeof_impl(void* type_like);

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
   
   Always zeroes newly allocated memory. There is no way to allocate memory that isn't zeroed.
   
   Crashes your program if allocation fails.
   
   Misuse of zalloc/zfree may cause logic errors where zalloc will return the same pointer as it had
   previously returned, so pointers that you expected to different objects will end up pointing at the
   same object.
   
   It's not possible to misuse zalloc/zfree to cause type confusion under the Fil-C P^I type system.
   
   type is a type expression. count must be __SIZE_TYPE__ish. */
#define zalloc(type, count) ({ \
        type __d_temporary; \
        (type*)zalloc_impl(&__d_temporary, (__SIZE_TYPE__)(count)); \
    })

#define zcalloc(type, something_to_multiply, another_thing_to_multiply) \
    zalloc(type, zcalloc_multiply((something_to_multiply), (another_thing_to_multiply)))

#define zaligned_alloc(type, alignment, count) ({ \
        type __d_temporary; \
        (type*)zaligned_alloc_impl(&__d_temporary, (__SIZE_TYPE__)(alignment), (__SIZE_TYPE__)(count)); \
    })

/* Allocates a flex object - that is, an object with a trailing array - with the given array length.

   struct_type is a type expression; this must be a struct (or a typedef for a struct). field is the
   name of a field in struct_type; this field is taken to be the trailing array. count must be
   __SIZE_TYPE__ ish.

   Misuse of this API may cause logic errors, or more likely, a situation where the returned pointer
   has a type that is incompatible with all subsequent accesses, leading to filc thwarting your
   program's execution. */
#define zalloc_flex(struct_type, field, count) ({ \
        struct_type __d_temporary; \
        __typeof__(__d_temporary.field[0]) __d_trailing_temporary; \
        (struct_type*)zalloc_flex_impl( \
            &__d_temporary, __builtin_offsetof(struct_type, field), \
            &__d_trailing_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Allocates a flex object, like zalloc_flex, but is useful for cases where the type doesn't
   actually have a flexible array member, and you'd like the flexible array to be placed after
   sizeof the type.
   
   This is not what you want if the type does have a flexible array member, regardless of whether
   it's old-school (type array[1]) or new-school (type array[]).
   
   base_type is a type expression. array_type is another type expression. count must be
   __SIZE_TYPE__ ish. */
#define zalloc_flex_cat(base_type, array_type, count) ({ \
        base_type __d_temporary; \
        array_type __d_trailing_temporary; \
        (base_type*)zalloc_flex_impl( \
            &__d_temporary, sizeof(base_type), \
            &__d_trailing_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Allocates count repetitions of the given type from virtual memory that has never been pointed at
   by pointers that view it as anything other than count or more repetitions of the given type.
   The new memory is populated with a copy of the passed-in pointer. If the pointer points at more
   than count repetitions of the type, then only the first count are copied. If the pointer points
   at fewer than count repetitions of the type, then we only copy whatever it has.
   
   The pointer must point at an allocation of the same type as the one we're requesting, else this
   traps. See the rules for zfree for all of the checks performed on the old pointer.
   
   In addition to copying data from the old allocation, the new memory is zeroed. So, if you use
   zrealloc to grow an array, then the added elements will start out zero.
   
   Misuse of zrealloc/zalloc/zfree may cause logic errors where zalloc/realloc will return the same
   pointer as it had previously returned.
   
   It's not possible to misuse zrealloc to cause type confusion under the Fil-C P^I type system.
   
   ptr is a pointer of the given type, type is a type expression, count must be __SIZE_TYPE__ ish. */
#define zrealloc(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zrealloc_impl(ptr, &__d_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Free the object starting at the given pointer.
   
   If you pass NULL, this silently returns.
   
   If you pass a pointer where lower is NULL, this thwarts your program's stupidity by panicking.
   
   If you pass a pointer that is offset from lower (i.e. ptr != lower), this panicks your dumb program.
   
   If you pass a pointer that has an opaque type (like the type type, the function type, or filc
   runtime types like the ones used to implement zthread APIs), this kills the shit out of your
   program.
   
   If you pass a pointer that is already freed or was never allocated, then this might trap either now
   or during any future call to zfree or zalloc. Note that the state where a trap is coming but hasn't
   happened is not a weird state for the allocator; it's just buffering up the part where it will free
   the object, and when that happens, the allocator will deterministically and well-defindedly either
   identify an object it had allocated that it will now free, or it will kill the shit out of your
   program.
   
   If you pass a pointer to the middle of an allocated object even when ptr == lower (which is still
   possible by zrestricting and then zfreeing), then this will trap.
   
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
        __typeof__((ptr) + 0) __d_ptr = (ptr); \
        (__typeof__((ptr) + 0))zgetupper(__d_ptr) - __d_ptr; \
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

/* Allocate a zero-initialized object that is exactly like the one passed in from the standpoint of
   the Fil-C type system. The new object will have the same type as what the obj pointer points to
   and the same lower/upper bounds. This information comes from the pointer itself, so if you had
   zrestricted the pointer or the pointer got borked, this will reflect in the allocated object. As
   well, the resulting pointer will point at the same offset from lower as the obj pointer you
   passed in. For example, if you pass in a pointer to the middle of an array, this will allocate
   an array and return a pointer to the middle of it, offset in the same way as the original.

   zalloc_like() is most useful if you want to describe the type+size in a simple-to-carry-around
   kind of way. In particular, if you want to describe the type+size of something in a const
   initializer, then using a pointer to a "default" instance of that thing is the simplest way to
   do it. This should be seen as a deficiency in Fil-C, but it's one we can live with for now.
   
   Both the OpenSSL and Curl patches have some variant of this idiom (read this as a unified diff
   snippet reflecting changes needed for Fil-C):
   
       +static const foo_bar_type foo_bar_type_prototype;
        static const type_info foo_bar_type_info = {
            ... (some stuff),
       +    &foo_bar_type_prototype
       -    sizeof(foo_bar_type)
        };

   In the original code, the size was used to tell some polymorphic allocator how big the foo_bar
   object was going to be, so they can pass that to malloc. But that's not enough for Fil-C. So,
   instead, we pass around a const void* prototype, and the polymorphic allocator now calls
   zalloc_like(type_info->prototype) instead of malloc(type_info->size).
   
   In the future, you'd have been able to say ztypeof(foo_bar_type) in the static initializer, but:
       1. That's not as convenient, since you still need to pass around the size!
       2. You can't do it today, because clang's and llvm's notion of constexpr strongly rejects
          everything about ztypeof(). A nontrivial amount of compiler surgery will be needed to
          make that possible. Someday, maybe.

   Ideally, there would be a way to carry around the type+size and put it in a const initializer.
   Basically, a neatly-packaged tuple of ztype* and size, which would then constitute a Fil-C
   dynamic equivalent of a C type. That thing would be a great replacement for prototypes. If we
   built it today, then it would solve problem (1) above, but not problem (2). Ergo, we shall
   proceed undaunted with prototypes and zalloc_like()! */
void* zalloc_like(void* obj);

/* Allocates a new string (with zalloc(char, strlen+1)) and prints a dump of the type to that string.
   Returns that string. You have to zfree the string when you're done with it.

   This is exposed as %T in the zprintf family of functions. */
char* ztype_to_new_string(ztype* type);

/* Allocates a new string (with zalloc(char, strlen+1)) and prints a dump of the ptr to that string.
   Returns that string. You have to zfree the string when you're done with it.

   This is exposed as %P in the zprintf family of functions. */
char* zptr_to_new_string(const void* ptr);

/* Low-level printing functions. These might die someday. They are useful for Fil-C's own tests. They
   print directly to stdout using write(). They are safe (passing an invalid ptr to zprint() will trap
   for sure, and it will never print out of bounds even if there is no null terminator). */
void zprint(const char* str);
void zprint_long(long x);
void zprint_ptr(const void* ptr);

/* Low-level functions that should be provided by libc, which lives above this. These are exposed for
   the purpose of Fil-C's own snprintf implementation, which lives below libc. They are also safe to
   call instead of what libc offers. */
__SIZE_TYPE__ zstrlen(const char* str);
int zisdigit(int chr);

/* This is almost like sprintf, but because Fil-C knows the upper bounds of buf, this actually ends
   up working exactly like snprintf where the size is upper-ptr. Hence, in Fil-C, it's preferable
   to call zsprintf instead of zsnprintf.

   In libc, sprintf (without the z) behaves kinda like zsprintf, but traps on OOB.

   The main difference from the libc sprintf is that it uses a different implementation under the hood.
   This is based on the samba snprintf, origindally by Patrick Powell, but it uses the zstrlen/zisdigit/etc
   functions rather than the libc ones, and it has some additional features:

       - '%P', which prints the full filc_ptr (i.e. 0xptr,0xlower,0xupper,type{thingy}).
       - '%T', which prints the filc_type or traps if you give it anything but a ztype.

   It's not obvious that this code will do the right thing for floating point formats. But this code is
   pizlonated, so if it goes wrong, at least it'll stop your program from causing any more damage. */
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

       - '%P', which prints the full filc_ptr (i.e. 0xptr,0xlower,0xupper,type{thingy}).
       - '%T', which prints the filc_type or traps if you give it anything but a ztype.

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

/* Currently, the compiler builtins for ptr CAS don't work for silly clang reasons. So, Fil-C
   offers these functions instead.

   But pointers are 32 bytes, you say! How can they be atomic, you say! What the fuck is going on!

   Fil-C pointers comprise a sidecar and a capability. The capability always stores the pointer
   itself and may store other information, too. The sidecar stores additional information, and may
   also store the pointer, or not. The system is complex, but is arranged so that:
   
   - If you ever get a sidecar from one pointer and a capability from another, then the resulting
     mix - the "borked ptr" - offers no escape from memory safety. Instead, it might trap in cases
     where a normal pointer wouldn't have.
     
   - If the capability corresponds to a pointer that points to the base of an allocation, then
     the sidecar is always ignored, and so it doesn't matter if it matches or not. The full
     ptr, lower, upper, type combo is encoded in the capability (since ptr = lower). So, racing
     with pointers to the base of things always works!
   
   - If the sidecar and capability really did correspond to the same pointer - i.e. it's not
     borked at all - and it wasn't to the base of the allocation, then the sidecar and capability
     combine to give you a precise and sound ptr, lower, upper, type combo.
   
   - If the capability corresponds to an out-of-bounds pointer (ptr < lower and ptr > upper; that's
     right, I said > not >=), then the borked ptr might just be a boxed int (i.e. it will think
     it points at nothing and all access will trap).
   
   - If both the capability and the sidecar corresponded to an out-of-bounds pointer, then the
     borked pointer will point wherever the capability claimed, but will have the capability
     (lower bounds, upper bounds, and type) from the sidecar. So, all accesses are likely to trap.
     Or, you could arithmetic your way in-bounds again, to a place you could have been pointing at
     the normal way were it not for the race.
   
   - If the capability corresponds to a pointer to the inside of an object or array of objects,
     but *not* to the base of a flex object, then a mismatched sidecar is guaranteed to result
     in the borked ptr's lower bounds snapping up towards the ptr, subject to the
     (upper - lower) = k * type->size rule. The upper bound and type are retained precisely. So,
     pointing into the middle of an object with nontrivial type will result in the lower bound
     snapping up to the base of that object, not the interior pointer. Snapping the lower bound
     up is memory-safe but it might cause your program to trap if you wanted to subtract from
     the pointer and then access it.
   
   - If the capability corresponds to a pointer to the inside of the base of a flex object,
     then the borked ptr will forget the flex array's bounds (the upper bound will snap down as
     if the flex had no array). Note that you have to point past the beginning of the flex, and
     not into the flex's array, for borked ptrs to have this effect.
   
   How do you avoid getting borked ptrs? Don't race on pointers.
   
   But what if you want to race on pointers? It's cool! Just make sure it's in bounds, or better
   yet, points at the base of something. Then borked ptrs will work just fine for you.
   
   Atomic ops always bork the ptr by storing 0 into the sidecar. This helps remind you what's
   happening in your racy code, if that code also involves CASing and such, which it usually
   does. But if you're racing so hard you don't even CAS, then you get to discover the borking
   in a probabilistic sort of way.
   
   This means that atomic ops just require a 128-bit CAS and a 128-bit store (and the store
   to the sidecar doesn't need any fancy ordering). Normal loads/stores of ptrs use 128-bit
   atomic loads/stores.

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
   with the address, and not any other locks that the Fil-C runtime uses.

   The timeout is according to the REALTIME clock on POSIX, but formatted as a double because
   this is a civilized API. Use positive infinity (aka 1. / 0.) if you just want this to wait
   forever.

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

/* Returns true if running in the build of the runtime that has extra (super expensive) testing
   checks.

   This is here so that the test suite can assert that it runs with testing asserts enabled. */
_Bool zis_runtime_testing_enabled(void);

/* Returns a pointer that has the sidecar from the first pointer and the capability from the
   second pointer. This simulates any kind of race you like. If the sidecar and capability are
   mismatched, then the resulting pointer will ignore the sidecar.
   
   This is here so that the test suite can simulate pointer races.
   
   There is no way to use this API in a memory-unsafe way. If you find a way to create a more
   powerful capability by using this API, then it's a Fil-C bug and we should fix it. */
void* zborkedptr(void* sidecar, void* capability);

/* Asks Fil-C to run additional pointer validation on this pointer. If memory safety holds, then
   these checks will succeed. If they don't, then it's a Fil-C bug, and we should fix it. It could
   be a real bug, or it could be a bug in the validation checks. They are designed to be hella strict
   and maybe I made them too strict.
   
   If you run with pizfix/lib_test in your library path, then this check happens in a bunch of
   random places anyway (and that's the main reason why the lib_test version is so slow). */
void zvalidate_ptr(void* ptr);

/* Suspend the scavenger. If the scavenger is suspended, then free pages are not returned to the OS.
   This is intended to be used only for testing. */
void zscavenger_suspend(void);
void zscavenger_resume(void);

/* ------------------ All APIs below here are intended for libc consumption ------------------------- */

/* These APIs are memory-safe, so you won't escape the filc by using them. But it's not clear to me to
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
int zsys_socket(int domain, int type, int protocol);
int zsys_setsockopt(int sockfd, int level, int optname, const void* optval, unsigned optlen);
int zsys_bind(int sockfd, const void* addr, unsigned addrlen);
int zsys_getaddrinfo(const char* node, const char* service, const void* hints, void** res);
int zsys_connect(int sockfd, const void* addr, unsigned addrlen);
int zsys_getsockname(int sockfd, void* addr, unsigned* addrlen);
int zsys_getsockopt(int sockfd, int level, int optname, void* optval, unsigned* optlen);
int zsys_getpeername(int sockfd, void* addr, unsigned* addrlen);
long zsys_sendto(int sockfd, const void* buf, __SIZE_TYPE__ len, int flags,
                 const void* addr, unsigned addrlen);
long zsys_recvfrom(int sockfd, void* buf, __SIZE_TYPE__ len, int flags,
                   void* addr, unsigned* addrlen);
int zsys_getrlimit(int resource, void* rlim);
unsigned zsys_umask(unsigned mask);
int zsys_uname(void* buf);

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

#endif /* FILC_STDFIL_H */

