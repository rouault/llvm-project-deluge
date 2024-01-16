#include <stdio.h>
#include <stdlib.h>

int values[] = { 88, 56, 100, 2, 25 };

int cmpfunc(const void * a, const void * b) {
    return *(int*)a - *(int*)b;
}

int main() {
    int n;

    printf("Before sorting the list is:");
    for (n = 0; n < 5; n++)
        printf(" %d", values[n]);
    printf("\n");
    
    qsort(values, 5, sizeof(int), cmpfunc);

    printf("After sorting the list is:");
    for(n = 0; n < 5; n++)
        printf(" %d", values[n]);
    printf("\n");
    
    return 0;
}
