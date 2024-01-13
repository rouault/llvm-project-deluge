#include <stdfil.h>

struct foo {
    struct bar* x;
    int y;
};

struct bar {
    int x;
    struct foo* y;
    unsigned short z[1];
};

int main(int argc, char** argv)
{
    struct bar* b = zalloc_flex(struct bar, z, 33333);
    b->x = 42;
    b->y = zalloc(struct foo, 1);
    b->y->x = zalloc_flex_zero(struct bar, z, 0);
    b->y->y = 1410;
    unsigned index;
    for (index = 33333; index--;)
        b->z[index] = index;

    ZASSERT(b->x == 42);
    ZASSERT(!b->y->x->x);
    ZASSERT(!b->y->x->y);
    ZASSERT(b->y->y == 1410);
    for (index = 33333; index--;)
        ZASSERT(b->z[index] == index);
    zprintf("wporzo\n");
    return 0;
}

