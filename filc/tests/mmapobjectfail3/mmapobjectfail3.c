#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>
#include <fcntl.h>

struct foo {
    int x;
    char* y;
};

void* opaque(void* ptr);

int main()
{
    unlink("filc/test-output/mmapobjectfail3/mmapfile.dat");
    int fd = open("filc/test-output/mmapobjectfail3/mmapfile.dat", O_CREAT | O_RDWR, 0644);
    ZASSERT(fd > 2);

    ZASSERT(!ftruncate(fd, 16384));

    struct foo* foo = mmap(0, 16384, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    foo->x = 42;
    zprintf("Got this far!\n");
    
    foo->y = "hello";
    struct foo* foo2 = opaque(foo);
    zprintf("x = %d\n", foo2->x);
    zprintf("y = %s\n", foo2->y);
    ZASSERT(!munmap(foo2, 16384));
    return 0;
}

