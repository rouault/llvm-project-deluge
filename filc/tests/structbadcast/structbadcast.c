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
    struct foo* ptr = zalloc(sizeof(struct foo));
    ptr->x = 42;
    ptr->string = "hello";
    struct bar* ptr2 = (struct bar*)ptr;
    if (ptr2->x == 42)
        zprint("NO\n");
    if (ptr2->string == "hello")
        zprint("NO\n");
    return 0;
}

