#include <stdfil.h>

static unsigned long foo(char** cursor, unsigned long multiplier)
{
    unsigned long result = 0;
    for (; **cursor; (*cursor)++) {
        if (**cursor == '(') {
            (*cursor)++;
            result += foo(cursor, multiplier + 1);
        } else if (**cursor == ')')
            return result;
        else {
            ZASSERT(**cursor >= '0' && **cursor <= '9');
            result += multiplier * (**cursor - '0');
        }
    }
    return result;
}

static unsigned long bar(char* str)
{
    return foo(&str, 1);
}

int main()
{
    ZASSERT(bar("123") == 6);
    ZASSERT(bar("1(1)1") == 4);
    ZASSERT(bar("1((((1))))1") == 7);
    ZASSERT(bar("(())") == 0);

    zprintf("Got this far.\n");

    unsigned long depth = 1000000;
    char* buf = zgc_alloc(depth * 2 + 1);
    unsigned long index;
    for (index = 0; index < depth; ++index) {
        buf[index] = '(';
        buf[index + depth] = ')';
    }
    ZASSERT(bar(buf) == 0);
    ZASSERT(!"Should not get here");
    
    return 0;
}

