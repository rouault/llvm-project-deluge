struct S { S() {} };
void test5()
{
    thread_local S s;
}

int main()
{
    test5();
    return 0;
}
