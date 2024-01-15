#include <stdfil.h>

int main(int argc, char** argv)
{
    int fd = zsys_open("deluge/tests/fileio/test.txt", 0);
    ZASSERT(fd > 2);

    char buf[100];
    __builtin_memset(buf, 0, sizeof(buf));
    int result = zsys_read(fd, buf, sizeof(buf));
    ZASSERT(result == 14);

    zprintf("read: %s", buf);
    return 0;
}


