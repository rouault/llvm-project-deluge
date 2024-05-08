#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct foo {
    char* str;
    int x;
    int array[1];
};

int main()
{
    struct foo* f1 = malloc(__builtin_offsetof(struct foo, array) + sizeof(int) * 42);
    f1->str = "hello";
    f1->x = 666;
    size_t index;
    for (index = 42; index--;)
        f1->array[index] = index;

    struct foo* f2 = malloc(__builtin_offsetof(struct foo, array) + sizeof(int) * 666);
    memcpy(f2, f1, __builtin_offsetof(struct foo, array));
    for (index = 666; index--;)
        f2->array[index] = 666 - index;

    printf("%s %d\n", f2->str, f2->x);
    return 0;
}

