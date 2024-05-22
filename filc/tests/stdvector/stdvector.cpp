#include <vector>
#include <stdfil.h>

using namespace std;

int main(int argc, char** artv)
{
    vector<int> v;

    for (size_t index = 0; index < 1000; ++index)
        v.push_back(index * index);

    for (size_t index = 1000; index--;)
        ZASSERT(v[index] == index * index);

    size_t count;
    for (int& entry : v) {
        ZASSERT(entry == count * count);
        count++;
    }
    
    return 0;
}

