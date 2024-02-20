# The Fil-C Manifesto

The C programming language is wonderful. There is a ton of amazing code written in C. But C is
an unsafe language. Simple logic errors may result in an attacker controlling where a pointer points
and what is written into it, which leads to an easy path to exploitation. Lots of other languages
(Rust, Java, Haskell, Verse, even JavaScript) don't have this problem!

Fil-C solves this problem by introducing memory safety at the core of C:
 
- All pointers carry a *capability*, which tracks the bounds and type of the pointed-to memory.

- All allocations are *isoheaped*: to allocate, you must describe the type being allocated, and the
  allocator will return a pointer to memory that had always been exactly that type. Use-after-free
  does not lead to type confusion in Fil-C.

- Fil-C's protections are designed to be comprehensive. There's no escape hatch. There's no `unsafe`
  keyword. Even data races result in memory-safe outcomes (there is no way to escalate a pointer's
  capability by racing on it). Fil-C operates on the GIMSO principle: Garbage In, Memory Safety Out!
  No program accepted by the Fil-C compiler can possibly go on to escape out of the Fil-C type
  system.

Fil-C is already powerful enough to run a memory-safe curl on top of a memory-safe OpenSSL,
memory-safe zlib, and memory-safe musl (Fil-C's current libc). The only unsafety in Fil-C is in
libpizlo (the runtime library), which exposes all of the API that musl needs (syscall and thread
primitives).

On the other hand, Fil-C is quite slow. It's 200x slower than legacy C right now. I have not done any
optimizations to it at all. I am focusing entirely on correctness and ergonomics and converting as
much code to it as I personally can in my spare time. It's important for Fil-C to be fast eventually,
but it'll be easiest to make it fast once there is a large corpus of code that can run on it.

This document goes into the details of Fil-C and is organized as follows. First, I show you how to
use Fil-C. Then, I describe the Fil-C development plan, which explains my views on adoption and
performance. Finally, I go into some technical details.

## Using Fil-C

First I'll tell you how to build Fil-C and then I'll tell you how to use it.

### Building Fil-C

Fil-C currently only works on Apple Silicon Macs. Upon getting Fil-C from
https://github.com/pizlonator/llvm-project-deluge.git, and making sure you're on the `deluge` branch,
simply do:

    ./setup_gits.sh
    ./build_all.sh

Note you should get a build error in ssh at the end, like this:

    ./arch/aarch64/syscall_arch.h:21:2: __syscall1: filc_error: Cannot handle inline asm:   %8 = call i64 asm sideeffect "svc 0", "={x0},{x8},{x0},~{memory},~{cc}"(i64 %filc_load97, i64 %filc_load103) #4, !dbg !26, !srcloc !28
    [13104] filc panic: thwarted a futile attempt to violate memory safety.
    /bin/bash: line 1: 13104 Trace/BPT trap: 5       ./ssh-keygen -A

This is because I've only started porting OpenSSH. If you get to this error, then it means you have
a working Fil-C. Now you can try to download something with the pizlonated curl, like maybe:

    pizfix/bin/curl https://www.google.com/

If this works, then congratulations! You just downloaded something over https using a memory-safe
userland stack!

### Using Fil-C

Let's start with the basics. Fil-C works like any C compiler. Take this program:

    #include <stdio.h>
    int main() { printf("Hello!\n"); return 0; }

Say it's named hello.c. We can do:

    xcrun build/bin/clang -o hello hello.c -g -O

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

    [pizlo@behemoth llvm-project-deluge] xcrun build/bin/clang -o bad bad.c -g -O
    [pizlo@behemoth llvm-project-deluge] ./bad                                   
    bad.c:4:37: main: cannot access pointer with ptr >= upper (ptr = 0x10d01416c,0x10d014144,0x10d014148,type{int}).
    [13157] filc panic: thwarted a futile attempt to violate memory safety.
    zsh: trace trap  ./bad

Fil-C thwarted this program's attempt to do something bad. Hooray!

The Fil-C version of clang works much like normal clang. It accepts C code and produces .o files
that your linker will understand. Some caveats:

- Fil-C currently relies on the staging environment (where all the pizlonated libraries, like OpenSSL
  and friends, live) to be in the llvm-project source directory.

- Fil-C currently relies on you *not* installing the compiler. You have to use it directly from the
  build directory created by the build_all.sh script.

- The FilPizlonatorPass uses assert() as its error checking for now, so you must compile llvm with
  assertions enabled (the build_all.sh script does this).

Fil-C requires that some C code does change. In particular, Fil-C must know about the types of any
allocations that contain pointers in them. It's fine to allocate primitive memory (like int arrays,
strings, etc) using malloc. But for anything with pointers in it, you must use the `zalloc` API
provided by `<stdfil.h>`. For example:

    char** str_ptr = zalloc(char*, 1);

Allocates enough memory to hold one `char*` and returns a pointer to it.

Fil-C allocation functions trap if allocation fails and returns zero-initialized memory on success.

You can free memory allocated by zalloc using the normal free() function. Freeing doesn't require
knowing the type. Also, malloc is internally just a wrapper for `zalloc(char, count)`.

Fil-C provides a rich API for memory allocation in <stdfil.h>. Some examples:

- You can allocate aligned by saying `zaligned_alloc(type, alignment, count)`.

- You can allocate *flexes* - objects with flexible array members, aka trailing arrays - using
  `zalloc_flex(type, array_field, count)`.

- You can reallocate with `zrealloc(old_ptr, type, count)`. You'll get a trap if you try to realloc
  to the wrong type. If you use `zrealloc` to grow an allocation, then the added memory is
  zero-initialized.

- You can allocate in a heap suitable for crypto using `zhard_alloc(type, count)`. This heap is
  mlocked and has other mitigations beyond what zalloc offers. It's also isoheaped, so the hard
  heap is really a bunch of heaps (one heap per type).

- You can reflectively build types at runtime using `zslicetype` and `zcattype` and then allocate
  memory of that type using `zalloc_with_type`.

- You can allocate memory that has exactly the same type as some pointer with `zalloc_like`.

Almost all of the changes you will make to your C code to use Fil-C involve changing calls like:

    malloc(sizeof(some_type))

to:

    zalloc(some_type, 1)

and stuff like:

    malloc(count * sizeof(some_type))

to:

    zalloc(some_type, count)

In some cases, the Fil-C changes make things a lot clearer. Consider this flex allocation in C:

    malloc(OFFSETOF(some_type, array_field) + sizeof(array_type) * count)

This just becomes:

    zalloc_flex(some_type, array_field, count)

Note that `zalloc` and friends do overflow checking and will trap if that fails, so it's not
possible to mess up by using a too-big count. Even if zalloc did no such checking, the capability
returned by zalloc is guaranteed to match the amount of memory actually allocated - so if `count`
is zero, then the resulting pointer will be inaccessible.

Most of the changes I've had to make to zlib, OpenSSL, curl, and OpenSSH are about replacing calls
to malloc/calloc/realloc to use zalloc/zalloc_flex/zrealloc instead. I believe that those changes
could be abstracted behind C preprocessor macros to make the code still also compile with legacy C,
but I have not done this for now; I just replaced the existing allocator calls with <stdfil.h>
calls.

## The Fil-C Development Plan

Lots of projects have attempted to make C memory-safe. I've worked on some of them. Usually, the
development plan starts with the truism that C has to be fast. This leads to antipatterns creeping
in from the start.

Premature optimization really is the root of all evil! In the case of new language bringup, or the
bringup of a radically new way of compiling an existing language, premature optimizations are
particularly evil because:

- If your language doesn't run anything yet, then you cannot meaningfully test any of your
  optimizations. Therefore, fast memory-safe C prototypes are likely to only be fast on whatever
  tiny corpus of tests that prototype was built to run. Worse, it's likely that they can ony run
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

Fil-C can already run most of musl, zlib, OpenSSL, and curl. It's starting to run OpenSSH. My goal
is to grow the corpus until I have a small UNIX-like userland that comprises only pizlonated
programs.

Corpus growth should proceed as follows:

- First get to at least 10 large, real-world C libaries or programs compiling with Fil-C. I don't
  consider zlib to be large, so it doesn't count. I don't consider musl to be part of the corpus,
  since I'm making lots of internal changes to it (and I'm willing to even completely rewrite it
  if it makes adding more programs easier). I don't have confidence that OpenSSL and curl fully
  work yet. So, right now, I'm somewhere between 0/10 and 2/10 depending on this goal, depending
  on whether you believe that I got OpenSSL and curl to really work or not.

- Then add C++ support and add at least 10 large, real-world C++ libraries or programs.

Once we have such a corpus, then it'll make sense to start thinking about some optimizations. The
hardest part of this will be expanding Fil-C to support C++, since C++ has its own allocation
story - namely, that you can overload `operator new` and `operator new` does not take a type. This
will have to change in Fil-C++ and that will require some more compiler surgery.

Even after the corpus grows to 10 C programs and 10 C++ programs, we will still want to keep
growing the corpus. But hopefully, it'll get easier to grow the corpus as the corpus grows. For
example, it's still the case that adding new code trips cases where some musl syscall isn't
implemented in libpizlo. Eventually musl+libpizlo will know all the syscalls.

### Making Fil-C Fast

