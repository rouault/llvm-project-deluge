#include <sys/eventfd.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdfil.h>

int main()
{
    int efd;
    uint64_t u;
    ssize_t s;

    efd = eventfd(0, 0);
    ZASSERT(efd > 2);

    int fork_result = fork();
    ZASSERT(fork_result >= 0);
    if (!fork_result) {
        u = 666;
        s = write(efd, &u, sizeof(uint64_t));
        ZASSERT(s == sizeof(uint64_t));
        return 0;
    }
    
    s = read(efd, &u, sizeof(uint64_t));
    ZASSERT(s == sizeof(uint64_t));
    ZASSERT(u == 666);
    return 0;
}
