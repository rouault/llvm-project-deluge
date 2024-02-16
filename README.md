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
  stuff after the function returns) will not violate the Deluge type system, but will most likely kill
  your program.

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
  that were racily stores. Deluge achieves this without requiring extra fences or atomic ops on pointer
  loads and stores. Deluge pointers support wait-free compare-and-swap. I call this algorithm SideCap
  and I'll have to write it up at some point because it is hella dank.

- Deluge has its own include and lib. To call a library from deluded code, you must delude that
  library. Deluge's libc is a hacked musl. Its runtime, libdeluge, is based on libpas. The deluded musl
  makes syscalls by calling into the syscall surface area that libdeluge provides. See
  libpas/src/libpas/deluge_runtime.c for most of the action. Currently everything gets put into the
  pizfix, so you can pizfix/bin/openssl or pizfix/bin/curl (kinda, there's more to do).

- Deluge has a zunsafe_forge() primitive for forging any kind of capability you like, but I haven't
  used it anywhere other than the test suite. It's not even critical for the test suite, so I might
  remove it. I thought it was going to be useful, but it's not. And it's unsafe. So it's great that I
  haven't had to use it!

Right now it's like 200x slower than C. But I can kinda sorta run some of openssl and some of curl.
Zlib works fine.
