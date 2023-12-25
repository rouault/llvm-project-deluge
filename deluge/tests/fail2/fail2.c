void print(const char* string);
void fail(void);
int* opaque(int*);

int main(void)
{
    int x[100];
    int* ptr = (int*)((char*)opaque(&x) + 99 * 4 + 2);
    *ptr = 42;
    if (*ptr == 42)
        print("YAY\n");
    else
        fail();
    return 0;
}

