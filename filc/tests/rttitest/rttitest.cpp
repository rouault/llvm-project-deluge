#include <iostream>
#include <string>

using namespace std;

class Foo {
public:
    Foo(int x)
        : m_x(x)
    {
    }
    
    virtual ~Foo() = default;

    virtual void thingy(const string& prefix)
    {
        cout << prefix << "Foo::thingy" << m_x << endl;
    }

protected:
    int m_x;
};

class Bar : public Foo {
public:
    Bar(int x, int y)
        : Foo(x)
        , m_y(y)
    {
    }

    void thingy(const string& prefix) override
    {
        cout << prefix << "Bar::thingy" << m_x << ", " << m_y << endl;
    }

    void stuff(const string& prefix)
    {
        cout << prefix << "Bar::stuff" << m_x << ", " << m_y << endl;
    }

private:
    int m_y;
};

void blah(const string& prefix, Foo* f)
{
    f->thingy(prefix);
    if (Bar* b = dynamic_cast<Bar*>(f))
        b->stuff(prefix);
    else
        cout << prefix << "castfail" << endl;
    cout << prefix << "type: " << typeid(*f).name() << endl;
}

int main(int argc, char** argv)
{
    Foo* f = new Foo(42);
    Bar* b = new Bar(666, 1410);

    blah("Foo", f);
    blah("Bar", b);

    return 0;
}

