#ifndef DELUGE_STDFIL_H
#define DELUGE_STDFIL_H

/* Opaque representation of the Deluge type. It's not possible to access the contents of a Deluge
   type except through whatever APIs we provide. */
struct ztype;
typedef struct ztype ztype;

/* Don't call these _impls directly. Any uses that aren't exactly like the ones in the #defines may 
   crash the compiler or produce a program that traps extra hard. */
void* zunsafe_forge_impl(const void* ptr, void* type_like, __SIZE_TYPE__ count);
void* zrestrict_impl(const void* ptr, void* type_type, __SIZE_TYPE__ count);
void* zalloc_impl(void* type_like, __SIZE_TYPE__ count);
void* zaligned_alloc_impl(void* type_like, __SIZE_TYPE__ alignment, __SIZE_TYPE__ count);
void* zrealloc_impl(void* old_ptr, void* type_like, __SIZE_TYPE__ count);
_Bool zcalloc_multiply(__SIZE_TYPE__ left, __SIZE_TYPE__ right, __SIZE_TYPE__ *result);

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

/* Allocates count repetitions of the given type from virtual memory that has never been pointed at
   by pointers that view it as anything other than count or more repetitions of the given type.
   The new memory is populated with a copy of the passed-in pointer. If the pointer points at more
   than count repetitions of the type, then only the first count are copied. If the pointer points
   at fewer than count repetitions of the type, then we only copy whatever it has.
   
   The pointer must point at an allocation of the same type as the one we're requesting, else this
   traps.
   
   Misuse of zrealloc/zalloc/zfree may cause logic errors where zalloc/realloc will return the same
   pointer as it had previously returned.
   
   It's not possible to misues zrealloc to cause type confusion under the Deluge P^I type system.
   
   ptr is a pointer of the given type, type is a type expression, count must be __SIZE_TYPE__ ish. */
#define zrealloc(ptr, type, count) ({ \
        type __d_temporary; \
        (type*)zrealloc_impl(ptr, &__d_temporary, (__SIZE_TYPE__)(count)); \
    })

/* Free the object starting at the given pointer.
   
   If you pass a pointer that is already freed, was never allocated, then this might trap either now
   or during any future call to zfree or zalloc.
   
   If you pass a pointer to the middle of an allocated object, then this might either free the whole
   object, or trap now, or trap at any future call to zfree or zalloc.
   
   If you free an object and then use it again, then that might be fine. But, free memory may be
   decommitted at any time (so many start to trap or suddenly become all-zero). */
void zfree(void* ptr);

/* Accessors for the bounds. */
void* zgetlower(void* ptr);
void* zgetupper(void* ptr);
ztype* zgettype(void* ptr);

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

   It's up to libc to decide if sprintf (without the z) behaves like zsprintf, or traps on OOB. */
int zvsprintf(char* buf, const char* format, __builtin_va_list args);
int zsprintf(char* buf, const char* format, ...);

int zvsnprintf(char* buf, __SIZE_TYPE__ size, const char* format, __builtin_va_list args);
int zsnprintf(char* buf, __SIZE_TYPE__ size, const char* format, ...);

/* This is like asprintf, but instead of super annoyingly returning the string in an out argument,
   it just fucking returns it in the return value like a fucking sensible function. */
char* zvasprintf(const char* format, __builtin_va_list args);
char* zasprintf(const char* format, ...);

/* This is just like printf, but does only per-call buffering. In particular, this relies on
   zvasprintf under the hood and then prints the entire string in one write(2) call (unless write
   demands that we call it again). */
void zvprintf(const char* format, __builtin_va_list args);
void zprintf(const char* format, ...);

/* This prints the given message and then shuts down the program using the same shutdown codepath
   used for memory safety violatins (i.e. it's designed to really kill the shit out of the process). */
void zerror(const char* str);

void zfence(void);

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

#endif /* DELUGE_STDFIL_H */

