#ifndef FILC_STDFIL_H
#define FILC_STDFIL_H

#ifdef __cplusplus
extern "C" {
#endif

#if 0
} /* tell emacs what's up */
#endif

void* zalloc(__SIZE_TYPE__ count);
void* zaligned_alloc(__SIZE_TYPE__ alignment, __SIZE_TYPE__ count);
void* zrealloc(void* old_ptr, __SIZE_TYPE__ count);
void* zaligned_realloc(void* old_ptr, __SIZE_TYPE__ alignment, __SIZE_TYPE__ count);
void zfree(void* ptr);

/* Accessors for the bounds.
 
   The lower and upper bounds have the same capability as the incoming ptr. So, if you know that a
   ptr points into the middle of struct foo and you want to get to the base of struct foo, you can
   totally do:

       struct foo* foo = (struct foo*)zgetlower(ptr);

   Or if you know that ptr points to an array of struct foos, and you want to get a pointer to the
   last one:
 
       struct foo* foo = (struct foo*)zgetupper(ptr) - 1;
       
   In both cases, the pointer is usable provided that the bounds are big enough for struct foo and
   that the type is compatible with struct foo. */
void* zgetlower(void* ptr);
void* zgetupper(void* ptr);

/* Get the pointer's array length, which is the distance to upper in units of the ptr's static type. */
#define zlength(ptr) ({ \
        __typeof__((ptr) + 0) __d_ptr = (ptr); \
        (__typeof__((ptr) + 0))zgetupper(__d_ptr) - __d_ptr; \
    })

/* Tells if the pointer is in bounds of lower/upper. This is not a guarantee that accesses will
   succeed, since this does not check type. For example, valid function pointers are zinbounds but
   cannot be "accessed" regardless of type (can only be called if in bounds). */
_Bool zinbounds(void* ptr);

/* Tells if a value of the given size is in bounds of the pointer. */
_Bool zvalinbounds(void* ptr, __SIZE_TYPE__ size);

/* Returns true if the pointer points to a byte with unset type. */
_Bool zisunset(void* ptr);

/* Returns true if the pointer points at an integer byte.
   
   If this returns false, then the pointer may point to a pointer, or be unset, or to opaque memory,
   or to any other type when we add more types.
 
   Pointer must be in bounds, else your process dies. */
_Bool zisint(void* ptr);

/* Returns the pointer phase of the pointer.
   
   - 0 means this points to the base of a pointer.
   
   - 1..31 (inclusive) means you're pointing into the middle of a pointer; so, you subtract that many
     bytes from your pointer, then you'll be able to dereference it.
   
   - -1 means that this does not point to a pointer at all. This means it could mean that it's an int
     or it could be unset or it could mean opaque memory. (Or any other type when we add more types.)
     
   Pointer must be in bounds, else your process dies. */
int zptrphase(void* ptr);

/* Returns true if the pointer points at any kind of pointer memory. Equivalent to
   isptrphase(p) != -1. */
_Bool zisptr(void* ptr);

/* Returns true if the pointer points at pointers or integers.
 
   New types, as well as opaque memory, will return false. */
_Bool zisintorptr(void* ptr);

/* Construct a pointer that has the capability from `object` but the address from `address`. This
   is a memory-safe operation, and it's guaranteed to be equivalent to:
   
       object -= (uintptr_t)object;
       object += address;
   
   This is useful for situations where you want to use part of the object's address for tag bits. */
void* zmkptr(void* object, unsigned long address);

/* Memory-safe helpers for doing bit math on addresses. */
void* zorptr(void* ptr, unsigned long bits);
void* zandptr(void* ptr, unsigned long bits);
void* zxorptr(void* ptr, unsigned long bits);

/* Returns a pointer that points to `newptr` masked by the `mask`, while preserving the
   bits from `oldptr` masked by `~mask`. Also asserts that `newptr` has no bits in `~mask`.
   
   Useful for situations where you want to reassign a pointer from `oldptr` to `newptr` but
   you have some kind of tagging in `~mask`. */
void* zretagptr(void* newptr, void* oldptr, unsigned long mask);

/* The pointer-nullifying memmove.
   
   This memmove will kill your process if anything goes out of bounds.
   
   But on pointers (either destination thinks the byte is a pointer or the source thinks the byte is
   a pointer), the value copied is zero.
   
   For example, if you call this to copy pointers to ints, those ints will become zero.
   
   Or if you call this to copy ints to pointers, those pointers will become zero.

   Also if you copy pointers to pointers, then zero will be copied.

   But if you copy ints to ints, then the actual bytes are copied. */
void zmemmove_nullify(void* dst, const void* src, __SIZE_TYPE__ count);

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
void zatomic_store_ptr(void** ptr, void* new_value);
void zunfenced_atomic_store_ptr(void** ptr, void* new_value);
void* zatomic_load_ptr(void** ptr);
void* zunfenced_atomic_load_ptr(void** ptr);

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
   
   This is signal-safe, but you're on your own if you park on something in a signal handler that
   can only be unparked by the thread that the handler interrupted. Also, to make that work, the
   condition callback is called with signals blocked. This is fine, since if you use that
   function to do unbounded work, then you run the risk of blocking the whole program (since the
   parking lot is holding a queue lock for the bucket for that address, which may be used by any
   number of other addresses).

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
   any atomic word there (or words, if you're fancy).

   This is signal-safe. */
_Bool zcompare_and_park(const int* address, int expected_value,
                        double absolute_timeout_in_milliseconds);

/* Unparks one thread from the queue associated with the given address, and calls the given
   callback while the address is locked. Reports to the callback whether any thread got
   unparked and whether there may be any other threads still on the queue.

   This is signal-safe. But, that implies that the callback is called with signals blocked. That's
   fine, since you have to avoid unbounded work in that function anyway, since it's called with
   the bucket lock held, an the bucket lock may be shared between your address and any number of
   other addresses. */
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
   replacement for musl's syscall layer, and they're very much tailored to my hacks to musl.

   Not all of these are syscalls! POSIX systems have many system functions, like opendir and getaddrinfo,
   that are not system calls but cannot be implemented portably any other way. */

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
int zsys_getitimer(int which, void* curr_value);
int zsys_setitimer(int which, const void* new_value, void* old_value);
int zsys_pause(void);
int zsys_pselect(int nfds, void* readfds, void* writefds, void* exceptfds, const void* timeout,
                 const void* sigmask);
int zsys_getpeereid(int fd, unsigned* uid, unsigned* gid);
int zsys_kill(int pid, int sig);
int zsys_raise(int sig);
int zsys_dup(int fd);
int zsys_dup2(int oldfd, int newfd);
int zsys_sigprocmask(int how, const void* set, void* oldset); /* This is pthread_sigmask, but sets
                                                                 errno and returns -1 on error. */
void* zsys_getpwnam(const char* name);
int zsys_setgroups(__SIZE_TYPE__ size, const unsigned* list);
void* zsys_opendir(const char* name);
void* zsys_fdopendir(int fd);
int zsys_closedir(void* dirp);
void* zsys_readdir(void* dirp);
void zsys_rewinddir(void* dirp);
void zsys_seekdir(void* dirp, long loc);
long zsys_telldir(void* dirp);
int zsys_dirfd(void* dirp);
void zsys_closelog(void);
void zsys_openlog(const char* ident, int option, int facility);
int zsys_setlogmask(int mask);
void zsys_syslog(int priority, const char* msg); /* formatting is up to libc and thankfully musl's printf
                                                    always just supports %m! */
int zsys_chdir(const char* path);
int zsys_fork(void); /* This currently panics if threads had ever been created. But that's only because
                        I haven't yet had to deal with a multithreaded forker. */
int zsys_waitpid(int pid, int* status, int options);
int zsys_listen(int sockfd, int backlog);
int zsys_accept(int sockfd, void* addr, unsigned* addrlen);
int zsys_socketpair(int domain, int type, int protocol, int* sv);
int zsys_setsid(void);
int zsys_execve(const char* pathname, char*const* argv, char*const* envp);
int zsys_getppid(void);
int zsys_chroot(const char* path);
int zsys_setuid(unsigned uid);
int zsys_seteuid(unsigned uid);
int zsys_setreuid(unsigned ruid, unsigned euid);
int zsys_setgid(unsigned gid);
int zsys_setegid(unsigned gid);
int zsys_setregid(unsigned rgid, unsigned egid);
int zsys_nanosleep(const void* req, void* rem);
int zsys_getgroups(int size, unsigned* list);
int zsys_getgrouplist(const char* user, unsigned group, unsigned* groups, int* ngroups);
int zsys_initgroups(const char* user, unsigned gid);
long zsys_readlink(const char* path, char* buf, __SIZE_TYPE__ bufsize);
int zsys_openpty(int* masterfd, int* slavefd, char* name, const void* term, const void* win);
int zsys_ttyname_r(int fd, char* buf, __SIZE_TYPE__ buflen);
void* zsys_getgrnam(const char* name);
int zsys_chown(const char* pathname, unsigned owner, unsigned group);
int zsys_chmod(const char* pathname, unsigned mode);
void zsys_endutxent(void);
void* zsys_getutxent(void);
void* zsys_getutxid(const void* utmpx);
void* zsys_getutxline(const void* utmpx);
void* zsys_pututxline(const void* utmpx);
void zsys_setutxent(void);
void* zsys_getlastlogx(unsigned uid, void* lastlogx);
void* zsys_getlastlogxbyname(const char* name, void* lastlogx);
long zsys_sendmsg(int sockfd, const void* msg, int flags);
long zsys_recvmsg(int sockfd, void* msg, int flags);
int zsys_fchmod(int fd, unsigned mode);
int zsys_rename(const char* oldname, const char* newname);
int zsys_unlink(const char* path);
int zsys_link(const char* oldname, const char* newname);
long zsys_sysconf_override(int name); /* If libpizlo wants to override this value, it returns
                                         it; otherwise it returns -1 with ENOSYS. Many sysconfs
                                         are handled by musl! */
int zsys_numcores(void);
void* zsys_mmap(void* address, __SIZE_TYPE__ length, int prot, int flags, int fd, long offset);
int zsys_munmap(void* address, __SIZE_TYPE__ length);
int zsys_ftruncate(int fd, long length);

void* zthread_self(void);
unsigned zthread_get_id(void* thread);
unsigned zthread_self_id(void);
void* zthread_get_cookie(void* thread);
void* zthread_self_cookie(void);
void zthread_set_self_cookie(void* cookie);
void* zthread_create(void* (*callback)(void* arg), void* arg); /* returns NULL on failure, sets
                                                                  errno. */
_Bool zthread_join(void* thread, void** result); /* Only fails with ESRCH for forked threads. Returns
                                                    true on success, false on failure and sets errno. */

#ifdef __cplusplus
}
#endif

#endif /* FILC_STDFIL_H */

