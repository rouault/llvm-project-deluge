void print(const char* string);
void fail(void);
int* opaque(int*);

int main(void)
{
    int x;
    int* ptr = opaque(&x) + 1;
    *ptr = 42;
    if (*ptr == 42)
        print("YAY\n");
    else
        fail();
    return 0;
}

