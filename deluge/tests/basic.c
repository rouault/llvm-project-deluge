void print(const char* string);
void fail(void);

int main(void)
{
    int x;
    x = 42;
    if (x == 42)
        print("YAY\n");
    else
        fail();
    return 0;
}

