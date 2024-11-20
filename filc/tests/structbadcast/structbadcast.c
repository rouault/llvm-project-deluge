#include <stdfil.h>

struct foo {
    int x;
    char* string;
    float* whatever;
};

struct bar {
    char* string;
    int x;
};

#define PRINT(exp) do { \
        zprint(#exp " = "); \
        zprint_long(exp); \
        zprint("\n"); \
    } while (0)

int main(void) {
    PRINT(sizeof(struct foo));
    struct foo* ptr = zgc_alloc(sizeof(struct foo));
    char* hello = "hello";
    ptr->x = 42;
    ptr->string = hello;
    struct bar* ptr2 = (struct bar*)ptr;
    zprintf("x = %d\n", ptr2->x); /* This'll be some weird value. */
    ZASSERT(ptr2->x == (int)hello);
    ZASSERT(ptr2->string != hello);
    ZASSERT(!zhasvalidcap(ptr2->string));
    return 0;
}

