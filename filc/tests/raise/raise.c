#include <signal.h>
#include <stdfil.h>

int main()
{
    zprintf("so far so good\n");
    raise(SIGTERM);
    zprintf("fail\n");
    return 0;
}

