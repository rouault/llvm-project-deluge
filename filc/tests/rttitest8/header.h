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

struct OtherBaz final : Bar {
    virtual ~OtherBaz();
    void thingy() override;
};

struct Blah : Baz, Mixin {
    virtual ~Blah();
    void thingy() override;
};

struct OtherBlah final : Baz {
    virtual ~OtherBlah();
    void thingy() override;
};

struct Stuff final : Blah {
    virtual ~Stuff();
    void thingy() override;
};

struct OtherStuff final : Blah {
    virtual ~OtherStuff();
    void thingy() override;
};

void doit(Bar*);
