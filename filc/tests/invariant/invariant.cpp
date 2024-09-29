#include <iostream>
#include <atomic>

using namespace std;

struct Foo {
    Foo(): x(666) {}
    int x;
};

static __attribute__((__noinline__)) atomic<const Foo*>& foo()
{
    static const Foo foo;
    static atomic<const Foo*> bar(&foo);
    return bar;
}

int main()
{
    cout << foo().load()->x << endl;
    return 0;
}

