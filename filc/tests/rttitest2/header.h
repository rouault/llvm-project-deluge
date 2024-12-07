#pragma once

struct Foo {
};

struct Bar : Foo {
    virtual ~Bar();
    virtual void thingy();
};

struct Baz : Bar {
    virtual ~Baz();
    void thingy() override;
};

struct OtherBaz : Bar {
    virtual ~OtherBaz();
    void thingy() override;
};

struct Blah : Baz {
    virtual ~Blah();
    void thingy() override;
};

struct OtherBlah : Baz {
    virtual ~OtherBlah();
    void thingy() override;
};

struct Stuff : Blah {
    virtual ~Stuff();
    void thingy() override;
};

struct OtherStuff : Blah {
    virtual ~OtherStuff();
    void thingy() override;
};

void doit(Bar*);
