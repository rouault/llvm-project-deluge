#include <stdfil.h>
#include <unistd.h>
#include <string.h>

void* opaque(void* p);

int main()
{
    char* str = opaque("hello");
    zprintf("Got this far.\n");
    getcwd(str, strlen(str));
    zprintf("Should not get this far.\n");
    return 0;
}

