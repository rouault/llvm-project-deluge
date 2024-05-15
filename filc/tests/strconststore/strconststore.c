#include <stdfil.h>

void* opaque(void* p);

int main()
{
    char* str = opaque("hello");
    zprintf("Got this far.\n");
    str[0] = 'j';
    zprintf("Should not get this far.\n");
    return 0;
}

