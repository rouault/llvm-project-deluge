# The Fil-C Manifesto: Garbage In, Memory Safety Out!

The C programming language is wonderful. There is a ton of amazing code written in C. But C is
an unsafe language. Simple logic errors may result in an attacker controlling where a pointer points
and what is written into it, which leads to an easy path to exploitation. Lots of other languages
(Rust, Java, Haskell, Verse, even JavaScript) don't have this problem!

But I love C. I grew up on it. It's such a joy for me to use! Therefore, in my spare time, I decided
to make my own memory-safe C. This is a personal project and an expression of my love for C.

Fil-C introduces memory safety at the core of C:
 
- All pointers carry a *capability*, which tracks the bounds and type of the pointed-to memory. Fil-C
  pointers are 32 bytes, require 16 byte alignment, and use a special encoding for bounds and type
  that makes pointers safely and usefully atomic (using a novel algorithm called SideCap).

- All allocations are *isoheaped*: to allocate, you must describe the type being allocated, and the
  allocator will return a pointer to memory that had always been exactly that type. Use-after-free
  does not lead to type confusion in Fil-C. Fil-C uses a modified version of
  [libpas](https://github.com/WebKit/WebKit/blob/main/Source/bmalloc/libpas/Documentation.md) based
  on the Unreal Engine version of libpas.

- The combination of SideCap and isoheaps means that pointer capabilities cannot be forged. Your
  program may have logic errors (bad casts, bogus pointer arithmetic, races, bad frees, whatever)
  but every pointer will remember the bounds and type of the thing it originated from. If you break
  that pointer's rules by trying to access out-of-bounds, or read an int as a pointer or vice-versa,
  Fil-C will thwart your program's further execution. And, once a piece of memory is chosen to be a
  pointer by the allocator, it will always be one; ditto for integers.

- Fil-C's protections are designed to be comprehensive. There's no escape hatch short of [delightful
  hacks that also break all memory-safe languages](https://blog.yossarian.net/2021/03/16/totally_safe_transmute-line-by-line).
  There's no `unsafe` keyword. Even data races result in memory-safe outcomes. Fil-C operates on the
  GIMSO principle: Garbage In, Memory Safety Out! No program accepted by the Fil-C compiler can
  possibly go on to escape out of the Fil-C type system. At worst, the program's execution will be
  thwarted at runtime by Fil-C.

Fil-C is already powerful enough to run a memory-safe curl on top of a memory-safe OpenSSL,
memory-safe zlib, and memory-safe musl (Fil-C's current libc). This works for me on my Apple Silicon
Mac:

    pizfix/bin/curl https://www.google.com/

Where the `pizfix` is the Fil-C staging environment for *pizlonated* programs (programs that now
successfully compile with Fil-C). The only unsafety in Fil-C is in libpizlo (the runtime library),
which exposes all of the API that musl needs (low-level
[syscall and thread primitives](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/libpas/src/libpas/filc_runtime.c#L3538),
which themselves perform comprehensive safety checking).

On the other hand, Fil-C is quite slow. It's 200x slower than legacy C right now. I have not done any
optimizations to it at all. I am focusing entirely on correctness and ergonomics and converting as
much code to it as I personally can in my spare time. It's important for Fil-C to be fast eventually,
but it'll be easiest to make it fast once there is a large corpus of code that can run on it.

This document goes into the details of Fil-C and is organized as follows. First, I show you how to
use Fil-C. Then, I describe the Fil-C development plan, which explains my views on growing the set
of things Fil-C can run and how to make it run fast. The section about making it run fast also delves
into a lot of technical details about how Fil-C works. Then I conclude with a description of the
SideCap 32-byte atomic pointer algorithm.

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

- The `llvm::FilPizlonatorPass` uses assert() as its error checking for now, so you must compile llvm
  with assertions enabled (the build_all.sh script does this).

Fil-C requires that some C code does change. In particular, Fil-C must know about the types of any
allocations that contain pointers in them. It's fine to allocate primitive memory (like int arrays,
strings, etc) using malloc. But for anything with pointers in it, you must use the `zalloc` API
provided by [`<stdfil.h>`](https://github.com/pizlonator/llvm-project-deluge/blob/deluge/filc/include/stdfil.h).
For example:

    char** str_ptr = zalloc(char*, 1);

Allocates enough memory to hold one `char*` and returns a pointer to it.

Fil-C allocation functions trap if allocation fails and returns zero-initialized memory on success.

You can free memory allocated by zalloc using the normal `free()` function. Freeing doesn't require
knowing the type. Also, malloc is internally just a wrapper for `zalloc(char, count)`.

Fil-C provides a rich API for memory allocation in `<stdfil.h>`. Some examples:

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
but I have not done this for now; I just replaced the existing allocator calls with `<stdfil.h>`
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

Fil-C can already run most of musl, zlib, OpenSSL, and curl. It's starting to run OpenSSH. My goal
is to grow the corpus until I have a small UNIX-like userland that comprises only pizlonated
programs.

Corpus growth should proceed as follows:

- First get to at least 10 large, real-world C libaries or programs compiling with Fil-C. I don't
  consider zlib to be large, so it doesn't count. I don't consider musl to be part of the corpus,
  since I'm making lots of internal changes to it (and I'm willing to even completely rewrite it
  if it makes adding more programs easier). I don't have confidence that OpenSSL and curl fully
  work yet. So, right now, I'm somewhere between 0/10 and 2/10 on this goal, depending
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

The biggest impediment to using Fil-C in production is speed. Fil-C is currently about 200x slower
than legacy C according to my tests (good old [Richards](https://www.cl.cam.ac.uk/~mr10/Bench.html),
zlib's minigzip, and OpenSSL's `enc` command all seem to agree).

Why is it so slow? Answering this question also gives us a fun way to talk about Fil-C's technical
details. So, this section will simultaneously explain how Fil-C works today and how it would work
if I cared about performance. And I won't care about performance until I've got a corpus!

First of all, performance is a deliberate non-goal of the current implementation. It's super hard
to make a C compiler widen all pointers to 32 bytes, align them to 16 bytes, and then transform
all of the code in a way that allows zero unsafety through. It's hard to even reason about whether
the chosen transformation strategy obeys the compiler's own laws let alone whether it's sound. To
make it easy for me to feel confident that I was doing the right thing when writing the runtime
and `llvm::FilPizlonatorPass`, I consistently went for implementation tactics that are dead simple
to get right, reason about, and test. For example:

- The runtime and compiler carry around SideCap pointers, which are awkward to encode and decode.
  So whenever your program requires looking a pointer's value, lower bound, upper bound, or type,
  or whenever your program requires creating a pointer with a new value or bounds/type, then it's
  either calling functions that are big and gross, or it's executing inlined code that is totally
  absurd. The compiler emits tons of calls, everywhere, even in cases of things you could emit
  decent llvm IR for, just because it's simpler. An obvious optimization is to just use SideCaps
  for pointers-at-rest in the heap and use a tuple of ptr,lower,upper,type for pointers-in-flight.
  But using SideCaps everywhere was simpler, and that probably accounts for something like 10x
  slowdown alone! And what an easy problem to fix, if I cared!

- The Fil-C ABI currently has the caller isoheap-allocate a buffer in the heap to store the
  arguments. The callee deallocates the argument buffer. This makes dealing with `va_list` (and
  all of the ways it could be misused) super easy. But, it wouldn't be hard to change the ABI to
  have the caller stack-allocate the buffer and then have the caller isoheap-allocate a clone if
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

The plan to make Fil-C fast is to do the following seven things. The last of those seven - language
changes - should happen after the first six have gained traction.

1. Switching to two pointer representations - pointers-in-flight and pointers-at-rest.
   Pointers-at-rest have to use SideCap, because they might be raced on. Pointers-in-flight can
   use a much more efficient tuple of ptr,lower,upper,type (which is semantically what SideCap
   gives you, just via complex logic). This will require some nontrivial rewriting of
   `llvm::FilPizlonatorPass`. It will require some refactoring in the runtime. This work will
   lead to an entirely new performance baseline, since this is the majority of the overhead
   right now. Once this is done, the rest of the optimizations in this list can proceed.

2. Stack-allocate the argument buffer instead of heap-allocating it. This likely accounts for a
   good chunk of the current slowness.

2. Grindy optimizations to `llvm::FilPizlonatorPass` and the runtime. There are many cases where
   the pass emits multiple calls to the runtime when it could have emitted one or none. I did it
   that way for ease and speed of bring-up. Fixing this just means typing more compiler code and
   being more mindful of how LLVM API is used. Profiling of the runtime's functions is likely to
   reveal that some of them could just be written better. It'll be easy to do this kind of work
   once there's a good corpus! It's going to involve typing more C code, but ought not be
   conceptually difficult.

4. Create a `llvm::FilCTargetMachine` with opaque 32-byte/16-byte-aligned pointers so that we can
   run LLVM optimizations before `llvm::FilPizlonatorPass`. Even if it's not possible to run the
   entire pipeline before pizlonation, even just running mem2reg would be a huge perf boost. Most
   likely all of the really good optimizations that eliminate the majority of loads and stores
   will work fine on such a target machine. In this world, `llvm::FilPizlonatorPass` would take
   in a `llvm::Module` that is in Fil-C LLVM IR, and emits a new `llvm::Module` that is in the
   actual target machine's LLVM IR (so now pointers are thin as usual, but the code has already
   been instrumented). Ideally, I'd do this so that LTO sees the Fil-C LLVM IR before pizlonation,
   so my pass can run over a maximal view of the program.

5. Teach the rest of LLVM about those Fil-C runtime functions that can be optimized after the
   `llvm::FilPizlonatorPass` runs. For example, LLVM could know about the semantics of Fil-C
   "isostack" allocation, since it's similar enough to `alloca` combined with lifetime intrinsics.

6. Abstract interpretation! Fil-C makes it super practical to deploy points-to analysis, shape
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

7. Language changes. Once Fil-C reaches a new performance baseline thanks to the above
   optimizations, we can introduce new pointer annotations to Fil-C to take performance even
   further. These annotations would be totally memory-safe. Fil-C will *always* allow unannotated
   pointers, and they will be wide SideCap pointers unless the compiler proves that they don't have
   to be. Annotations will make pointers will make the pointer thinner and less capable. For
   example, I might add a `zthin` annotation that requests that the pointer is 64-bit. Those
   pointers would have to always point to the declared element type in C (so `Foo*zthin` can only
   point at structural subtypes of `Foo`) and they will only point to a single object (pointer
   arithmetic on them is sure to result in an out-of-bounds pointer that cannot be accessed). I
   may add other annotations, like `zarray`, that requests a 128-bit pointer that just has a
   pointer and a size. I'll add enough different pointer kinds to cover most of the use cases of
   pointers, while keeping full SideCaps as the default. This will create the following situation
   for Fil-C users: you can easily convert your code to Fil-C and you don't have to annotate your
   pointers to get there. But if you want speed, then just profile your code and throw in some
   pointer annotations in the hot spots.

I believe that all seven of these things put together might bring Fil-C to 1.5x of legacy C perf.

## SideCap

To me, the most exciting thing about Fil-C is that even races on pointers cannot break memory
safety. This works even though pointers are 32 bytes. Storing and loading SideCaps just requires
128-bit atomic stores and loads, and the ordering can be relaxed. Compare-and-swapping SideCaps
just requires one 128-bit compare-and-swap and one 128-bit atomic store. Pretty cool, right?

Let's go into how this works by first considering a few options that don't work, but that give
us the intuitions we need to get to SideCap.

### Intuition One: Compress To 128 Bits

If I was willing to restrict the address space of Fil-C to 32 bits, then I could compress the
entire ptr,lower,upper,type combination to 32 bits.

I don't want to do that! This would severely restrict the utility of Fil-C. But, interestingly,
this is always an option for folks who want a faster Fil-C with a smaller address space.

SideCaps support 48-bit address spaces. For boxed integers (the result of casing an int to a
pointer), SideCaps support 64-bit integer values.

### Intuition Two: Forget The Lower Bound

Say we didn't care about the lower bound. In that case, we could compress the pointer to
48 bits, compress the upper bound to 48 bits, and compress the type to 32 bits. This fits in
128 bits!

But it's not good enough. Losing the lower bound would mean that if you subtract from any
pointer - even one stored to a local variable - then you'll immediately go out of bounds.

### Intuition Three: Forget The Lower Bound On Races

Is it ever OK to forget the lower bound? Sure! Lots of C pointers point to the base of
something. Pointers stored in heap data structures, shared across function call boundaries,
and most pointers that you can create (either with `&`, pointer decay, or allocation) point
at the lower bound already. Those pointers work fine with the no-lower-bound representation.

I believe that it's very unusual to race on a pointer that is subtracted from after load,
since those pointers usually arise in local algorithms where the pointer is an iterator,
rather than being stored into what the structured assembly programmer thinks of as their
heap, let alone raced on.

This is the intuition behind SideCap: preserve lower and upper bounds if there is no race,
but lose the lower bound if there is a race. So, SideCap is all about encoding the bounds,
type, and pointer value in two atomic 128-bit words in such a way that we can always tell
if they are mismatched and we always know which of them to rely on in that case.

SideCap pointers comprise:

- The *sidecar*, which may be ignored if it doesn't match the capability.
- The *capability*, which is authoritative.

SideCap pointers can take the following kinds. Two bits are used to represent kind in both
the capability and the sidecar. This leaves 30 bits for type and 2x48 bits for two pointer
values.

- Boxed integers. The type is zero in this case, and the lower 64 bits of the capability
  stores an integer. The pointer is interpreted as having NULL bounds and invalid type.
  These pointers cannot be accessed, but can be cast back to integer.

- `at_lower` pointers, where `ptr == lower` already. These are encoded entirely in the
  capability and the sidecar is always ignored.

- `in_array` pointers, where `ptr > lower` and `ptr <= upper`. The capability stores the
  pointer itself, the upper bounds, and the type. The sidecar stores the pointer itself,
  the lower bounds, and the type. For any such pointer, if the pointer and type of the
  sidecar and capability match, then we know that the lower bounds in the sidecar can be
  matched with the upper bounds of the capability. This is ensured thanks to all
  pointers originating from isoheaped allocations. Two pointers into the same allocation
  claiming to be in-bounds and to have the same type must have bounds that are mixable.
  If the sidecar doesn't match, the lower bound is inferred from ptr, upper, and type.
  Because the type has a size, and the span must be a multiple of type size, Fil-C
  usually infers a lower bound that is the base of the object.

- `flex_base` pointers, where `ptr > lower` and the pointer is inside the base of a flex
  object (an object with a trailing array). In this case, the capability stores the
  upper bound of the flex base. The lower bound can be inferred from the upper bound and
  type. The sidecar stores the true upper bound. Mismatched sidecar means you lose the
  flex array's bound (so the flex array looks to be a zero-length array).

- `oob` pointers, where `ptr < lower` or `ptr > upper`. In this case, the capability
  stores just the pointer, and the sidecar stores lower,upper,type. This means that racing
  on OOB pointers might result in an OOB pointer with some other OOB pointer's bounds and
  type.

Forging a SideCap pointer requires picking the right kind based on where it is in its
bounds. Decoding a SideCap pointer means checking its kind, checking if the sidecar is
relevant, and then doing a bunch of bit fiddling.

Types are represented as 30-bit indices into a type table controlled by libpizlo.

## Conclusion

I would like to thank my awesome employer, Epic Games, for allowing me to work on this in my spare
time. Hence, Fil-C is (C) Epic Games, but all of its components are permissively-licensed open
source. In short, Fil-C's compiler bits are Apache2 while the runtime bits are BSD.

