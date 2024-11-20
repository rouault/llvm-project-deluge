#include <stdfil.h>
#include <stdbool.h>
#include <string.h>

static bool callback(zstack_frame_description description,
                     void* arg)
{
    *(char**)description.function_name = "hello";
    return true;
}

static void foo(void)
{
    zstack_scan(callback, (void*)666);
}

static void bar(void)
{
    foo();
}

int main()
{
    bar();
    return 0;
}

