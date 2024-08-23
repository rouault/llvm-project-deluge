#define _GNU_SOURCE

#include <iostream>
#include <unordered_map>
#include <string>
#include <stdfil.h>
#include <sstream>
#include <inttypes.h>
#include <thread>
#include <mutex>
#include <unistd.h>

using namespace std;

constexpr size_t num_threads = 10;
constexpr size_t repeats = 50000;
constexpr size_t num_toplevel_keys = 2000;
constexpr size_t num_inner_keys = 2000;
constexpr size_t max_nodes_to_add = 20;
constexpr size_t max_nodes_to_remove = 20;

constexpr unsigned selection_range = 100;
constexpr unsigned add_nodes_probability = 60;
constexpr unsigned remove_top_level_probability = 10;
constexpr unsigned remove_inner_probability = 30;

unsigned fast_random_state = 42;

inline uint32_t xorshift32(uint32_t state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline unsigned get_fast_random(unsigned upper_bound)
{
    unsigned rand_value;

    if (!upper_bound)
        upper_bound = UINT32_MAX;

    fast_random_state = xorshift32(fast_random_state);
    rand_value = fast_random_state % upper_bound;

    return rand_value;
}

struct inner_map {
    mutex lock;
    unordered_map<string, string> map;
};

static mutex global_lock;
static unordered_map<string, inner_map*> hash_map;
static unsigned num_threads_done;

inline void add_nodes()
{
    string key;
    {
        ostringstream buf;
        buf << "key " << get_fast_random(num_toplevel_keys);
        key = buf.str();
    }
    inner_map* inner;
    {
        lock_guard guard(global_lock);
        auto result = hash_map.emplace(key, nullptr);
        if (result.second)
            result.first->second = new inner_map;
        inner = result.first->second;
    }
    for (size_t count = get_fast_random(max_nodes_to_add); count--;) {
        string subkey;
        string value;
        {
            ostringstream buf;
            buf << "subkey " << get_fast_random(num_inner_keys);
            subkey = buf.str();
        }
        {
            ostringstream buf;
            buf << "subvalue " << get_fast_random(0);
            value = buf.str();
        }
        lock_guard guard(inner->lock);
        inner->map[subkey] = value;
    }
}

inline void remove_top_level()
{
    string key;
    {
        ostringstream buf;
        buf << "key " << get_fast_random(num_toplevel_keys);
        key = buf.str();
    }
    lock_guard guard(global_lock);
    hash_map.erase(key);
}

inline void remove_inner()
{
    string key;
    {
        ostringstream buf;
        buf << "key " << get_fast_random(num_toplevel_keys);
        key = buf.str();
    }
    inner_map* inner = nullptr;
    {
        lock_guard guard(global_lock);
        auto iter = hash_map.find(key);
        if (iter != hash_map.end()) {
            inner = iter->second;
            ZASSERT(inner);
        }
    }
    if (!inner)
        return;
    for (size_t count = get_fast_random(max_nodes_to_remove); count--;) {
        string subkey;
        {
            ostringstream buf;
            buf << "subkey " << get_fast_random(num_inner_keys);
            subkey = buf.str();
        }
        lock_guard guard(inner->lock);
        inner->map.erase(subkey);
    }
}

inline void thread_main()
{
    for (size_t repeat = repeats; repeat--;) {
        unsigned selection = get_fast_random(selection_range);
        if (selection < add_nodes_probability) {
            add_nodes();
            continue;
        }
        selection -= add_nodes_probability;
        if (selection < remove_top_level_probability) {
            remove_top_level();
            continue;
        }
        selection -= remove_top_level_probability;
        ZASSERT(selection < remove_inner_probability);
        remove_inner();
    }
    lock_guard guard(global_lock);
    num_threads_done++;
}

int main(int argc, char** argv)
{
    vector<thread> threads;
    for (size_t index = num_threads; index--;)
        threads.push_back(thread(thread_main));

    while (num_threads_done < num_threads) {
        usleep(1000000);
        size_t num_toplevel_entries = 0;
        size_t num_inner_entries = 0;
        {
            lock_guard guard(global_lock);
            for (auto& pair : hash_map) {
                num_toplevel_entries++;
                num_inner_entries += pair.second->map.size();
            }
        }
        zprintf("num_toplevel_entries = %zu, num_inner_entries = %zu\n",
                num_toplevel_entries, num_inner_entries);
    }

    for (thread& t : threads)
        t.join();

    printf("Sukces!\n");
    return 0;
}

