#include <setjmp.h>
#include <stdio.h>

static void doit(int mode)
{
    volatile int x = 42;
    jmp_buf jb1;
    jmp_buf jb2;
    jmp_buf jb3;
    if (setjmp(jb1)) {
        printf("(1) x = %d.", x);
        return;
    }
    if (setjmp(jb2)) {
        printf("(2) x = %d.", x);
        return;
    }
    if (setjmp(jb3)) {
        printf("(3) x = %d.", x);
        return;
    }
    x = 666;
    switch (mode) {
    case 1:
        longjmp(jb1, 1);
        break;
    case 2:
        longjmp(jb2, 1);
        break;
    case 3:
        longjmp(jb3, 1);
        break;
    default:
        break;
    }
    printf("Should not get here.\n");
}

int main(int argc, char** argv)
{
    doit(1);
    doit(2);
    doit(3);
    printf("\n");
    return 0;
}

