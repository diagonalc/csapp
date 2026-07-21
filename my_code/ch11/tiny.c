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

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];
    //sprintf with a offset "len", as strcat does not accept placeholders like "%s" while something like "sprintf(buf, "%s %s", buf, uri)" is undefined
    int len = 0;
    len += sprintf(body + len, "<html><title>Tiny Error</title>");
    len += sprintf(body + len, "<body bgcolor=\"ffffff\">\r\n");
    len += sprintf(body + len, "%s: %s\r\n", errnum, shortmsg);
    len += sprintf(body + len, "<p>%s: %s\r\n", longmsg, cause);
    len += sprintf(body + len, "<hr><em>The Tiny Web server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    //although the buffer zone won't be flushed, sprintf will attach a \0 at the end and strlen(buf) will only count to \0
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp){
    char buf[MAXLINE];

    rio_readlineb(rio, buf, MAXLINE)
    
    while(strcmp(buf, "\r\n")){
        printf("%s", buf);
        rio_readlineb(rio, buf, MAXLINE);
    }
    return;
}

void parse_uri(char* uri, char *filename, char* cgiargs){
    char *ptr;
    if(!strstr(uri, "cgi-bin")){    //static content, as all dynamic content should be inside the folder "cgi-bin"
        strcpy(cgiargs, "");        //erase cgiargs, as client requests a static content
        strcpy(filename, ".");
        strcpy
    }
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
