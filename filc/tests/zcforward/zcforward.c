#include <stdfil.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int foo(int x, char* y)
{
    return x + strlen(y);
}

static void bar(void)
{
    zcforward(foo);
}

static void universal_forward(void* target_function)
{
    zreturn(zcall(target_function, (void**)zargs() + 1, ZC_RET_BYTES), ZC_RET_BYTES);
}

static char* thingy(int x, int y)
{
    char* result;
    asprintf(&result, "%d, %d", x, y);
    return result;
}

static int stuff(void)
{
    zreturn_value(1410);
}

int main()
{
    int (*f)(int, char*) = (int (*)(int x, char* y))bar;
    ZASSERT(f(42, "hello") == 47);

    int (*g)(void*, int, char*) = (int (*)(void*, int, char*))universal_forward;
    ZASSERT(g(foo, 666, "blah") == 670);

    char* (*h)(void*, int, int) = (char* (*)(void*, int, int))universal_forward;
    ZASSERT(!strcmp(h(thingy, 1400, 10), "1400, 10"));

    ZASSERT(stuff() == 1410);
    ZASSERT(zcall_value(stuff, NULL, int) == 1410);

    struct {
        int x;
        char* y;
    } args;
    args.x = 10;
    args.y = "skibidi";
    ZASSERT(zcall_value(foo, &args, int) == 17);
    
    return 0;
}

