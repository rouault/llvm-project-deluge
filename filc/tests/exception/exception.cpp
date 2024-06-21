#include <iostream>

using namespace std;;

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
        cout << "RAII construct.";
    }

    ~RAII()
    {
        cout << "RAII destruct.";
    }
};

void foo()
{
    throw Exception(666);
}

void bar()
{
    RAII raii;
    foo();
}

int main(int argc, char** argv)
{
    try {
        bar();
        cout << "Didn't catch exception.";
    } catch (Exception e) {
        cout << "Caught exception: x = " << e.x() << ".";
    } catch (...) {
        cout << "Caught wrong exception" << ".";
    }
    cout << "The end" << endl;
    return 0;
}

