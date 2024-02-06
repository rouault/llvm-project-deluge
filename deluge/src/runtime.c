#include <stdfil.h>

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
