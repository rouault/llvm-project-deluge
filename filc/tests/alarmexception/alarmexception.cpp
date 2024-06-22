#define _GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include <stdfil.h>
#include <stdbool.h>
#include <string>
#include <exception>
#include <iostream>

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

static void handler(int signo)
{
    ZASSERT(signo == SIGALRM);
    throw Exception("wtf");
}

int main()
{
    signal(SIGALRM, handler);
    ualarm(1000, 0);
    for (;;)
        zcompiler_fence();
    cout << "Should not get here." << endl;
    return 0;
}

