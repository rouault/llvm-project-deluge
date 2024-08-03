#include <stdfil.h>

int main()
{
    zptrtable* dst = zptrtable_new();
    char* src = "hello";

    zmemmove(dst, &src, 16);

    return 0;
}

