#include <setjmp.h>
#include <inttypes.h>
#include <iostream>

using namespace std;

extern "C" void* opaque(void*);

int main(int argc, char** argv)
{
    try {
        int x = (int)(uintptr_t)opaque((void*)42);
        jmp_buf jb;
        if (setjmp(jb)) {
            cout << "x = " << x << endl;
            return 0;
        }
        x = 666;
        longjmp(jb, 1);
        cout << "Should not get here." << endl;
        return 1;
    } catch (...) {
        cout << "WTF" << endl;
        return 1;
    }
}

