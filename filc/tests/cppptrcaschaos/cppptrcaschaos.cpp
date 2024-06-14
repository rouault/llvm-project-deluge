#include <thread>
#include <atomic>
#include <iostream>
#include <stdlib.h>
#include <stdfil.h>

using namespace std;

constexpr unsigned nthreads = 10;
constexpr unsigned repeat = 30000;

struct foo {
    foo* next { nullptr };
    unsigned value { 0 };
};

int main(int argc, char** argv)
{
    vector<thread> threads;
    foo* head;

    for (unsigned i = nthreads; i--;) {
        threads.push_back(thread([&] () {
            for (unsigned i = repeat; i--;) {
                foo* f = new foo;
                f->value = i;
                for (;;) {
                    foo* h = head;
                    f->next = head;
                    foo* tmp = h;
                    if (reinterpret_cast<std::atomic<foo*>*>(&head)->compare_exchange_weak(tmp, f))
                        break;
                }
            }
        }));
    }

    for (thread& t : threads)
        t.join();

    unsigned histogram[repeat];
    memset(histogram, 0, sizeof(histogram));

    for (foo* f = head; f; f = f->next)
        histogram[f->value]++;

    for (unsigned i = repeat; i--;)
        ZASSERT(histogram[i] == nthreads);

    return 0;
}

