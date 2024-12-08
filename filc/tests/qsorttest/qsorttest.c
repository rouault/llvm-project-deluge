#include <stdlib.h>
#include <stdfil.h>
#include <string.h>

struct foo {
    char* string;
};

static const char* strings[] = {
    "foo",
    "bar",
    "baz",
    "tak",
    "nie",
    "thingy",
    "stuff",
    "blah",
    "witaj",
    "bywaj"
};

static unsigned hash32(unsigned a)
{
    a = a ^ (a >> 4);
    a = (a ^ 0xdeadbeef) + (a << 5);
    a = a ^ (a >> 11);
    return a;
}

static int compare(const void* a, const void* b)
{
    const struct foo* x = (const struct foo*)a;
    const struct foo* y = (const struct foo*)b;
    return strcmp(x->string, y->string);
}

int main()
{
    unsigned num_strings = sizeof(strings) / sizeof(const char*);
    ZASSERT(num_strings > 1);
    unsigned length = 10000;
    struct foo* array = malloc(sizeof(struct foo) * length);
    unsigned index;
    for (index = length; index--;)
        array[index].string = strdup(strings[hash32(index) % num_strings]);
    qsort(array, length, sizeof(struct foo), compare);
    for (index = length - 1; index--;)
        ZASSERT(strcmp(array[index].string, array[index + 1].string) <= 0);
    return 0;
}

