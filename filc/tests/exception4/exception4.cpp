#include <iostream>
#include <string>
#include <exception>

using namespace std;

class Exception : public exception {
public:
    Exception(const string& str) : m_str(str) { }
    virtual ~Exception() throw() { }
    virtual const char* what() const throw()
    {
        return m_str.c_str();
    }
private:
    string m_str;
};

static void foo()
{
    throw Exception("oops");
}

static void bar() throw()
{
    foo();
}

int main(int argc, char** argv)
{
    try {
        bar();
    } catch (exception& e) {
        cout << "caught exception, what = " << e.what() << endl;
    } catch (...) {
        cout << "caught something." << endl;
    }
    return 0;
}
