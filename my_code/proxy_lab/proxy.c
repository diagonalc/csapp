#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// User input example: GET http://192.168.1.2:8080/cgi-bin/adder?1&1 HTTP/1.1
// method: GET
// host: 192.168.1.2
// port: 8080
// path: /cgi-bin/adder?1&1

void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, char *short_msg, char *long_msg);
void parse_uri(char *uri, char *host, char *port, char *path);

void doit(int clifd)
{
    rio_t rio_cli, rio_server;
    int serverfd;
    char buf[MAXBUF], header[65536];
    char reply[65536];
    char method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char host[MAXBUF], port[MAXBUF], path[MAXBUF];

    rio_readinitb(&rio_cli, clifd);
    rio_readlineb(&rio_cli, buf, MAXBUF);
    printf("Request Headers: %s\n", buf);
    if (sscanf(buf, "%s %s %s", method, uri, version) < 3)
    {
        clienterror(clifd, buf, "400", "Bad Request", "Proxy server received a malformed request");
        return;
    }
    if (strcasecmp(method, "GET"))
    {
        clienterror(clifd, method, "501", "Not Implemented", "Proxy server has not implemented this method yet");
        return;
    }
    parse_uri(uri, host, port, path);
    serverfd = open_clientfd(host, port);
    rio_readinitb(&rio_server, serverfd);
    sprintf(header, "%s %s HTTP/1.0\r\n\r\n", method, path);
    rio_writen(serverfd, header, strlen(header));
    ssize_t n;
    while ((n = rio_readnb(&rio_server, reply, 65535)) > 0)
    {
        rio_writen(clifd, reply, n);
    }
    close(serverfd);
}

void parse_uri(char *uri, char *host, char *port, char *path)
{
    char *first_colon, *second_colon, *slash_aft_port;
    first_colon = strchr(uri, ':');
    first_colon += 3;
    // 192.168.1.2:8080/cgi-bin/adder?1&1
    second_colon = strchr(first_colon, ':');
    *second_colon = '\0';
    // 192.168.1.2 \0 8080/cgi-bin/adder?1&1
    strcpy(host, first_colon);
    slash_aft_port = strchr(second_colon + 1, '/');
    *slash_aft_port = '\0';
    // 192.168.1.2 \0 8080 \0 cgi-bin/adder?1&1
    strcpy(port, second_colon + 1);
    strcpy(path, "/");
    strcat(path, slash_aft_port + 1);
}

void clienterror(int fd, char *cause, char *errnum, char *short_msg, char *long_msg)
{
    char buf[MAXBUF], body[MAXBUF];
    int ofs = 0;
    ofs += sprintf(body + ofs, "<html><title>Proxy Error</title>");
    ofs += sprintf(body + ofs, "<body bgcolor=\"ffffff\">\r\n");
    ofs += sprintf(body + ofs, "%s: %s\r\n", errnum, short_msg);
    ofs += sprintf(body + ofs, "<p>%s: %s\r\n", long_msg, cause);
    ofs += sprintf(body + ofs, "<hr><em>The Proxy server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, short_msg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        unix_error("format: ./proxy <port>");
        exit(0);
    }
    int listenfd, connfd;
    char host[MAXLINE];
    char port[MAXLINE];
    struct sockaddr_storage cliaddr;
    socklen_t clilen;

    listenfd = open_listenfd(argv[1]);
    while (1)
    {
        clilen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);
        Getnameinfo((SA *)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to client: %s:%s\n", host, port);
        doit(connfd);
        close(connfd);
    }

    printf("%s", user_agent_hdr);
    return 0;
}
