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
    struct bar* b = zalloc(__builtin_offsetof(struct bar, z) + 3333 * sizeof(struct foo));
    b->x = 42;
    b->y = zalloc(sizeof(struct foo));
    b->y->x = zalloc(__builtin_offsetof(struct bar, z));
    b->y->y = 1410;
    unsigned index;
    for (index = 3333; index--;) {
        b->z[index].x = zalloc(__builtin_offsetof(struct bar, z) + (index % 666) * sizeof(struct foo));
        b->z[index].y = 1000 - index;
    }

    ZASSERT(b->x == 42);
    ZASSERT(!b->y->x->x);
    ZASSERT(!b->y->x->y);
    ZASSERT(b->y->y == 1410);
    for (index = 3333; index--;) {
        unsigned jndex;
        ZASSERT(!b->z[index].x->x);
        ZASSERT(!b->z[index].x->y);
        for (jndex = index % 666; jndex--;) {
            ZASSERT(!b->z[index].x->z[jndex].x);
            ZASSERT(!b->z[index].x->z[jndex].y);
        }
        ZASSERT(b->z[index].y == 1000 - index);
    }
    zprintf("wporzo\n");
    return 0;
}

