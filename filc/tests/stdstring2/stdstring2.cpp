#include <iostream>
#include <string>
#include <stdfil.h>

using namespace std;

int main()
{
    string buf = "a";

    for (unsigned count = 1000; count--;)
        buf += "b";

    buf += "c";

    string buf2;
    buf2 = move(buf);
    ZASSERT(!buf.size());
    ZASSERT(buf == "");

    string buf3 = "yo yo yo";
    buf3 = move(buf2);
    ZASSERT(!buf2.size());
    ZASSERT(buf2 == "");

    buf = move(buf3);
    ZASSERT(!buf3.size());
    ZASSERT(buf3 == "");

    cout << buf << endl;
    return 0;
}

