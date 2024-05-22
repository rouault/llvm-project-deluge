#include <stdfil.h>

class foo {
public:
    foo(int field)
        : m_field(field)
    {
    }

    virtual ~foo()
    {
        zprintf("foo::~foo(%d)", m_field);
    }

    virtual void thingy()
    {
        zprintf("foo::thingy(%d)", m_field);
    }

    virtual void stuff()
    {
        zprintf("foo::stuff(%d)", m_field);
    }

private:
    int m_field;
};

class bar : public foo {
public:
    bar(int field, int otherField)
        : foo(field)
        , m_otherField(otherField)
    {
    }

    virtual ~bar()
    {
        zprintf("bar::~bar(%d)", m_otherField);
    }

    virtual void thingy()
    {
        zprintf("bar::thingy(%d)", m_otherField);
    }

    virtual void stuff()
    {
        zprintf("bar::stuff(%d)", m_otherField);
    }

private:
    int m_otherField;
};

static void test(const char* prefix, foo* f)
{
    zprintf("%s:thingy:", prefix);
    f->thingy();
    zprintf("\n");

    zprintf("%s:stuff:", prefix);
    f->stuff();
    zprintf("\n");

    zprintf("%s:delete:", prefix);
    delete f;
    zprintf("\n");
}

int main()
{
    test("foo", new foo(666));
    test("bar", new bar(42, 1410));
    return 0;
}

