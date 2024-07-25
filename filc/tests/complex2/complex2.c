#include <stdfil.h>
#include "utils.h"

#define I (0.0f+1.0fi)
#define complex _Complex

static float real(float complex x)
{
    return (float)x;
}

static float imag(float complex x)
{
    union {
        float complex c;
        float d[2];
    } u;
    u.c = x;
    return u.d[1];
}

#define PRINT(exp) do { \
        float complex v = (exp); \
        zprintf("%s = %lf + %lfi\n", #exp, real(v), imag(v)); \
    } while (0)

int main(int argc, char** argv)
{
    float complex x;
    float complex y;
    *(float complex*)opaque(&x) = 5 + 7 * I;
    *(float complex*)opaque(&y) = -1 + -1 * I;

    PRINT(x);
    PRINT(y);
    PRINT(x + y);
    PRINT(x * y);
    PRINT(x - y);
    PRINT(x / y);

    return 0;
}

