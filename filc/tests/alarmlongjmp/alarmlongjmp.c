#include <unistd.h>
#include <signal.h>
#include <stdfil.h>
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>

static jmp_buf jb;

static void handler(int signo)
{
    ZASSERT(signo == SIGALRM);
    longjmp(jb, 1);
}

int main()
{
    signal(SIGALRM, handler);
    if (setjmp(jb)) {
        printf("Znakomicie\n");
        return 0;
    }
    ualarm(1000, 0);
    for (;;)
        zcompiler_fence();
    return 1;
}

