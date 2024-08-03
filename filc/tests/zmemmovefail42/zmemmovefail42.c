#include <stdfil.h>

int main()
{
    zptrtable* src = zptrtable_new();
    char* dst = "hello";

    zmemmove(&dst, src, 16);

    return 0;
}

