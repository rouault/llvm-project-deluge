#ifndef DELUGE_STDFIL_H
#define DELUGE_STDFIL_H

/* Don't call these _impls directly. Any uses that aren't exactly like the ones in the #defines may 
   crash the compiler or produce a program that traps extra hard. */
void* zunsafe_forge_impl(const void* ptr, void* type_like, __SIZE_TYPE__ count);
void* zrestrict_impl(const void* ptr, void* type_type, __SIZE_TYPE__ count);
void* zalloc_impl(void* type_like, __SIZE_TYPE__ count);

/* Unsafely creates a pointer that will claim to point at count repetitions of the given type.
   
   It's super awesome to never use this.
   
   This is the only escape hatch.
   
   ptr can be anything castable to const void*. type is a type expression. count must be __SIZE_TYPE__ ish. */
#define zunsage_forge(ptr, type, count) ({ \
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

/* Free the object starting at the given pointer.
   
   If you pass a pointer that is already freed, was never allocated, then this might trap either now
   or during any future call to zfree or zalloc.
   
   If you pass a pointer to the middle of an allocated object, then this might either free the whole
   object, or trap now, or trap at any future call to zfree or zalloc.
   
   If you free an object and then use it again, then that might be fine. But, free memory may be
   decommitted at any time (so many start to trap or suddenly become all-zero). */
void zfree(void* ptr);

/* Low-level printing functions. These might die someday. They are useful for Deluge's own tests. They
   print directly to stdout using write(). They are safe (passing an invalid ptr to zprint() will trap
   for sure, and it will never print out of bounds even if there is no null terminator). */
void zprint(const char* str);
void zprint_long(long x);

void zerror(const char* str);

#endif /* DELUGE_STDFIL_H */

