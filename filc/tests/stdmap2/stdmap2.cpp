#include <map>
#include <string>
#include <sstream>
#include <stdfil.h>

using namespace std;

int main(int argc, char** argv)
{
    map<string, string> m;

    for (size_t index = 1000; index--;) {
        ostringstream buf1;
        buf1 << "index = " << index;

        ostringstream buf2;
        buf2 << "value = " << (index * index);

        m[buf1.str()] = buf2.str();
    }

    for (size_t index = 1000; index--;)
        ZASSERT(m[zasprintf("index = %d", index)] == zasprintf("value = %d", index * index));

    return 0;
}

