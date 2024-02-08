#include <stdfil.h>

int main()
{
    struct {
        int x;
        int y;
        void* a;
        void* b;
        int m;
        int n;
    } s;

    int* x = zrestrict(&s.x, int, 1);
    int* y = zrestrict(&s.y, int, 1);
    void** a = zrestrict(&s.a, void*, 1);
    void** b = zrestrict(&s.b, void*, 1);
    int* m = zrestrict(&s.m, int, 1);
    int* n = zrestrict(&s.n, int, 1);

    ZASSERT(zgetlower(x) == x);
    ZASSERT(zgetupper(x) == x + 1);
    ZASSERT(zgettype(x) == ztypeof(int));
    ZASSERT(zgetlower(y) == y);
    ZASSERT(zgetupper(y) == y + 1);
    ZASSERT(zgettype(y) == ztypeof(int));
    ZASSERT(zgetlower(a) == a);
    ZASSERT(zgetupper(a) == a + 1);
    ZASSERT(zgettype(a) == ztypeof(void*));
    ZASSERT(zgetlower(b) == b);
    ZASSERT(zgetupper(b) == b + 1);
    ZASSERT(zgettype(b) == ztypeof(void*));
    ZASSERT(zgetlower(m) == m);
    ZASSERT(zgetupper(m) == m + 1);
    ZASSERT(zgettype(m) == ztypeof(int));
    ZASSERT(zgetlower(n) == n);
    ZASSERT(zgetupper(n) == n + 1);
    ZASSERT(zgettype(n) == ztypeof(int));

    int* p = zborkedptr(x, y);
    ZASSERT(p == y);
    ZASSERT(zgetlower(p) == y);
    ZASSERT(zgetupper(p) == y + 1);
    ZASSERT(zgettype(p) == ztypeof(int));

    p = zborkedptr(x, (char*)y + 1);
    ZASSERT(p == (char*)y + 1);
    zprintf("x = %P, y = %P, p = %P\n", x, y, p);
    ZASSERT(zgetlower(p) == (char*)y + 1);
    ZASSERT(zgetupper(p) == y + 1);
    ZASSERT(zgettype(p) == ztypeof(int));

    void* q = zborkedptr(a, b);
    ZASSERT(q == b);
    ZASSERT(zgetlower(q) == b);
    ZASSERT(zgetupper(q) == b + 1);
    ZASSERT(zgettype(q) == ztypeof(void*));

    q = zborkedptr(a, (char*)b + 1);
    ZASSERT(q == (char*)b + 1);
    ZASSERT(zgetlower(q) == b);
    ZASSERT(zgetupper(q) == b + 1);
    ZASSERT(zgettype(q) == ztypeof(void*));

    q = zborkedptr((char*)(a + 1) + 1, (char*)b + 1);
    ZASSERT(q == (char*)b + 1);
    ZASSERT(zgetlower(q) == b);
    ZASSERT(zgetupper(q) == b + 1);
    ZASSERT(zgettype(q) == ztypeof(void*));

    p = zborkedptr(x, n);
    ZASSERT(p == n);
    ZASSERT(zgetlower(p) == n);
    ZASSERT(zgetupper(p) == n + 1);
    ZASSERT(zgettype(p) == ztypeof(int));

    int* above_x = x + 3;
    int* below_n = n - (n - x) + 3;
    ZASSERT(above_x == below_n);
    ZASSERT(zgetlower(above_x) == x);
    ZASSERT(zgetupper(above_x) == x + 1);
    ZASSERT(zgettype(above_x) == ztypeof(int));
    ZASSERT(zgetlower(below_n) == n);
    ZASSERT(zgetupper(below_n) == n + 1);
    ZASSERT(zgettype(below_n) == ztypeof(int));

    p = zborkedptr(above_x, below_n);
    ZASSERT(p == above_x);
    ZASSERT(p == below_n);
    ZASSERT(zgetlower(p) == x);
    ZASSERT(zgetupper(p) == x + 1);
    ZASSERT(zgettype(p) == ztypeof(int));

    above_x = x + 4;
    below_n = n - 3;
    ZASSERT(above_x != below_n);
    ZASSERT(zgetlower(above_x) == x);
    ZASSERT(zgetupper(above_x) == x + 1);
    ZASSERT(zgettype(above_x) == ztypeof(int));
    ZASSERT(zgetlower(below_n) == n);
    ZASSERT(zgetupper(below_n) == n + 1);
    ZASSERT(zgettype(below_n) == ztypeof(int));

    p = zborkedptr(above_x, below_n);
    ZASSERT(p == below_n);
    ZASSERT(zgetlower(p) == x);
    ZASSERT(zgetupper(p) == x + 1);
    ZASSERT(zgettype(p) == ztypeof(int));

    /* FIXME: write flex tests! */

    return 0;
}

