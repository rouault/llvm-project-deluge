# Deluge

This is an LLVM fork that I am doing for fun. It's called Deluge.

Deluge is a memory-safe compiler and runtime for C. What I mean by that is:

- Deluge will kill your program if it detects a futile attempt to violate memory safety.

- Deluge operates on the GIMSO principle: "Garbage In, Memory Safety Out!". In other words, given any
  sequence of characters in your .h and .c files, even those written adversarially and in anger, the
  Deluge compiler will either reject the program (possibly by crashing - most of my static checks are
  implemented as assert() rn, LMAO - so be sure to compile with asserts on), or it will generate a
  program that obeys the Deluge type system.

- Every pointer carries a capability under the Deluge type system. Each pointer knows its bounds and
  each pointer knows the type of the memory it points at. Deluge will kill your program if you attempt
  out-of-bounds accesses or type-confused accesses (accessing an int as a ptr or vice-versa). Deluge
  will kill your program if you try to access a function pointer or call a data pointer. Deluge loves
  killing your program!

- The Deluge calling convention carries bounds and type for the arguments and return value, so calling
  a function with the wrong signature will result in Deluge killing your program.

- Deluge instruments va_arg and friends, so misusing it (even stashing/escaping it and then doing
  stuff after the function returns) will not break the Deluge type system, but will most likely kill
  your program.

- Deluge is careful about what it tells the linker about your program, so that it can protect type or
  bounds confusion arising from mislinked globals. If you define a global of one type in one file, and
  declare it extern with a different type (or different bounds) in another file, and then link and run,
  then the moment that the second file actually tries to access its type-confused reference, Deluge
  will kill your program.

- Memory allocation in Deluge prevents type confusion by isolating each Deluge type into its own heap
  and being careful about type alignment. This makes free() into at worst a logic error. This also
  means that some code changes are necessary. You need to tell Deluge the type at time of allocation.
  Deluge's allocation API natively supports objects or arrays of objects of any size and alignment
  (including enormous alignments), flexes (objects with flexible array members, aka trailing arrays),
  cloning (allocate memory of a type that matches some other object), reflection (you can build up a
  type at runtime and ask the allocator to deal with it), and optional "hard" allocation (what OpenSSL
  calls the secure heap, so it's mlocked, DONT_DUMPed, zeroed on alloc and free, guarded with
  mprotected pages, and some other stuff).

- I'm a lover of good GCs, so I've been designing Deluge with that in mind. There isn't anything in
  Deluge that prevents me switching to a real-time GC instead of isolated heaps. I have a sick algo in
  mind. But for now, I like how stuff Just Works with isoheaps.

- Although Deluge pointers are 32 bytes, they are atomic by default, and are safe and useful to race
  on. Racing on a Deluge pointer will never result in a more capable pointer than any of the pointers
  that were racily stored. Deluge achieves this without requiring extra fences or atomic ops on pointer
  loads and stores. Deluge pointers support wait-free compare-and-swap. I call this algorithm SideCap
  and I'll have to write it up at some point because it is hella dank.

- Deluge has its own include and lib. To call a library from deluded code, you must delude that
  library. Deluge's libc is a hacked musl. Its runtime, libdeluge, is based on libpas. The deluded musl
  makes syscalls by calling into the syscall surface area that libdeluge provides. See
  libpas/src/libpas/deluge_runtime.c for most of the action. Currently everything gets put into the
  pizfix, so you can pizfix/bin/openssl or pizfix/bin/curl (kinda, there's more to do).

- There is no escape! Deluded code cannot call anything other than other Deluded code or legacy C code
  that exposes itself using Deluge ABI. The only such legacy C code that exists right now is in
  libdeluge and we should probably keep it that way. Therefore, from within Deluge, it's not possible
  to escape Deluge. It's just as GIMSO as Java. (It's even more GIMSO than Java if you put yourself in
  the Java boot classpath and go to town on sun.misc.Unsafe - Deluge has nothing like that.)

Right now it's like 200x slower than C. But I can kinda sorta run some of openssl and some of curl.
Zlib works fine.

This currently only works on Mac. To start, do:

    ./setup_gits.sh
    ./build_all.sh

Then you can try compiling your first memory-safe hello world like:

    xcrun build/bin/clang -o hello hello.c -g -O

Feel free to use <stdio.h> since musl mostly works.

If you want to play with Deluge API, include <stdfil.h> (see deluge/include/stdfil.h).

Note that the resulting binary will run without needing to specify a DYLD_LIBRARY_PATH because hacks.
