#include <sys/ioctl.h>
#include <unistd.h>
#include <stdfil.h>
#include <errno.h>
#include <string.h>

int main()
{
    int fds[2];
    ZASSERT(!pipe(fds));
    ZASSERT(fds[0] > 2);
    ZASSERT(fds[1] > 2);
    int data;
    ZASSERT(!ioctl(fds[0], FIONREAD, (char*)zgetupper(&data) - 4));
    return 0;
}

