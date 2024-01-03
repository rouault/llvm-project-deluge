/* Shallow-clones the given object into the heap. You can call this on any pointer, even pointers to
   locals or globals. */
void* zclone(void* ptr);

/* Clones the arguments passed to the caller of this function. */
void* zcloneargs(void);

/* Clones the return buffer passed to the caller of the caller of this function. That's a bit weird, since
   the buffer is zeroed by the caller - so you'll always get a zero-filled allocation. But, the allocation
   will have exactly the type that the caller had expected. */
void* zclonerets(void);

/* Copies the contents of the given object (all of it) to the caller function's rets buffer. This will
   fail if the type or size are wrong. */
void zsplatrets(void* src);

/* Calls the given functions with the given arguments object and return object.
 
   Here's an example. Say you wanted to write a function that forwards all of its arguments (no matter
   what signature it's called with) to some other function, and then forwards that function's returns to
   the caller. Here's what it would look like, if we want foo to foward to bar. Note that bar could have
   any signature.

   void foo(void)
   {
       void* args = zcloneargs();
       void* rets = zclonerets();
       zcall(bar, args, rets);
       zsplatrets(rets);
       zfree(args);
       zfree(rets);
   }

   Note that foo taking void doesn't matter at all to the underlying Deluge calling convention. The fact
   that it returns void means that returning from foo won't write anything to rets, so zsplatrets's value
   will survive. */
void zcall(void* callee, void* args, void* rets);

