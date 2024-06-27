/* Test that readv/writev unpin everything that they pin. */

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdfil.h>

int main()
{
    int fds[2];
    ZASSERT(!pipe(fds));

    struct iovec iov[2];
    char c1;
    char c2;
    iov[0].iov_base = &c1;
    iov[0].iov_len = 1;
    iov[1].iov_base = &c2;
    iov[1].iov_len = 1;

    size_t index;
    for (index = 100000; index--;) {
        c1 = (char)(index % 42);
        c2 = (char)(index % 43);
        ZASSERT(writev(fds[1], &iov, 2) == 2);
        ZASSERT(c1 == (char)(index % 42));
        ZASSERT(c2 == (char)(index % 43));
        c1 = 0;
        c2 = 0;
        ZASSERT(readv(fds[0], &iov, 2) == 2);
        ZASSERT(c1 == (char)(index % 42));
        ZASSERT(c2 == (char)(index % 43));
    }

    return 0;
}

