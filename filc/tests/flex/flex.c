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
    struct bar* b = zgc_alloc(__builtin_offsetof(struct bar, z) + 666 * sizeof(struct foo));
    b->x = 42;
    b->y = zgc_alloc(sizeof(struct foo));
    b->y->x = zgc_alloc(__builtin_offsetof(struct bar, z));
    b->y->y = 1410;
    unsigned index;
    for (index = 666; index--;) {
        b->z[index].x = zgc_alloc(__builtin_offsetof(struct bar, z) + index * sizeof(struct foo));
        b->z[index].y = 1000 - index;
    }

    ZASSERT(b->x == 42);
    ZASSERT(!b->y->x->x);
    ZASSERT(!b->y->x->y);
    ZASSERT(b->y->y == 1410);
    for (index = 666; index--;) {
        unsigned jndex;
        ZASSERT(!b->z[index].x->x);
        ZASSERT(!b->z[index].x->y);
        for (jndex = index; jndex--;) {
            ZASSERT(!b->z[index].x->z[jndex].x);
            ZASSERT(!b->z[index].x->z[jndex].y);
        }
        ZASSERT(b->z[index].y == 1000 - index);
    }
    zprintf("wporzo\n");
    return 0;
}

