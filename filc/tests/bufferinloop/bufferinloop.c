#include <stdfil.h>

int main(void) {
    for (unsigned i = 666; i--;) {
        int **ptrs[100];
        for (unsigned j = 100; j--;) {
            ptrs[j] = zalloc(sizeof(int*));
            *ptrs[j] = zalloc(sizeof(int));
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
