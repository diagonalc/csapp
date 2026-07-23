#include "csapp.h"

void echo(int connfd)
{
    rio_t rio;
    int n;
    rio_readinitb(&rio, connfd);
    char buf[MAXLINE];
    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        rio_writen(connfd, buf, n);
        printf("Received %d bytes data\n", n);
        fputs(buf, stdout);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: ./server <port>");
        exit(0);
    }

    int listenfd, connfd;
    char host[MAXLINE];
    char port[MAXLINE];
    socklen_t clilen;
    struct sockaddr_storage cliaddr;
    listenfd = open_listenfd(atoi(argv[1]));
    while (1)
    {
    	
        clilen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA*)&cliaddr, &clilen);
        getnameinfo((SA*)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to: %s:%s\n", host, port);
        echo(connfd);
        close(connfd);
    }
    exit(0);
}
