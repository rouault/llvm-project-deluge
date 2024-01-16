#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* values[] = { "88", "56", "100", "2", "25" };

int cmpfunc(const void * a, const void * b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int main() {
    int n;

    printf("Before sorting the list is:");
    for (n = 0; n < 5; n++)
        printf(" %s", values[n]);
    printf("\n");
    
    qsort(values, 5, sizeof(const char*), cmpfunc);

    printf("After sorting the list is:");
    for(n = 0; n < 5; n++)
        printf(" %s", values[n]);
    printf("\n");
    
    return 0;
}
