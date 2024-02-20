#include <stdfil.h>
#include "utils.h"

#define I (0.0f+1.0fi)
#define complex _Complex

static double real(double complex x)
{
    return (double)x;
}

static double imag(double complex x)
{
    union {
        double complex c;
        double d[2];
    } u;
    u.c = x;
    return u.d[1];
}

#define PRINT(exp) do { \
        double complex v = (exp); \
        zprintf("%s = %lf + %lfi\n", #exp, real(v), imag(v)); \
    } while (0)

int main(int argc, char** argv)
{
    double complex x;
    double complex y;
    *(double complex*)opaque(&x) = 5 + 7 * I;
    *(double complex*)opaque(&y) = -9 + 2 * I;

    PRINT(x);
    PRINT(y);
    PRINT(x + y);
    PRINT(x * y);
    PRINT(x - y);
    PRINT(x / y);

    return 0;
}

