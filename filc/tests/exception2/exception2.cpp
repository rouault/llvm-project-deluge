#include <iostream>
#include <exception>
#include <string>

using namespace std;

// Overkill lyrics mean it's working correctly.
// Taylor Swift lyrics mean it's not.

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

class RAII {
public:
    RAII()
    {
        cout << "Every time I turn around.";
    }

    ~RAII()
    {
        cout << "I start to hear a familiar sound.";
    }
};

void foo()
{
    throw Exception("Counting down as time stands still.");
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
        cout << "Fever dream high in the quiet of the night.";
    } catch (exception& e) {
        cout << "Caught exception: what = " << e.what() << ".";
    } catch (...) {
        cout << "You know that I caught it." << ".";
    }
    cout << "Yes I think it's time to kill." << endl;
    return 0;
}

