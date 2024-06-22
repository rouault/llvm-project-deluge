#include <setjmp.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

void* opaque(void*);

void doer(jmp_buf jb, bool do_jump)
{
    int x = (int)opaque((void*)42);
    if (!do_jump) {
        if (setjmp(jb)) {
            printf("x = %d\n", x);
            exit(0);
        }
        return;
    }
    x = 666;
    longjmp(jb, 1);
    printf("Should not get here.\n");
    exit(1);
}

