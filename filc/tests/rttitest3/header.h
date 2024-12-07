#pragma once

struct Foo {
};

struct Bar : Foo {
    virtual ~Bar();
    virtual void thingy();
};

struct Mixin {
    Mixin(): x(666) { }
    virtual ~Mixin();
    int x;
};

struct Baz : Bar {
    virtual ~Baz();
    void thingy() override;
};

struct OtherBaz : Bar, Mixin {
    virtual ~OtherBaz();
    void thingy() override;
};

struct Blah : Baz {
    virtual ~Blah();
    void thingy() override;
};

struct OtherBlah : Baz, Mixin {
    virtual ~OtherBlah();
    void thingy() override;
};

struct Stuff : Blah {
    virtual ~Stuff();
    void thingy() override;
};

struct OtherStuff : Blah, Mixin {
    virtual ~OtherStuff();
    void thingy() override;
};

void doit(Bar*);
