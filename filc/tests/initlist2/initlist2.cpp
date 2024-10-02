#include <initializer_list>
#include <vector>
#include <iostream>

using namespace std;

static constexpr size_t foo(initializer_list<int> l)
{
    return l.size();
}

int main()
{
    cout << foo({1, 2}) << endl;
    return 0;
}

