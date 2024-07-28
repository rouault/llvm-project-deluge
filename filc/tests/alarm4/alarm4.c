#include <unistd.h>
#include <signal.h>
#include <stdfil.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

static bool gotit = false;

static void handler(int signo)
{
    ZASSERT(signo == SIGALRM);
    gotit = true;
}

int main()
{
    signal(SIGALRM, handler);
    sigset_t set;
    ZASSERT(!sigemptyset(&set));
    ZASSERT(!sigaddset(&set, SIGALRM));
    ZASSERT(!pthread_sigmask(SIG_BLOCK, &set, NULL));
    ualarm(1000, 0);
    ZASSERT(!sigemptyset(&set));
    ZASSERT(sigsuspend(&set) == -1);
    ZASSERT(errno == EINTR);
    printf("Znakomicie\n");
    return 0;
}

