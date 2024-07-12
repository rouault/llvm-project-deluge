#include <stdfil.h>
#include <stdbool.h>

static const char* foo = "foo";

static bool was_called = false;

static void* unpark_callback(bool did_unpark_thread,
                             bool may_have_more_threads,
                             void* arg)
{
    ZASSERT(!did_unpark_thread);
    ZASSERT(!may_have_more_threads);
    ZASSERT(foo == arg);
    was_called = true;
    return "hello";
}

int main()
{
    int x;
    zunpark_one(&x, (void (*)(filc_bool, filc_bool, void*))unpark_callback, foo);
    ZASSERT(was_called);
    return 0;
}

