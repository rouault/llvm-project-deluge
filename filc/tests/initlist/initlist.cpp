#include <initializer_list>
#include <vector>
#include <iostream>

using namespace std;

static constexpr vector<int> foo(initializer_list<int> l)
{
    return vector<int>(l);
}

int main()
{
    vector<int> v = foo({1, 2});
    for (size_t i = 0; i < v.size(); ++i) {
        if (i)
            cout << ",";
        cout << i << ":" << v[i];
    }
    cout << endl;
    return 0;
}

