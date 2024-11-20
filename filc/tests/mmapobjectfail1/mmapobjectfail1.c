#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>

struct foo {
    int x;
    char* y;
};

void* opaque(void* ptr);

int main()
{
    struct foo* foo = mmap(0, 16384, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANON, -1, 0);
    foo->x = 42;
    foo->y = "hello";
    struct foo* foo2 = opaque(foo);
    zprintf("x = %d\n", foo2->x);
    zprintf("y = %s\n", foo2->y);
    ZASSERT(!munmap(foo2, 16384));
    return 0;
}

