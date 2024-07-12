#include <signal.h>
#include <stdio.h>
#include <stdfil.h>
#include <setjmp.h>

static sigjmp_buf jb;

static void handler(int signo)
{
    siglongjmp(jb, 1410);
}

int main()
{
    signal(SIGUSR1, handler);
    volatile unsigned i;
    for (i = 1000; i--;) {
        volatile int x = 42;
        int result = sigsetjmp(jb, 1);
        if (result) {
            ZASSERT(result == 1410);
            ZASSERT(x == 666);
            continue;
        }
        x = 666;
        raise(SIGUSR1);
        printf("Should not get here.\n");
        return 1;
    }
    return 0;
}

