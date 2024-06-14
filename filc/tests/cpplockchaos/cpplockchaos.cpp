#include <thread>
#include <iostream>
#include <mutex>
#include <vector>

using namespace std;

constexpr unsigned nthreads = 10;
constexpr unsigned repeat = 10000;

int main(int argc, char** argv)
{
    mutex lock;
    unsigned count = 0;

    vector<thread> threads;
    for (unsigned i = nthreads; i--;) {
        threads.push_back(thread([&] () {
            for (unsigned i = repeat; i--;) {
                lock_guard guard(lock);
                count++;
            }
        }));
    }

    for (thread& t : threads)
        t.join();

    cout << "count = " << count << endl;
    
    return 0;
}

