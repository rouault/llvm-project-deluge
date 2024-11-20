#include <stdfil.h>
#include <atomic>
#include <functional>

void foo(const std::function<void()>& bar);

int main()
{
    const char* x = "hello";
    foo([&] () {
        zprintf("&x = %P\n", &x);
        const char* v = x;
        zprintf("x = %s\n", v);
    });
    return 0;
}

