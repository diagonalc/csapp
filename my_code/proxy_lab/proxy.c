#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_THREAD 20
#define MAX_CACHE_LINES 100

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// User input example: GET http://192.168.1.2:8080/cgi-bin/adder?1&1 HTTP/1.1
// method: GET
// host: 192.168.1.2
// port: 8080
// path: /cgi-bin/adder?1&1

typedef struct cache_line cache_line_t;
typedef struct cache cache_t;
void *doit(void *fd);
void clienterror(int fd, char *cause, char *errnum, char *short_msg, char *long_msg);
void parse_uri(char *uri, char *host, char *port, char *path);
void read_requesthdrs(rio_t *rio);
void cache_insert(char *url, char *buf);

struct cache
{
    cache_line_t lines[MAX_CACHE_LINES];
    int total_size;
    unsigned int cnt;
    pthread_mutex_t lock;
};
cache_t cache;

struct cache_line
{
    unsigned int lru_cnt;
    char *data;
    char *url;
    int size;
};

void *doit(void *clifd_ptr)
{
    int clifd = *((int *)clifd_ptr);
    free(clifd_ptr);
    pthread_detach(pthread_self());

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
        close(clifd);
        return NULL;
    }
    if (strcasecmp(method, "GET"))
    {
        clienterror(clifd, method, "501", "Not Implemented", "Proxy server has not implemented this method yet");
        close(clifd);
        return NULL;
    }
    read_requesthdrs(&rio_cli);
    parse_uri(uri, host, port, path);
    serverfd = open_clientfd(host, port);
    if (serverfd == -1)
    {
        clienterror(clifd, host, "502", "Bad Gateway", "Proxy server cannot connect to the required server");
        close(clifd);
        return NULL;
    }
    rio_readinitb(&rio_server, serverfd);
    sprintf(header, "%s %s HTTP/1.0\r\n", method, path);
    sprintf(header + strlen(header), "Host: %s\r\n", host);
    sprintf(header + strlen(header), "%s", user_agent_hdr);
    sprintf(header + strlen(header), "Connection: close\r\n");
    sprintf(header + strlen(header), "Proxy-Connection: close\r\n\r\n");
    rio_writen(serverfd, header, strlen(header));
    ssize_t n;
    while ((n = rio_readnb(&rio_server, reply, 65535)) > 0)
    {
        rio_writen(clifd, reply, n);
    }
    close(serverfd);
    close(clifd);
    return NULL;
}

void read_requesthdrs(rio_t *rio)
{
    char buf[MAXBUF];
    rio_readlineb(rio, buf, MAXBUF);
    while (strcmp(buf, "\r\n") && strcmp(buf, "\n")) // read either \r\n or \n
    {
        printf("%s", buf);
        rio_readlineb(rio, buf, MAXBUF);
    }
    return;
}

void parse_uri(char *uri, char *host, char *port, char *path)
{
    char *first_colon, *second_colon, *slash_aft_port;
    first_colon = strchr(uri, ':');
    first_colon += 3;
    // 192.168.1.2:8080/cgi-bin/adder?1&1
    second_colon = strchr(first_colon, ':');
    if (second_colon == NULL)
    {
        strcpy(port, "80");
        // first_colon: 192.168.1.2/cgi-bin/adder?1&1
        slash_aft_port = strchr(first_colon, '/');
        if (slash_aft_port == NULL)
        {
            // 192.168.1.2
            strcpy(host, first_colon);
            strcpy(path, "/");
            return;
        }
        *slash_aft_port = '\0';
        // first_colon: 192.168.1.2 \0 cgi-bin/adder?1&1
        strcpy(host, first_colon);
        strcpy(path, "/");
        strcat(path, slash_aft_port + 1);
    }
    else
    {
        *second_colon = '\0';
        // 192.168.1.2 \0 8080/cgi-bin/adder?1&1
        strcpy(host, first_colon);
        slash_aft_port = strchr(second_colon + 1, '/');
        if (slash_aft_port == NULL)
        {
            // 192.168.1.2 \0 8080
            strcpy(host, first_colon);
            strcpy(port, second_colon + 1);
            strcpy(path, "/");
            return;
        }
        *slash_aft_port = '\0';
        // 192.168.1.2 \0 8080 \0 cgi-bin/adder?1&1
        strcpy(port, second_colon + 1);
        strcpy(path, "/");
        strcat(path, slash_aft_port + 1);
    }
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
    pthread_t tid;

    Signal(SIGPIPE, SIG_IGN);
    listenfd = open_listenfd(argv[1]);
    int *ptr, i;
    while (1)
    {
        clilen = sizeof(struct sockaddr_storage);
        ptr = malloc(sizeof(int));
        connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);
        *ptr = connfd;
        Getnameinfo((SA *)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to client: %s:%s\n", host, port);
        pthread_create(&tid, NULL, doit, ptr);
    }

    printf("%s", user_agent_hdr);
    return 0;
}
