#include <iostream>
#include "header.h"

using namespace std;

void doit(Bar* bar)
{
    bar->thingy();
    cout << ";Bar:";
    if (Bar* x = dynamic_cast<Bar*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";Baz:";
    if (Baz* x = dynamic_cast<Baz*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";OtherBaz:";
    if (OtherBaz* x = dynamic_cast<OtherBaz*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";Blah:";
    if (Blah* x = dynamic_cast<Blah*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";OtherBlah:";
    if (OtherBlah* x = dynamic_cast<OtherBlah*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";Stuff:";
    if (Stuff* x = dynamic_cast<Stuff*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << ";OtherStuff:";
    if (OtherStuff* x = dynamic_cast<OtherStuff*>(bar)) {
        cout << "yes,";
        x->thingy();
    } else
        cout << "no";
    cout << endl;
}
