#include <setjmp.h>
#include <stdio.h>
#include <stdfil.h>

int main(int argc, char** argv)
{
    unsigned i;
    for (i = 1000; i--;) {
        int x = 42;
        jmp_buf jb;
        if (setjmp(jb)) {
            ZASSERT(x == 666);
            continue;
        }
        x = 666;
        longjmp(jb, 1);
        printf("Should not get here.\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}

