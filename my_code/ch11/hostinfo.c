#include "csapp.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: ./hostinfo <website>");
    }

    struct addrinfo hints;
    struct addrinfo *result, *cur;
    char host[MAXLINE];
    char service[MAXLINE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[1], 443, &hints, &result);

    int flag = NI_NUMERICHOST;
    for (cur = result; cur; cur = cur->ai_next)
    {
        getnameinfo(cur->ai_addr, cur->ai_addrlen, host, MAXLINE, service, MAXLINE, flag);
        printf("host: %s\nservice: %s\n", host, service);
    }
    freeaddrinfo(result);
    return 0;
}