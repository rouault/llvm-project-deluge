#include <stdfil.h>

int main(void) {
    for (unsigned i = 666; i--;) {
        int **ptrs[100];
        for (unsigned j = 100; j--;) {
            ptrs[j] = zalloc(int*, 1);
            *ptrs[j] = zalloc(int, 1);
            **ptrs[j] = j;
        }
        for (unsigned j = 100; j--;) {
            if (**ptrs[j] != j)
                zprint("ERROR\n");
            zfree(*ptrs[j]);
            zfree(ptrs[j]);
        }
    }
    return 0;
}
