#include "header.h"

char* foo;
extern char* bar __attribute__((__weak__, __alias__("foo")));
