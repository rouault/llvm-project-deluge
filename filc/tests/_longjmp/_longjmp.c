#include <setjmp.h>
#include <stdio.h>

void* opaque(void*);

int main(int argc, char** argv)
{
    int x = (int)opaque((void*)42);
    jmp_buf jb;
    if (_setjmp(jb)) {
        printf("x = %d\n", x);
        return 0;
    }
    x = 666;
    _longjmp(jb, 1);
    printf("Should not get here.\n");
    return 1;
}

