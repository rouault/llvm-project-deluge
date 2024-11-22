#include <sys/ioctl.h>
#include <unistd.h>
#include <stdfil.h>
#include <errno.h>

int main()
{
    int fds[2];
    ZASSERT(!pipe(fds));
    ZASSERT(fds[0] > 2);
    ZASSERT(fds[1] > 2);
    ioctl(fds[0], FIONREAD, (void*)0x100000000lu);
    return 0;
}

