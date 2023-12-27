#include <stdfil.h>

#define BUFFER_IN_LOOP(iterations, elements) do { \
        for (unsigned i = iterations; i--;) { \
            int **ptrs[elements]; \
            for (unsigned j = elements; j--;) { \
                ptrs[j] = zalloc(int*, 1); \
                *ptrs[j] = zalloc(int, 1); \
                **ptrs[j] = j; \
            } \
            for (unsigned j = elements; j--;) { \
                if (**ptrs[j] != j) \
                    zprint("ERROR\n"); \
                zfree(*ptrs[j]); \
                zfree(ptrs[j]); \
            } \
        } \
    } while (0)

int main(void) {
    BUFFER_IN_LOOP(10, 20);
    BUFFER_IN_LOOP(20, 10);
    BUFFER_IN_LOOP(10, 200);
    BUFFER_IN_LOOP(20, 100);
    BUFFER_IN_LOOP(100, 30);
    BUFFER_IN_LOOP(200, 40);
    BUFFER_IN_LOOP(666, 666);
    return 0;
}
