#include <stdlib.h>
#include <string.h>
#include <stdfil.h>

struct foo {
    int x;
    int y;
    char* str;
    int z;
    int w;
};

int main()
{
    struct foo a;

    a.x = 1;
    a.y = 2;
    a.str = "hello";
    a.z = 3;
    a.w = 4;

    zmemmove(&a, &a.str, sizeof(struct foo) - __builtin_offsetof(struct foo, str));

    return 0;
}

