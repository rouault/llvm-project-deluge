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

    virtual void dumpFromFoo() = 0;

private:
    int m_field;
};

class bar {
public:
    bar(int field)
        : m_otherOtherField(field)
    {
    }

    virtual ~bar()
    {
        zprintf("bar::~bar(%d)", m_otherOtherField);
    }

    virtual void blah()
    {
        zprintf("bar::blah(%d)", m_otherOtherField);
    }

    virtual void bleh()
    {
        zprintf("bar::bleh(%d)", m_otherOtherField);
    }

    virtual void dumpFromBar() = 0;

private:
    int m_otherOtherField;
};

class baz : public virtual foo, public virtual bar {
public:
    baz(int field, int otherOtherField, int otherField)
        : foo(field)
        , bar(otherOtherField)
        , m_otherField(otherField)
    {
        zprintf("baz this = %P\n", this);
        zprintf("baz vtable = %P\n", *(void**)this);
    }

    virtual ~baz()
    {
        zprintf("baz::~baz(%d)", m_otherField);
    }

    virtual void thingy()
    {
        zprintf("baz::thingy(%d)", m_otherField);
    }

    virtual void stuff()
    {
        zprintf("baz::stuff(%d)", m_otherField);
    }

    virtual void blah()
    {
        zprintf("baz::blah(%d)", m_otherField);
    }

    virtual void bleh()
    {
        zprintf("baz::bleh(%d)", m_otherField);
    }

    virtual void dumpFromFoo()
    {
        zprintf("dumpFromFoo: baz this = %P\n", this);
        zprintf("dumpFromFoo: baz vtable = %P\n", *(void**)this);
    }

    virtual void dumpFromBar()
    {
        zprintf("dumpFromBar: baz this = %P\n", this);
        zprintf("dumpFromBar: baz vtable = %P\n", *(void**)this);
    }

private:
    int m_otherField;
};

static void testFoo(foo* f)
{
    zprintf("testFoo: f = %P\n", f);
    
    zprintf("foo:thingy:");
    f->thingy();
    zprintf("\n");

    zprintf("foo:stuff:");
    f->stuff();
    zprintf("\n");

    f->dumpFromFoo();

    zprintf("foo:delete:");
    delete f;
    zprintf("\n");
}

static void testBar(bar* b)
{
    zprintf("testBar: b = %P\n", b);
    
    zprintf("bar:blah:");
    b->blah();
    zprintf("\n");

    zprintf("bar:bleh:");
    b->bleh();
    zprintf("\n");

    b->dumpFromBar();

    zprintf("bar:delete:");
    delete b;
    zprintf("\n");
}

int main()
{
    testFoo(new baz(42, 666, 1410));
    testBar(new baz(42, 666, 1410));
    return 0;
}

