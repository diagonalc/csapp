#include "csapp.h"

void echo(int connfd)
{
    rio_t rio;
    rio_readinitb(&rio, connfd);
    char buf[MAXLINE];
    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        rio_writen(connfd, buf, n);
        printf("Received %d bytes data\n", n);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: %s <port>", argv[0]);
        exit(0);
    }

    int listenfd;
    char host[MAXLINE];
    char port[MAXLINE];
    socklen_t clilen;
    struct sockaddr_storage cliaddr;
    listenfd = open_listenfd(argv[1]);
    while (1)
    {
        socketlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, &cliaddr, &clilen);
        getnameinfo(&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to: %s:%s\n", host, port);
        echo(connfd);
        close(connfd);
    }
    exit(0);
}