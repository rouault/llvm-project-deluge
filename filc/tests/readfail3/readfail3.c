#include <unistd.h>
#include <stdfil.h>
#include <string.h>

int main()
{
    int fds[2];
    ZASSERT(!pipe(fds));

    char buf[100];
    unsigned index;
    for (index = 100; index--;)
        buf[index] = (char)(index + 666);

    ZASSERT(write(fds[1], buf, 100) == 100);

    char* array[100];
    for (index = 100; index--;)
        array[index] = "hello";
    ZASSERT(read(fds[0], array, 100) == 100);
    for (index = 100; index--;) {
        ZASSERT(((char*)array)[index] == (char)(index + 666));
        ZASSERT(!strcmp((char*)zgetlower(array[index]), "hello"));
    }
    for (index = (100 + sizeof(char*) - 1) / sizeof(char*) + 1; index < 100; ++index)
        ZASSERT(!strcmp(array[index], "hello"));
    return 0;
}

