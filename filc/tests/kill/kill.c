#include <signal.h>
#include <unistd.h>
#include <stdfil.h>

int main()
{
    zprintf("so far so good\n");
    kill(getpid(), SIGTERM);
    zprintf("fail\n");
    return 0;
}

