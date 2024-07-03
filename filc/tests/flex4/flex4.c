#include <stdfil.h>

struct foo {
    struct bar* x;
    int y;
};

struct bar {
    int x;
    struct foo* y;
    unsigned short w;
    unsigned short z[1];
};

int main(int argc, char** argv)
{
    struct bar* b = zgc_alloc(__builtin_offsetof(struct bar, z) + 3333 * sizeof(unsigned short));
    b->x = 42;
    b->y = zgc_alloc(sizeof(struct foo));
    b->y->x = zgc_alloc(__builtin_offsetof(struct bar, z));
    b->y->y = 1410;
    b->w = 666;
    unsigned index;
    for (index = 3333; index--;)
        b->z[index] = index;

    ZASSERT(b->x == 42);
    ZASSERT(!b->y->x->x);
    ZASSERT(!b->y->x->y);
    ZASSERT(b->y->y == 1410);
    ZASSERT(b->w == 666);
    for (index = 3333; index--;)
        ZASSERT(b->z[index] == index);
    zprintf("wporzo\n");
    return 0;
}

