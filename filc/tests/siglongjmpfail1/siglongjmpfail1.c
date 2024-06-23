#include <setjmp.h>
#include <stdio.h>

void* opaque(void*);

int main(int argc, char** argv)
{
    int x = (int)opaque((void*)42);
    sigjmp_buf jb;
    if (sigsetjmp(jb, 1)) {
        printf("x = %d\n", x);
        return 0;
    }
    x = 666;
    longjmp(jb, 1);
    printf("Should not get here.\n");
    return 1;
}

