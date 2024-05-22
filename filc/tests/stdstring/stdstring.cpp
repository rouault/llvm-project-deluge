#include <iostream>
#include <string>
#include <stdfil.h>

using namespace std;

int main(int argc, char** argv)
{
    string hello = "hello";
    string world = "world";

    string message = hello + " " + world + "!";

    ZASSERT(message == "hello world!");

    cout << "The message is: " << message << endl;

    return 0;
}

