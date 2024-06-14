#include <thread>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
    string str;
    
    thread t([&] () {
        str = "hello std::thread!";
    });

    t.join();

    cout << str << endl;

    return 0;
}

