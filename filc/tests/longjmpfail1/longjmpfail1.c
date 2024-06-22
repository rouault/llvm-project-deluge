#include <setjmp.h>
#include <stdio.h>

void* opaque(void*);

int helper(jmp_buf jb);

int main(int argc, char** argv)
{
    int x = (int)opaque((void*)42);
    jmp_buf jb;
    if (helper(jb)) {
        printf("x = %d\n", x);
        return 0;
    }
    x = 666;
    longjmp(jb, 1);
    printf("Should not get here.\n");
    return 1;
}

