#include <functional>

void foo(const std::function<void()>& bar)
{
    bar();
}
