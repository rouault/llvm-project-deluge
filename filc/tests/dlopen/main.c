#include <stdio.h>
#include <dlfcn.h>
#include <stdfil.h>

void* opaque(void*);

int main()
{
    void* lib = dlopen("filc/tests/dlopen/libtest.so", RTLD_LAZY | RTLD_LOCAL);
    ZASSERT(lib);

    int* (*foo)(void) = dlsym(lib, "foo");
    ZASSERT(foo);

    int* x_ptr = foo();
    ZASSERT(*x_ptr == 42);
    *x_ptr = 100;
    ZASSERT(*(int*)opaque(x_ptr) == 100);

    int* bar_ptr = dlsym(lib, "bar");
    ZASSERT(bar_ptr);
    ZASSERT(*bar_ptr == 666);
    *bar_ptr = 200;
    ZASSERT(*(int*)opaque(bar_ptr) == 200);

    dlclose(lib);

    return 0;
}

