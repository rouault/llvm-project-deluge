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
    volatile int x = 42;
    int result = sigsetjmp(jb, 1);
    if (result) {
        ZASSERT(result == 1410);
        printf("x = %d\n", x);
        return 0;
    }
    signal(SIGUSR1, handler);
    x = 666;
    raise(SIGUSR1);
    printf("Should not get here.\n");
    return 1;
}

