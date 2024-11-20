#include <stdfil.h>
#include <string.h>

int main()
{
    char* array[4];
    unsigned i;
    for (i = 4; i--;)
        array[i] = "bruh";
    zmemset((char*)array + 16, 0, 15);
    ZASSERT(!strcmp(array[0], "bruh"));
    ZASSERT(!strcmp(array[1], "bruh"));
    ZASSERT(!array[2]);
    ZASSERT(!array[3]);
    ZASSERT(!zhasvalidcap(array[2]));
    ZASSERT(!zhasvalidcap(array[3]));
    return 0;
}

