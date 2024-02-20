#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdfil.h>

int main()
{
    struct addrinfo hints;
    struct addrinfo* result = NULL;
    struct addrinfo* cur;
    
    bzero(&hints,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    ZASSERT(!getaddrinfo("localhost", "8000", &hints, &result));
    ZASSERT(result);
    
    for (cur = result; cur != NULL; cur = cur->ai_next) {
        ZASSERT(cur->ai_addrlen == sizeof(struct sockaddr_in));
        struct sockaddr_in* addr = (struct sockaddr_in*)cur->ai_addr;
        printf("(1) Address:Port = %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    }

    freeaddrinfo(result);
    result = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    ZASSERT(!getaddrinfo("127.0.0.1", 0, &hints, &result));
    ZASSERT(result);

    for (cur = result; cur != NULL; cur = cur->ai_next) {
        ZASSERT(cur->ai_addrlen == sizeof(struct sockaddr_in));
        struct sockaddr_in* addr = (struct sockaddr_in*)cur->ai_addr;
        printf("(2) Address:Port = %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    }

    return 0;
}

