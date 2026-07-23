#include "csapp.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        unix_error("format: ./client <host> <port>");
        exit(0);
    }

    char *host = argv[1];
    char *port = argv[2];
    int clifd = open_clientfd(host, atoi(port));

    rio_t rio;
    rio_readinitb(&rio, clifd);
	char buf[MAXLINE];
    while (fgets(buf, MAXLINE, stdin) != NULL)
    {
        rio_writen(clifd, buf, strlen(buf));
        rio_readlineb(&rio, buf, MAXLINE);
        fputs(buf, stdout);
    }
    close(clifd);
    exit(0);
}
