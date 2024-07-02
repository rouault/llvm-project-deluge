/* Tests that repeated use of the same buffer on read/write doesn't cause a pin overflow. */

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

    unsigned index2;
    for (index2 = 10000; index2--;) {
        ZASSERT(write(fds[1], buf, 100) == 100);
        memset(buf, 0, 100);
        for (index = 100; index--;)
            ZASSERT(!buf[index]);
        ZASSERT(read(fds[0], buf, 100) == 100);
        for (index = 100; index--;)
            ZASSERT(buf[index] == (char)(index + 666));
    }

    return 0;
}
