# Fil-C

This is an LLVM fork that I am doing for fun. It's called Fil-C.

[See here for more information.](Manifesto.md)

Fil-C is a memory-safe compiler and runtime for C. What I mean by that is:

- Fil-C will kill your program if it detects a futile attempt to violate memory safety.

- Fil-C operates on the GIMSO principle: "Garbage In, Memory Safety Out!". In other words, given any
  sequence of characters in your .h and .c files, even those written adversarially and in anger, the
  Fil-C compiler will either reject the program (possibly by crashing), or it will generate a
  program that obeys the Fil-C type system.

- Every pointer carries a capability under the Fil-C type system. Each pointer knows its bounds and
  each pointer knows the type of the memory it points at. Fil-C will kill your program if you attempt
  out-of-bounds accesses or type-confused accesses (accessing an int as a ptr or vice-versa). Fil-C
  will kill your program if you try to access a function pointer or call a data pointer. Fil-C loves
  killing your program!

- Memory allocation in Fil-C prevents type confusion by isolating each Fil-C type into its own heap
  and being careful about type alignment. This makes free() into at worst a logic error. This also
  means that some code changes are necessary. You need to tell Fil-C the type at time of allocation.
  Fil-C's allocation API natively supports objects or arrays of objects of any size and alignment
  (including enormous alignments), flexes (objects with flexible array members, aka trailing arrays),
  cloning (allocate memory of a type that matches some other object), reflection (you can build up a
  type at runtime and ask the allocator to deal with it), and optional "hard" allocation (what OpenSSL
  calls the secure heap, so it's mlocked, DONT_DUMPed, zeroed on alloc and free, guarded with
  mprotected pages, and some other stuff).

- The Fil-C calling convention carries bounds and type for the arguments and return value, so calling
  a function with the wrong signature will result in Fil-C killing your program.

- Fil-C is careful about what it tells the linker about your program, so that it can protect type or
  bounds confusion arising from mislinked globals. If you define a global of one type in one file, and
  declare it extern with a different type (or different bounds) in another file, and then link and run,
  then the moment that the second file actually tries to access its type-confused reference, Fil-C
  will kill your program.

- Fil-C allocates all local variables in isoheaps, including ones created with alloca(). Escaping a
  pointer to a stack allocation does not lead to type confusion, but may lead to logic errors.

- Fil-C instruments va_arg and friends, so misusing it (even stashing/escaping it and then doing
  stuff after the function returns) will not break the Fil-C type system, but will most likely kill
  your program.

- I'm a lover of good GCs, so I've been designing Fil-C with that in mind. There isn't anything in
  Fil-C that prevents me switching to a real-time GC instead of isolated heaps. I have a sick algo in
  mind. But for now, I like how stuff Just Works with isoheaps.

- Although Fil-C pointers are 32 bytes, they are atomic by default, and are safe and useful to race
  on. Racing on a Fil-C pointer will never result in a more capable pointer than any of the pointers
  that were racily stored. Fil-C achieves this without requiring extra fences or atomic ops on pointer
  loads and stores. Fil-C pointers support wait-free compare-and-swap. I call this algorithm SideCap
  and I'll have to write it up at some point because it is hella dank.

- Fil-C has its own include and lib. To call a library from pizlonated code (code compiled with Fil-C),
  you must pizlonate that library. Fil-C's libc is a hacked musl. Its runtime, libpizlo, is based on
  libpas. The pizlonated musl makes syscalls by calling into the syscall surface area that libpizlo
  provides. See libpas/src/libpas/filc_runtime.c for most of the action. Currently everything gets put
  into the pizfix, so you can pizfix/bin/openssl or pizfix/bin/curl (kinda, there's more to do).

- There is no escape! Pizlonated code cannot call anything other than other pizlonated code or legacy
  C code that exposes itself using Fil-C ABI. The only such legacy C code that exists right now is in
  libpizlo and we should probably keep it that way. Therefore, from within Fil-C, it's not possible
  to escape Fil-C. It's just as GIMSO as Java. (It's even more GIMSO than Java if you put yourself in
  the Java boot classpath and go to town on sun.misc.Unsafe - Fil-C has nothing like that.)

Right now it's like 200x slower than C. But I can kinda sorta run curl with openssl and zlib. I can
download things over https with it. It's pretty cool.

This currently only works on Mac. To start, do:

    ./setup_gits.sh
    ./build_all.sh

Then you can try compiling your first memory-safe hello world like:

    xcrun build/bin/clang -o hello hello.c -g -O

Feel free to use <stdio.h> since musl mostly works.

If you want to play with Fil-C API, include <stdfil.h> (see filc/include/stdfil.h).

Note that the resulting binary will run without needing to specify a DYLD_LIBRARY_PATH because hacks.
