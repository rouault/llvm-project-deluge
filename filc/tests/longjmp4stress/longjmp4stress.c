#include <setjmp.h>
#include <stdfil.h>

static void doit()
{
    volatile int x = 42;
    jmp_buf jb;
    int result;
    if ((result = setjmp(jb))) {
        ZASSERT(result == 1);
        ZASSERT(x == 665);
        if ((result = setjmp(jb))) {
            ZASSERT(result == 2);
            ZASSERT(x == 666);
            if ((result = setjmp(jb))) {
                ZASSERT(result == 3);
                ZASSERT(x == 667);
                return;
            }
            x = 667;
            longjmp(jb, 3);
            ZASSERT(!"Should not get here.\n");
            return;
        }
        x = 666;
        longjmp(jb, 2);
        ZASSERT(!"Should not get here.\n");
        return;
    }
    x = 665;
    longjmp(jb, 1);
    ZASSERT(!"Should not get here.\n");
}

int main(int argc, char** argv)
{
    unsigned i;
    for (i = 10000; i--;)
        doit();
    return 0;
}

