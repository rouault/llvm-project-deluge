#include <stdfil.h>
#include <string.h>
#include <stdio.h>

int main()
{
    char* buf = strdup("hello");
    char* buf2 = zrealloc(buf, char, 100);
    ZASSERT(!strcmp(buf2, "hello"));
    const char* str = "gjdhas;ofjdkagjdsai;gjdfpakgjdfkslajgdkls;ajhgdkfls;hgfkldzhgfjlk;dahglj;fdsag";
    strcpy(buf2, str);
    ZASSERT(!strcmp(buf2, str));
    zprintf("zrestricted ptr = %P\n", zrestrict(buf2, char, 3));
    char* buf3 = zrealloc(zrestrict(buf2, char, 3), char, 15);
    printf("buf3 = %s\n", buf3);
    ZASSERT(!strcmp(buf3, "gjd"));
    strcpy(buf3, "Hello, world!\n");
    printf("%s", buf3);
    return 0;
}

