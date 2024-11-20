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
    struct foo* foo = mmap(
        0, sizeof(struct foo), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
    foo->x = 42;
    foo->y = "hello";
    struct foo* foo2 = opaque(foo);
    printf("x = %d\n", foo2->x);
    printf("y = %s\n", foo2->y);
    ZASSERT(!munmap(foo2, 4096));
    return 0;
}

