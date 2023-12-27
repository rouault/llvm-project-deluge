#include <stdfil.h>

struct foo {
    int x;
    char* string;
    int y;
};

#define OFFSETOF(type, field) ((char*)&(((type*)666)->field) - (char*)666)

#define PRINT(exp) do { \
        zprint(#exp " = "); \
        zprint_long(exp); \
        zprint("\n"); \
    } while (0)

int main(void)
{
    PRINT(sizeof(void*));
    PRINT(sizeof(struct foo));
    PRINT(__builtin_offsetof(struct foo, x));
    PRINT(__builtin_offsetof(struct foo, string));
    PRINT(__builtin_offsetof(struct foo, y));
    PRINT(OFFSETOF(struct foo, x));
    PRINT(OFFSETOF(struct foo, string));
    PRINT(OFFSETOF(struct foo, y));
    return 0;
}

