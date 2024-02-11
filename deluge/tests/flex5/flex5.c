#include <stdfil.h>

struct foo {
    struct bar* x;
    int y;
};

struct bar {
    int x;
    struct foo* y;
};

#define bar_z(b) ((unsigned short*)((b) + 1))

int main(int argc, char** argv)
{
    struct bar* b = zalloc_flex_cat(struct bar, unsigned short, 3333);
    b->x = 42;
    b->y = zalloc(struct foo, 1);
    b->y->x = zalloc_flex_cat_zero(struct bar, unsigned short, 0);
    b->y->y = 1410;
    unsigned index;
    for (index = 3333; index--;)
        bar_z(b)[index] = index;

    ZASSERT(b->x == 42);
    ZASSERT(!b->y->x->x);
    ZASSERT(!b->y->x->y);
    ZASSERT(b->y->y == 1410);
    for (index = 3333; index--;)
        ZASSERT(bar_z(b)[index] == index);
    zprintf("wporzo\n");
    return 0;
}

