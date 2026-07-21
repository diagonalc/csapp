#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

void doit(int fd)
{
    rio_t rio;
    int is_static;
    struct stat *statbuf;
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiarg[MAXLINE];

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s\n", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not Implemented", "Tiny web server has not implement this method yet");
        return;
    }
    read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiarg);
    if (stat(filename, statbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found", "Required file cannot be found");
        return;
    }
    if (is_static)
    {
        if (!(S_ISREG(statbuf->st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Required file cannot be read");
            return;
        }
        serve_static(fd, filename, statbuf->st_size);
    }
    else
    {
        if (!(S_ISREG(statbuf->st_mode)) || !(S_IRUSR & sbuf.st_mode))
        {

            clienterror(fd, filename, "403", "Forbidden", "Required CGI program cannot be run");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}


void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg){
    char buf[MAXLINE], body[MAXLINE];
    sprintf()
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: ./tiny <port>");
        exit(0);
    }
    int listenfd, connfd;
    char host[MAXLINE];
    char port[MAXLINE];
    struct sockaddr_storage cliaddr;
    socklen_t clilen;

    listenfd = open_listenfd(atoi(argv[1]));
    while (1)
    {
        clilen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
        getnameinfo((SA *)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to: %s:%s\n", host, port);
        doit(connfd);
        close(connfd);
    }

    exit(0);
}
