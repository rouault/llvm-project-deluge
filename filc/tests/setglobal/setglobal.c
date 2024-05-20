#include <stdfil.h>

static char buf[256] = { 1 };

void* opaque(void* ptr);

int main()
{
    char** strptr = (char**)buf;
    *strptr = "hello";
    strptr = opaque(strptr);
    zprintf("%s\n", *strptr);
    return 0;
}

