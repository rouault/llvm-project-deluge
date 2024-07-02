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
    read(fds[0], array, 100);
    return 0;
}

