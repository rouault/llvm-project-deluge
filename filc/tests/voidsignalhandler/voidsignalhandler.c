#include <signal.h>
#include <stdfil.h>
#include <stdbool.h>

static bool was_called = false;

static void signal_handler(int signo)
{
    ZASSERT(signo == SIGUSR1);
    ZASSERT(!was_called);
    was_called = true;
}

int main()
{
    ZASSERT(signal(SIGUSR1, signal_handler) == SIG_DFL);
    ZASSERT(!raise(SIGUSR1));
    ZASSERT(was_called);
    return 0;
}

