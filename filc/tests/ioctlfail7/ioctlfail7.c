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
    int result = ioctl(fds[0], FIONREAD, (char*)zgetupper(&data) - 1);
    int my_errno = errno;
    zprintf("result = %d, error = %s\n", result, strerror(my_errno));
    return 0;
}

