#include <stdfil.h>

int* opaque(int*);

int main(void)
{
    int x; /* This gets a word-sized box. */
    int* ptr = opaque(&x) + 1;
    *ptr = 42;
    if (*ptr == 42)
        zprint("YAY\n");
    else
        zerror("NAY\n");
    return 0;
}

