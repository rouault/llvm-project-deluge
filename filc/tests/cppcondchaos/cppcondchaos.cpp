#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

constexpr unsigned nthreads = 10;
constexpr unsigned repeat = 10000;

using namespace std;

int main(int argc, char** argv)
{
    mutex lock;
    condition_variable cond;
    bool is_available = true;
    unsigned count = 0;

    vector<thread> threads;
    
    for (unsigned i = nthreads; i--;) {
        threads.push_back(thread([&] () {
            for (unsigned i = repeat; i--;) {
                {
                    unique_lock guard(lock);
                    cond.wait(guard, [&] () { return is_available; });
                    is_available = false;
                }
                count++;
                {
                    lock_guard guard(lock);
                    is_available = true;
                    cond.notify_all();
                }
            }
        }));
    }

    for (thread& t : threads)
        t.join();

    cout << "count = " << count << endl;
    
    return 0;
}

