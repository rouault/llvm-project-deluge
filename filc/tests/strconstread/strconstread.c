#include <stdfil.h>
#include <unistd.h>

void* opaque(void* p);

int main()
{
    char* str = opaque("hello");
    zprintf("Got this far.\n");
    read(0, str, 1);
    zprintf("Should not get this far.\n");
    return 0;
}

