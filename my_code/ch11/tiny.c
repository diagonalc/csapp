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
        if (!(S_ISREG(statbuf->st_mode)) || !(S_IRUSR & statbuf->st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Required file cannot be read");
            return;
        }
        serve_static(fd, filename, statbuf->st_size);
    }
    else
    {
        if (!(S_ISREG(statbuf->st_mode)) || !(S_IRUSR & statbuf->st_mode))
        {

            clienterror(fd, filename, "403", "Forbidden", "Required CGI program cannot be run");
            return;
        }
        serve_dynamic(fd, filename, cgiarg);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];
    // sprintf with a offset "len", as strcat does not accept placeholders like "%s" while something like "sprintf(buf, "%s %s", buf, uri)" is undefined
    int len = 0;
    len += sprintf(body + len, "<html><title>Tiny Error</title>");
    len += sprintf(body + len, "<body bgcolor=\"ffffff\">\r\n");
    len += sprintf(body + len, "%s: %s\r\n", errnum, shortmsg);
    len += sprintf(body + len, "<p>%s: %s\r\n", longmsg, cause);
    len += sprintf(body + len, "<hr><em>The Tiny Web server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    // although the buffer zone won't be flushed, sprintf will attach a \0 at the end and strlen(buf) will only count to \0
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    rio_readlineb(rp, buf, MAXLINE);

    while (strcmp(buf, "\r\n")) // looking for a return and a empty line (marks as the ending of request headers)
    {
        printf("%s", buf);
        rio_readlineb(rp, buf, MAXLINE); // just comsuming the request headers as we're not gonna read them anyway
    }
    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin"))
    {                          // static content, as all dynamic content should be inside the folder "cgi-bin"
        strcpy(cgiargs, "");   // erasing cgiargs, as client requests a static content (but there's nothing inside anyway)
        strcpy(filename, "."); // same for filename
        strcat(filename, uri);
        if (filename[strlen(filename) - 1] == '/')
            strcat(filename, "home.html");
        return 1; // return 1 if its static
    }
    else
    {
        ptr = strchr(uri, '?'); // return the pointer to the required char
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1); // the next char after '?'
            *ptr = '\0';              // uri: from "/cgi-bin/adder?150&120" to "/cgi-bin/adder \0 150&120", cutting the uri string. the arguments part "150&120" will not be read again due to the '\0'
        }
        else
        {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0; // return 0 it its dynamic
    }
}

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    get_filetype(filename, filetype);
    int ofs = 0;
    ofs += sprintf(buf + ofs, "HTTP/1.0 200 OK\r\n");
    ofs += sprintf(buf + ofs, "Server: Tiny Web Server\r\n");
    ofs += sprintf(buf + ofs, "Connection: close\r\n");
    ofs += sprintf(buf + ofs, "Content-length: %d\r\n", filesize);
    ofs += sprintf(buf + ofs, "Content-type: %s\r\n\r\n", filetype); // two consecutive \r\n marks the ending of the headers
    rio_writen(fd, buf, strlen(buf));

    printf("Response headers:\n");
    printf("%s", buf);

    srcfd = open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}


void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE];
    char *emptylist[] = {NULL};
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (fork() == 0)
    {
        // due to CGI Protocol, arguments should be passed to the cgi program through environment variables
        setenv("QUERY_STRING", cgiargs, 1); // saving the arguments into the environment variables
        dup2(fd, STDOUT_FILENO);            // so that the std output within the program will be redirect to the client fd
        execve(filename, emptylist, environ);
    }
    wait(NULL);
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
