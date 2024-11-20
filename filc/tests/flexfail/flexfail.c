#include <stdlib.h>
#include <stdio.h>
#include <stdfil.h>

struct foo {
    struct bar* x;
    int y;
};

struct bar {
    int x;
    struct foo* y;
    struct foo z[1];
};

int main(int argc, char** argv)
{
    struct bar* b = malloc(__builtin_offsetof(struct bar, z) + 666 * sizeof(struct foo));
    b->x = 42;
    b->y = malloc(sizeof(struct foo));
    b->y->x = malloc(__builtin_offsetof(struct bar, z));
    b->y->y = 1410;
    unsigned index;
    for (index = 666; index--;) {
        b->z[index].x = malloc(__builtin_offsetof(struct bar, z) + index * sizeof(struct foo));
        b->z[index].y = 1000 - index;
    }

    unsigned size = __builtin_offsetof(struct bar, z) + 666 * sizeof(struct foo);
    char* buf = (char*)b;
    for (index = size; index--;)
        buf[index] = 'f';

    ZASSERT(!zinbounds(b->y));
    for (index = 666; index--;)
        ZASSERT(!zinbounds(b->z[index].x));
    
    return 0;
}

