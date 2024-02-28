#include <signal.h>
#include <stdfil.h>
#include <stdlib.h>

int main()
{
    sigset_t set, oldset;
    sigfillset(&set);
    ZASSERT(!pthread_sigmask(SIG_BLOCK, &set, &oldset));
    raise(SIGTERM);
    zprintf("Still here!\n");
    ZASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    zprintf("Oops I should be dead!\n");
    return 0;
}

