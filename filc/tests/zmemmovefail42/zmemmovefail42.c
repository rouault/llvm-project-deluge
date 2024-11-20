#include <stdfil.h>

int main()
{
    zptrtable* src = zptrtable_new();
    char* dst = "hello";

    zmemmove(&dst, src, 8);

    return 0;
}

