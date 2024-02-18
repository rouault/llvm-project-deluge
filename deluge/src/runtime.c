#include <stdfil.h>

_Bool zinbounds(void* ptr)
{
    return ptr >= zgetlower(ptr) && ptr < zgetupper(ptr);
}

void* zalloc_clone(void* obj)
{
    /* FIXME: This doesn't work right for flexes, but probably only because zalloc_with_type doesn't
       work for flexes. */
    void* result = zalloc_with_type(zgettype(obj), (char*)zgetupper(obj) - (char*)zgetlower(obj));
    if (!result)
        return 0;
    return (char*)result + ((char*)obj - (char*)zgetlower(obj));
}

typedef struct {
    const int* address;
    int expected_value;
} compare_and_park_data;

static _Bool compare_and_park_condition(void* arg)
{
    compare_and_park_data* data = (compare_and_park_data*)arg;
    return *data->address == data->expected_value;
}

static void empty_before_sleep(void* arg)
{
    (void)arg;
}

_Bool zcompare_and_park(const int* address, int expected_value, double absolute_timeout_in_milliseconds)
{
    compare_and_park_data data;
    data.address = (const int*)((const char*)address - ((unsigned long)address % sizeof(int)));
    data.expected_value = expected_value;
    return zpark_if(address, compare_and_park_condition, empty_before_sleep, &data,
                    absolute_timeout_in_milliseconds);
}

unsigned zthread_self_id(void)
{
    return zthread_get_id(zthread_self());
}

void* zthread_self_cookie(void)
{
    return zthread_get_cookie(zthread_self());
}
