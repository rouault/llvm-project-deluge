#include <setjmp.h>

int helper(jmp_buf jb)
{
    return setjmp(jb);
}
