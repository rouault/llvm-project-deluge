#include <stdfil.h>

static const char* const str = "hello";

void* opaque(void* p);

int main()
{
    char** strptr = opaque(&str);
    zprintf("Got this far.\n");
    *strptr = "world";
    zprintf("Should not get this far.\n");
    return 0;
}

