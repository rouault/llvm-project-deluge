#include <stdfil.h>

using namespace std;

static unsigned counter;

class Exception {
public:
    Exception(int x)
        : m_x(x)
    {
    }

    int x() const { return m_x; }

private:
    int m_x;
};

class RAII {
public:
    RAII()
    {
        ZASSERT(++counter == 1);
    }

    ~RAII()
    {
        ZASSERT(++counter == 3);
    }
};

void foo()
{
    throw Exception(++counter);
}

void bar()
{
    RAII raii;
    foo();
}

void doit()
{
    try {
        bar();
        ZASSERT(!"Should not get here");
    } catch (Exception e) {
        ZASSERT(e.x() == 2);
    } catch (...) {
        ZASSERT(!"Should not get here");
    }
    ZASSERT(counter == 3);
}

int main(int argc, char** argv)
{
    for (unsigned i = 10000; i--;) {
        counter = 0;
        doit();
    }
    return 0;
}

