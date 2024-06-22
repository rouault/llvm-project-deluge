#include <stdbool.h>
#include <setjmp.h>

int doer(jmp_buf jb, bool do_jump);

int main(int argc, char** argv)
{
    jmp_buf jb;
    doer(jb, false);
    doer(jb, true);
    return 0;
}

