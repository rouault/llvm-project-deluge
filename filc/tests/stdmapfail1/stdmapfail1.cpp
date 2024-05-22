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

    string& ref = m["index = 500"];
    ZASSERT(ref == "value = 250000");

    zprintf("(good) ref = %s.\n", ref.c_str());

    m.erase("index = 500");

    zprintf("Got this far.\n");

    zprintf("(bad) ref = %s.\n", ref.c_str());

    return 0;
}

