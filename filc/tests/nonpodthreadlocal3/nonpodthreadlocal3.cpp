#include <map>
#include <set>
#include <string>
#include <stdfil.h>

using namespace std;

struct S {
    map<int, vector<int>> x;
    set<string> y;
};

thread_local S s;

void foo()
{
    s.x[42].push_back(666);
    s.y.insert("hello");
}

void bar()
{
    ZASSERT(s.x[42][0] == 666);
    ZASSERT(s.y.count("hello"));
}

int main()
{
    foo();
    bar();
    return 0;
}
