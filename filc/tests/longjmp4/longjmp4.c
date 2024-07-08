#include <setjmp.h>
#include <stdio.h>

static void doit()
{
    volatile int x = 42;
    jmp_buf jb;
    if (setjmp(jb)) {
        printf("(1) x = %d.", x);
        if (setjmp(jb)) {
            printf("(2) x = %d.", x);
            if (setjmp(jb)) {
                printf("(3) x = %d.", x);
                return;
            }
            x = 667;
            longjmp(jb, 1);
            printf("Should not get here.\n");
            return;
        }
        x = 666;
        longjmp(jb, 1);
        printf("Should not get here.\n");
        return;
    }
    x = 665;
    longjmp(jb, 1);
    printf("Should not get here.\n");
}

int main(int argc, char** argv)
{
    doit();
    printf("\n");
    return 0;
}

