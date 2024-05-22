#include <map>
#include <stdfil.h>

using namespace std;

int main(int argc, char** argv)
{
    map<int, int> m;

    m[42] = 666;
    m[1410] = 9000;
    m[666] = 11;

    ZASSERT(m.count(42) == 1);
    ZASSERT(m.count(1410) == 1);
    ZASSERT(m.count(666) == 1);

    ZASSERT(m[42] == 666);
    ZASSERT(m[1410] == 9000);
    ZASSERT(m[666] == 11);

    ZASSERT(!m[100]);

    size_t count = 0;
    for (auto& entry : m) {
        switch (count) {
        case 0:
            ZASSERT(entry.first == 42);
            ZASSERT(entry.second == 666);
            break;
        case 1:
            ZASSERT(entry.first == 100);
            ZASSERT(!entry.second);
            break;
        case 2:
            ZASSERT(entry.first == 666);
            ZASSERT(entry.second == 11);
            break;
        case 3:
            ZASSERT(entry.first == 1410);
            ZASSERT(entry.second == 9000);
            break;
        default:
            ZASSERT(!"Should not get here");
            break;
        }
        count++;
    }

    return 0;
}

