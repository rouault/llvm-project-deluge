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
    int result = ioctl(fds[0], FIONREAD, 666);
    int my_errno = errno;
    ZASSERT(result == -1);
    ZASSERT(my_errno == EFAULT);
    return 0;
}

