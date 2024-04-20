#include <unistd.h>
#include <signal.h>
#include <stdfil.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

static bool gotit = false;

static void handler(int signo)
{
    ZASSERT(signo == SIGALRM);
    gotit = true;
}

int main()
{
    signal(SIGALRM, handler);
    ualarm(1000, 0);
    while (!gotit) {
        /* FIXME: This should use sigwait! */
        ZASSERT(pause() < 0);
        ZASSERT(errno == EINTR);
    }
    printf("Znakomicie\n");
    return 0;
}

