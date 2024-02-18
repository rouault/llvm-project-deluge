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
    struct bar* b = zalloc_flex(struct bar, z, 666);
    b->x = 42;
    b->y = zalloc(struct foo, 1);
    b->y->x = zalloc_flex(struct bar, z, 0);
    b->y->y = 1410;
    unsigned index;
    for (index = 666; index--;) {
        b->z[index].x = zalloc_flex(struct bar, z, index);
        b->z[index].y = 1000 - index;
    }

    unsigned size = __builtin_offsetof(struct bar, z) + 666 * sizeof(struct foo);
    char* buf = (char*)b;
    for (index = size; index--;)
        buf[index] = 'f';
    zprintf("FAIL\n");
    return 0;
}

