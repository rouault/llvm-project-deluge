# The Fil-C Memory Safety Manifesto: FUGC Yeah!

The C and C++ programming languages are wonderful. There is a ton of amazing code written in both of
them. But C and C++ are unsafe languages. Simple logic errors may result in an attacker controlling
where a pointer points and what is written into it, which leads to an easy path to exploitation. Lots
of other languages (Rust, Java, Haskell, even JavaScript) don't have this problem!

But I love C. And I love C++ almost as much. I grew up on them. It's such a joy for me to
use both of them! Therefore, in my spare time, I decided
to make my own memory-safe C and C++. This is a personal project and an expression of my love for C.

Fil-C introduces memory safety at the core of C and C++:
 
- All pointers carry a *capability*, which tracks the bounds and type of the pointed-to memory. Fil-C
  use a novel pointer encoding called *MonoCap*, which is a 16-byte atomic tuple of object pointer
  and raw pointer. The *object* contains the lower and upper bounds and dynamic type information for
  each 16 byte word in the payload. Accessing memory causes a bounds check and a type check. Type
  checks do dynamic type inference but disallow ping-ponging (once a word becomes an integer, it cannot
  become pointer, or vice-versa).

- All allocations are *garbage collected* using
  [FUGC](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/fugc.c) (Fil's
  Unbelievable Garbage Collector). FUGC is a concurrent, real time, accurate garbage collector.
  Threads are never suspended for GC. Freeing an object causes all word types to transition to *free*,
  which prevents all future access. FUGC will redirect all object pointers to free objects to the free
  singleton object, which ensures that freed objects are definitely collected on the next cycle.
  Accessing a freed object before or after the next GC is guaranted to trap. Also, freeing objects is
  optional.

- The combination of MonoCaps and FUGC means that it's not necessary to instrument or change `malloc`
  and `free` calls; the semantics are compatible with C. It's also not necessary to change unions and
  the active union member rule only results in traps if it results in int-pointer confusion.

- The combination of MonoCaps and FUGC means that pointer capabilities cannot be forged. Your
  program may have logic errors (bad casts, bogus pointer arithmetic, races, bad frees, use-after-free,
  whatever)
  but every pointer will remember the bounds and type of the thing it originated from. If you break
  that pointer's rules by trying to access out-of-bounds, or read an int as a pointer or vice-versa, or
  access a freed object, Fil-C will thwart your program's further execution.

- Fil-C supports tricky features like pthreads, signal handlers, mmap, C++ exceptions (which implies
  libunwind), and setjmp/longjmp. All of these features are memory-safe. Because Fil-C pointers are
  atomic, lock-free algorithms using pointers work just fine. It's even possible to allocate memory
  using `malloc` from within a signal handler (which is necessary because Fil-C heap-allocates stack
  allocations).

- Fil-C's protections are designed to be comprehensive. There's no escape hatch short of [delightful
  hacks that also break all memory-safe languages](https://blog.yossarian.net/2021/03/16/totally_safe_transmute-line-by-line).
  There's no `unsafe` keyword. Even data races result in memory-safe outcomes. Fil-C operates on the
  GIMSO principle: Garbage In, Memory Safety Out! No program accepted by the Fil-C compiler can
  possibly go on to escape out of the Fil-C type system. At worst, the program's execution will be
  thwarted at runtime by Fil-C.

Fil-C is already powerful enough to run a [memory-safe curl](https://github.com/pizlonator/deluded-curl-8.5.0)
and a [memory-safe OpenSSH (both client and server)](https://github.com/pizlonator/deluded-openssh-portable)
on top of a [memory-safe OpenSSL](https://github.com/pizlonator/deluded-openssl-3.2.0),
[memory-safe zlib](https://github.com/pizlonator/deluded-zlib-1.3),
[memory-safe pcre](https://github.com/pizlonator/pizlonated-pcre-8.39) (which required no changes),
[memory-safe CPython](https://github.com/pizlonator/pizlonated-cpython) (which required some changes
and even [found a bug](https://github.com/python/cpython/issues/118534)),
[memory-safe SQLite](https://github.com/pizlonator/pizlonated-sqlite),
[memory-safe libcxx and libcxxabi](https://github.com/pizlonator/llvm-project-deluge/tree/deluge), and
[memory-safe musl](https://github.com/pizlonator/deluded-musl) (Fil-C's current libc). This works for
me on my Linux X86_64 box:

    pizfix/bin/curl https://www.google.com/

as does this:

    pizfix/bin/ssh user@some.server.com

Where the `pizfix` is the Fil-C staging environment for *pizlonated* programs (programs that now
successfully compile with Fil-C). The only unsafety in Fil-C is in libpizlo (the runtime library),
which exposes all of the API that musl needs (low-level
[syscall and thread primitives](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/filc_runtime.c#2901),
which themselves perform comprehensive safety checking).

On the other hand, Fil-C is quite slow. It's ~50x slower than legacy C right now. I have not done any
optimizations to it at all. I am focusing entirely on correctness and ergonomics and converting as
much code to it as I personally can in my spare time. It's important for Fil-C to be fast eventually,
but it'll be easiest to make it fast once there is a large corpus of code that can run on it.

Note that the very first prototype of Fil-C used isoheaps instead of GC. The isoheap version is obsolete,
since it's slower and requires more changes to C code. If you want to read about it,
[see here](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/Manifesto-isoheaps-old.md).

This document goes into the details of Fil-C and is organized as follows. First, I show you how to
use Fil-C. Then, I describe the Fil-C development plan, which explains my views on growing the set
of things Fil-C can run and how to make it run fast. The section about making it run fast also delves
into a lot of technical details about how Fil-C works. Then I conclude with a description of the
FUGC and MonoCap algorithms.

## Using Fil-C

First I'll tell you how to build Fil-C and then I'll tell you how to use it.

### Building Fil-C

Fil-C currently only works on Linux/X86_64. Upon getting Fil-C from
https://github.com/pizlonator/llvm-project-deluge.git, and making sure you're on the `deluge` branch,
simply do:

    ./setup_gits.sh
    ./build_all.sh

This will build memory-safe musl, zlib, OpenSSL, curl, OpenSSH, and pcre. Now you can try to download
something with the pizlonated curl, like maybe:

    pizfix/bin/curl https://www.google.com/

Or fire up a memory-safe sshd:

    sudo $PWD/pizfix/sbin/sshd -p 10022

And then even connect to it:

    pizfix/bin/ssh -p 10022 localhost

You'll probably encounter bugs.

### Using Fil-C

Let's start with the basics. Fil-C works like any C compiler. Take this program:

    #include <stdio.h>
    int main() { printf("Hello!\n"); return 0; }

Say it's named hello.c. We can do:

    build/bin/clang -o hello hello.c -g -O

Note that without -g, the Fil-C runtime errors will not be as helpful, and if you don't add -O to
-g, the compiler will currently crash.

Let's quickly look at what happens with a broken program:

    #include <stdio.h>
    int main() {
        int x;
        printf("memory after x = %d\n", (&x)[10]);
        return 0;
    }

Here's what happens when we compile and run this:

    [pizlo@behemoth llvm-project-deluge] build/bin/clang -o bad bad.c -O -g
    [pizlo@behemoth llvm-project-deluge] ./bad
    filc safety error: cannot access pointer with ptr >= upper (ptr = 0x10a4104c8,0x10a4104a0,0x10a4104b0,_).
        bad.c:4:41: main
        <crt>: main
    [62394] filc panic: thwarted a futile attempt to violate memory safety.
    zsh: trace trap  ./bad

Fil-C thwarted this program's attempt to do something bad. Hooray!

The Fil-C version of clang works much like normal clang. It accepts C code and produces .o files
that your linker will understand. Some caveats:

- Fil-C currently relies on the staging environment (where all the pizlonated libraries, like OpenSSL
  and friends, live) to be in the llvm-project source directory.

- Fil-C currently relies on you *not* installing the compiler. You have to use it directly from the
  build directory created by the build_all.sh script.

- The [`llvm::FilPizlonatorPass`](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/llvm/lib/Transforms/Instrumentation/FilPizlonator.cpp)
  uses assert() as its error checking for now, so you must compile llvm with assertions enabled (the
  build_all.sh script does this).

Fil-C requires almost no changes to C or C++ code. Inline assembly is currently disallowed. Some configure
script jank has to change. Other than that, I only had to make a couple one-line changes in OpenSSL
and OpenSSH to get them to work.

## The Fil-C Development Plan

Lots of projects have attempted to make C memory-safe. I've worked on some of them. Usually, the
development plan starts with the truism that C has to be fast. This leads to antipatterns creeping
in from the start.

Premature optimization really is the root of all evil! In the case of new language bringup, or the
bringup of a radically new way of compiling an existing language, premature optimizations are
particularly evil because:

- If your language doesn't run anything yet, then you cannot meaningfully test any of your
  optimizations. Therefore, fast memory-safe C prototypes are likely to only be fast on whatever
  tiny corpus of tests that prototype was built to run. Worse, it's likely that they can only run
  whatever corpus they were optimized for and nothing else! In practice, it leads to
  abandonware like [CCured](https://people.eecs.berkeley.edu/~necula/Papers/ccured_toplas.pdf)
  and [SoftBound](https://llvm.org/pubs/2009-06-PLDI-SoftBound.html) or solutions that are not
  practical to deploy widely because they require new hardware like
  [CHERI](https://www.cl.cam.ac.uk/research/security/ctsrd/cheri/). In other words, we already have
  multiple fast memory-safe Cs, but they require compilers you can't run or hardware you can't buy.

- Working on a language implementation that is optimized is harder than working on one that is
  designed for simplicity. When bringing up a new language and growing its corpus, it's important
  to have the flexibility to make lots of changes, including to the language itself and fundamental
  things about how it's compiled or interpreted. Therefore, prematurely optimizing a language
  implementation makes it harder to grow the language's corpus. The moment you encounter something
  you didn't expect in some new program, you'll have to not just deal with conceptual complexity of
  the idiom you encountered, but the implementation complexity of all your optimizations.

It's always possible to optimize later. "My software runs correctly but too slowly" is not as
bad of a problem as "my software is fast but wrong" or "my software is fast but riddled with
security bugs".

Therefore, my development plan for Fil-C is all about deliberately avoiding any performance work
until I have a large corpus of C code that works in Fil-C. Once that corpus exists, it will make
sense to start optimizing. But until that corpus exists, I want to have the flexibility of
rewriting major pieces of the Fil-C compiler and runtime to support whatever kind of weird C
idioms I encounter as I grow the corpus.

The rest of this section describes the next two phases of the development plan: growing the corpus
and then making it super fast.

### Growing the Corpus

Fil-C can already run a bunch of stuff, but not enough to be able to do meaningful optimizations, yet.
My goal is to grow the corpus until I have a small UNIX-like userland that comprises only pizlonated
programs.

Corpus growth should proceed as follows:

- First get to at least 10 large, real-world C libaries or programs compiling with Fil-C.
  I don't consider musl to be part of the corpus,
  since I'm making lots of internal changes to it (and I'm willing to even completely rewrite it
  if it makes adding more programs easier). I'm close to this goal already, depending on whether
  you believe that the things I've gotten to work actually work well enough for meaningful performance
  benchmarking.

- Then add C++ support and add at least 10 large, real-world C++ libraries or programs.

Once we have such a corpus, then it'll make sense to start thinking about some optimizations.

Even after the corpus grows to 10 C programs and 10 C++ programs, we will still want to keep
growing the corpus. But hopefully, it'll get easier to grow the corpus as the corpus grows. For
example, it's still the case that adding new code trips cases where some musl syscall isn't
implemented in libpizlo. Eventually musl+libpizlo will know all the syscalls.

### Making Fil-C Fast

The biggest impediment to using Fil-C in production is speed. Fil-C is currently about 50x slower
than legacy C according to my tests (good old [Richards](https://www.cl.cam.ac.uk/~mr10/Bench.html),
zlib's minigzip, and OpenSSL's `enc` command are roughly in that ballpark).

Why is it so slow? Answering this question also gives us a fun way to talk about Fil-C's technical
details. So, this section will simultaneously explain how Fil-C works today and how it would work
if I cared about performance. And I won't care about performance until I've got a corpus!

First of all, performance is a deliberate non-goal of the current implementation. It's super hard
to make a C compiler widen all pointers to 16 bytes, align them to 16 bytes, handle accurate
concurrent GC, and then transform
all of the code in a way that allows zero unsafety through. It's hard to even reason about whether
the chosen transformation strategy obeys the compiler's own laws let alone whether it's sound. To
make it easy for me to feel confident that I was doing the right thing when writing the runtime
and `llvm::FilPizlonatorPass`, I consistently went for implementation tactics that are dead simple
to get right, reason about, and test. For example:

- Every local variable is currently heap-allocated without even the dumbest escape analysis. This
  is shockingly easy to fix and is likely to change the performance by an order of magnitude, if I
  cared!

- The Fil-C ABI currently has the caller allocate a buffer in the heap to store the
  arguments. The callee deallocates the argument buffer. This makes dealing with `va_list` (and
  all of the ways it could be misused) super easy. But, it wouldn't be hard to change the ABI to
  have the caller stack-allocate the buffer and then have the caller heap-allocate a clone if
  it finds itself needing to `va_start`.

- The code for doing checks on pointer access has almost no fast path optimizations and sometimes
  does hard math like modulo. Other parts of the runtime are similarly written to just get it right.

- `llvm::FilPizlonatorPass` is jammed into the very beginning of clang's optimization pipeline.
  It then emits hella function calls and does nothing to tell the rest of the optimizer about what
  they are. Those who grok LLVM can probably see the problem. I'm pizlonating loads and stores
  before any mem2reg, sroa, inlining, gvn, or indeed any of the optimizations that eliminate the
  "language technicality" loads and stores that structured assembly programmers like Yours Truly
  don't even think of as loads and stores. It's easy to forget that assigning to a local variable
  that you never point any pointers at is a store from the language's standpoint; it's just that
  the compiler can super reliably work out that you didn't really mean for the dang thing to be in
  memory. But I'm pizlonating the program before LLVM gets to do its magic. Once the program is
  pizlonated, there is no hope for LLVM to do anything about those loads and stores, since by now
  they will have been surrounded by hella function calls. What fun it will be to fix this silly
  mistake!

The plan to make Fil-C fast is to do the following things.

1. Stack-allocate the argument buffer instead of heap-allocating it. This likely accounts for a
   good chunk of the current slowness.

2. Grindy optimizations to `llvm::FilPizlonatorPass` and the runtime. There are many cases where
   the pass emits multiple calls to the runtime when it could have emitted one or none. I did it
   that way for ease and speed of bring-up. Fixing this just means typing more compiler code and
   being more mindful of how LLVM API is used. Profiling of the runtime's functions is likely to
   reveal that some of them could just be written better. It'll be easy to do this kind of work
   once there's a good corpus! It's going to involve typing more C code, but ought not be
   conceptually difficult.

3. Create a `llvm::FilCTargetMachine` with opaque 16-byte/16-byte-aligned pointers so that we can
   run LLVM optimizations before `llvm::FilPizlonatorPass`. Even if it's not possible to run the
   entire pipeline before pizlonation, even just running mem2reg would be a huge perf boost, since
   this would remove most of the local variable allocations. This is where the biggest win is
   likely to happen! Most
   likely all of the really good optimizations that eliminate the majority of loads and stores
   will work fine on such a target machine. In this world, `llvm::FilPizlonatorPass` would take
   in a `llvm::Module` that is in Fil-C LLVM IR, and emits a new `llvm::Module` that is in the
   actual target machine's LLVM IR (so now pointers are thin as usual, but the code has already
   been instrumented). Ideally, I'd do this so that LTO sees the Fil-C LLVM IR before pizlonation,
   so my pass can run over a maximal view of the program.

4. Teach the rest of LLVM about those Fil-C runtime functions that can be optimized after the
   `llvm::FilPizlonatorPass` runs.

5. Abstract interpretation! Fil-C makes it super practical to deploy points-to analysis, shape
   analysis, and integer range analysis at scale for C code optimization, since Fil-C is already
   sound even without that analysis. Normally, making a sound static analysis for C means making
   a static analysis that proves that the program isn't just scribbling memory at random. This
   means that to even get to the point where the analysis gives useful optimization hints, you
   first have to make the analysis powerful enough to prove that your C program is memory safe!
   And that's super hard! But there is no such requirement when analyzing Fil-C. Quite the
   opposite: in Fil-C, every pointer access "proves" something about the pointed-at object
   (including establishing some bounds on its bounds, type, points-to set, etc) because Fil-C
   will definitely execute a check. So, the job of the static analysis is to be just good enough
   that it can prove that some of those checks aren't necessary. Even better, the analysis will
   be able to tell exactly which part of a check needs to execute. Maybe it proves that we know
   that we're above lower bound but not below upper bound; in that case we just need to emit the
   upper bounds check. Additionally, a sufficiently powerful abstract interpreter could
   automatically thin some pointers. If it proves that a pointer only gets values that are of some
   trivial type and bounds, or if it proves that a pointer is only ever used in a way that requires
   trivial type and bounds, then we could possibly shrink that pointer's representation to just 64
   bits.

I believe that all of these things put together might bring Fil-C to 1.5x of legacy C perf.

## MonoCap

Fil-C uses a pointer representation that is a 16-byte atomic tuple of `filc_object*` and `void*`.
The raw pointer component can point anywhere as a result of pointer arithmetic. The object component
points to the base of a GC-allocated *monotonic capability* object (hence the MonoCap name).

It so happens that for most allocations, the payload (where the raw pointer can perform accesses
without trapping) is the same allocation as the capability object. The payload is right after the
capability object within that allocation. MonoCaps also support `mmap`. In that case, the capability
object is a separate allocation from the payload, since the payload comes directly from `mmap`.
MonoCaps also support `munmap`, by first invalidating the capability and then unmapping the payload.

The object format contains:

- 64-bit lower bounds.

- 64-bit upper bounds.

- 16-bit flags. This is used for supporting unusual situations, like globally allocated objects,
  special runtime-internal objects (like threads and signal handlers), as well as the free object
  state. Function pointers also leverage the flags.

- 8-bit word type per 16-byte word in the object payload. This forms an array that is 1/16th the size
  of the allocation payload and allows inferring the types of the words in the allocation on the fly.
  Each word type forms a lattice that starts
  with *unset* at the bottom, *int* and *ptr* in the middle, and *free* at the top. Accessing an
  *unset* word using an int access makes it *int*. Accessing an *unset* word using a ptr access makes
  it a *ptr*. Once the word type is not *unset*, it can never become *unset* again, so future accesses
  must conform to the type you first picked. Once you `free()` the object, all word types become
  *free*, and then all accesses trap. There are additional word types for special objects that used
  by the runtime as well as function pointers.

The monotonicity of word types is what enables MonoCap to support automatic inference of type for
unions, `malloc` calls, and unusual things Real C Programmers (TM) do (like using a `char buf[100]`
as the object payload by casting the `char*` to whatever).

When Fil-C dumps pointers in error messages, the word types are printed as follows:

- `_` means *unset*.

- `i` means *int*.

- `P` means *ptr*.

- `/` means *free*.

If you include [`<stdfil.h>`](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/filc/include/stdfil.h),
you can `zprintf()` with the `%P` format specifier to print the full Fil-C view of a pointer.

## Fil's Unbelievable Garbage Collector

[FUGC](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/fugc.c)
is a semi-novel algorithm. For those well-versed in concurrent GC design, it'll sound almost like
old hat - but the sort of old hat you enjoy wearing.

In GC mafia jargon, FUGC is a *concurrent on-the-fly grey-stack Dijkstra accurate non-moving* collector.
Let's break that down:

- Concurrent: marking and sweeping happen on some thread, which can run on whatever core or be
  scheduled by the OS however the OS wants. The *mutator* (i.e. your program and all of its threads)
  can run in other threads, concurrently to the collector. (This is different from saying
  that the collector is *parallel* - a parallel collector is one that runs marking and sweeping in
  multiple threads; a parallel-but-not-concurrent collector will pause your program to do this. FUGC
  is concurrent, meaning that it doesn't pause your program to do this.) The interaction between the
  collector thread and mutator threads is mostly non-blocking (locking is only used on allocation slow
  paths).

- On-the-fly: there is no global stop-the-world, but instead we use what some mafia members will call
  "soft handshakes" while other made men will call "ragged safepoints". In the language of civilians,
  this means that the GC may ask threads to do some work (like scan stack), but threads do this
  asynchronously, on their own time, without waiting for the collector or other threads. The only "pause"
  threads experience is the callback executed in response to the soft handshake, which does work bounded
  by that thread's stack height. That "pause" is usually shorter than the slowest path you might take
  through a typical `malloc` implementation.

- Grey-stack: the collector assumes it must rescan thread stacks to fixpoint. That is, GC starts with
  a soft handshake to scan stack, and then marks in a loop. If this
  loop runs out of work, then FUGC does another soft handshake. If that reveals more objects, then
  concurrent marking resumes. This prevents us from having a *load barrier* (no instrumentation runs
  when loading a pointer from the heap into a local variable). Only a *store barrier* is
  necessary, and that barrier is very simple. This fixpoint converges super quickly because all newly
  allocated objects during GC are pre-marked.

- Dijkstra: storing a pointer field in an object that's in the heap or in a global variable while FUGC
  is in its marking phase causes the newly pointed-to object to get marked. This is called a *Dijkstra
  barrier* and it is a kind of *store barrier*. Due to the grey stack, there is no load barrier like
  in the [classic Dijkstra collector](https://lamport.azurewebsites.net/pubs/garbage.pdf). The FUGC store
  barrier uses a compare-and-swap with relaxed memory ordering on the slowest path (if the GC is running
  and the object being stored was not already marked).

- Accurate: the GC accurately (aka precisely, aka exactly) finds all pointers to objects, nothing more,
  nothing less. `llvm::FilPizlonator` ensures that the runtime always knows where the root pointers are
  on the stack and in globals. The Fil-C runtime has a clever API and Ruby code generator for tracking
  pointers in low-level code that interacts with pizlonated code. All objects know where their outgoing
  pointers are thanks to the word type array in MonoCaps.

- Non-moving: the GC doesn't move objects. This makes concurrency easy to implement and avoids
  a lot of synchronization between mutator and collector. However, FUGC will "move" pointers to free
  objects (it will repoint the `filc_object*` component of the MonoCap to the free singleton so it
  doesn't have to mark the freed allocation).

This makes FUGC an *advancing wavefront* garbage collector. Advancing wavefront means that the
mutator cannot create new work for the collector by modifying the heap. Once an
object is marked, it'll stay marked for that GC cycle. It's also an *incremental update* collector, since
some objects that would have been live at the start of GC might get freed if they become free during the
collection cycle.

FUGC relies on *safepoints*, which comprise:

- *Pollchecks* emitted by the compiler. The `llvm::FilPizlonator` emits pollchecks often enough that only a
  bounded amount of progress is possible before a pollcheck happens. The fast path of a pollcheck is
  just a load-and-branch. The slow path runs a *pollcheck callback*, which does work for FUGC.

- Soft handshakes, which request that a pollcheck callback is run on all threads and then waits for
  this to happen.

- *Enter*/*exit* functionality. This is for allowing threads to block in syscalls or long-running
  runtime functions without executing pollchecks. Threads that are in the *exited* state will have
  pollcheck callbacks executed by the collector itself (when it does the soft handshake). The only
  way for a Fil-C program to block is either by looping while entered (which means executing a
  pollcheck at least once per loop iteration, often more) or by calling into the runtime and then
  exiting.

Safepointing is essential for supporting threading (Fil-C supports pthreads just fine) while avoiding
a large class of race conditions. For example, safepointing means that it's safe to load a pointer from
the heap and then use it; the GC cannot possibly delete that memory until the next pollcheck or exit.
So, the compiler and runtime just have to ensure that the pointer becomes tracked for stack scanning at
some point between when it's loaded and when the next pollcheck/exit happens, and only if the pointer is
still live at that point.

The safepointing functionality also supports *stop-the-world*, which is currently used to implement
`fork(2)` and for debugging FUGC (if you set the `FUGC_STW` environment variable to `1` then the
collector will stop the world and this is useful for triaging GC bugs; if the bug reproduces in STW
then it means it's not due to issues with the store barrier). The safepoint infrastructure also allows
safe signal delivery; Fil-C makes it possible to use signal handling in a practical way. Safepointing is
a common feature of virtual machines that support multiple threads and accurate garbage collection,
though usually, they are only used to stop the world rather than to request asynchronous activity from all
threads. See [here](https://foojay.io/today/the-inner-workings-of-safepoints/) for a write-up about
how OpenJDK does it. The Fil-C implementation is in [`filc_runtime.c`](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/filc_runtime.c).

Here's the basic flow of the FUGC collector loop:

1. Wait for the GC trigger.
2. Turn on the store barrier, then soft handshake with a no-op callback.
3. Turn on black allocation (new objects get allocated marked), then soft handshake with a callback
   that resets thread-local caches.
4. Mark global roots.
5. Soft handshake with a callback that requests stack scan and another reset of thread-local caches.
   If all collector mark stacks are empty after this, go to step 7.
6. Tracing: for each object in the mark stack, mark its outgoing references (which may grow the mark
   stack). Do this until the mark stack is empty. Then go to step 5.
7. Turn off the store barrier and prepare for sweeping, then soft handshake to reset thread-local
   caches again.
8. Perform the sweep. During the sweep, objects are allocated black if they happen to be allocated out
   of not-yet-swept pages, or white if they are allocated out of alraedy-swept pages.
9. Victory! Go back to step 1.

If you're familiar with the literature, FUGC is sort of like the DLG (Doligez-Leroy-Gonthier) collector
(published in [two](https://xavierleroy.org/publi/concurrent-gc.pdf)
[papers](http://moscova.inria.fr/~doligez/publications/doligez-gonthier-popl-1994.pdf) because they
had a serious bug in the first one), except it uses the Dijkstra barrier and a grey stack, which
simplifies everything but isn't as academically pure (FUGC fixpoints, theirs doesn't). I first came
up with the grey-stack Dijkstra approach when working on
[Fiji VM](http://www.filpizlo.com/papers/pizlo-eurosys2010-fijivm.pdf)'s CMR and
[Schism](http://www.filpizlo.com/papers/pizlo-pldi2010-schism.pdf) garbage collectors. The main
advantage of FUGC over DLG is that it has a simpler (cheaper) store barrier and it's a slightly more
intuitive algorithm. While the fixpoint seems like a disadvantage, in practice it converges after a few
iterations.

Additionally, FUGC relies on a sweeping algorithm based on bitvector SIMD. This makes sweeping insanely
fast compared to marking. This is made thanks to the
[Verse heap config](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/verse_heap.h)
that I added to
[libpas](https://github.com/WebKit/WebKit/blob/main/Source/bmalloc/libpas/Documentation.md). FUGC
typically spends <5% of its time sweeping.

FUGC can easily be made parallel. Sweeping is trivial to parallelize; I just haven't done it because
I want to test it more and fix all the bugs I find before I do that. Marking is easy to paralellize
using any of the usual parallel marking algorithms (though I am especially fond of the one I used in
[Riptide](https://webkit.org/blog/7122/introducing-riptide-webkits-retreating-wavefront-concurrent-garbage-collector/),
so I'll probably just do that when ready).

## Conclusion

I would like to thank my awesome employer, Epic Games, for allowing me to work on this in my spare
time. Hence, Fil-C is (C) Epic Games, but all of its components are permissively-licensed open
source. In short, Fil-C's compiler bits are Apache2 while the runtime bits are BSD.

