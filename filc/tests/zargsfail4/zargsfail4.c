#include <stdio.h>
#include <stdfil.h>

struct bad_args {
    char* x;
    char* y;
};
static void thingy(char* x)
{
    struct bad_args* args = zargs();
    ZASSERT(args->x == x);
    printf("%s\n", args->x);
    printf("%s\n", args->y);
}

int main()
{
    thingy("hello");
    return 0;
}

