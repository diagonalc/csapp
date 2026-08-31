#include "csapp.h"
#include "sbuf.h"
#define SBUF_SIZE 16
#define NTHREAD 4
#define MAXTHREAD 16

void doit(int fd, int pn);
int read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);
void *monitor(void *a);

sbuf_t s;
int thread_no;
sem_t m_tno;
int peak;

void *monitor(void *a)
{
    while (1)
    {
        sleep(1);
        P(&m_tno);
        int t = thread_no;
        V(&m_tno);
        printf("[monitor] thread_no=%d peak_cnt=%d\n", t, peak);
        fflush(stdout);
    }
    return NULL;
}

void doit(int fd, int pn)
{
    rio_t rio;
    int is_static;
    struct stat statbuf;
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiarg[MAXLINE];

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s\n", buf);
    if (sscanf(buf, "%s %s %s", method, uri, version) < 3)
    {
        clienterror(fd, buf, "400", "Bad Request", "Tiny web server received a malformed request");
        return;
    }
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD") && strcasecmp(method, "POST"))
    {
        clienterror(fd, method, "501", "Not Implemented", "Tiny web server has not implement this method yet");
        return;
    }
    int content_length = read_requesthdrs(&rio);
    is_static = parse_uri(uri, filename, cgiarg);

    if (strcasecmp(method, "POST") == 0)
    {
        if (content_length > 0)
        {
            if (content_length >= MAXLINE)
            {
                clienterror(fd, filename, "412", "Bad Request", "POST body too large");
                return;
            }
            rio_readnb(&rio, cgiarg, content_length);
            cgiarg[content_length] = '\0';
        }
    }

    if (stat(filename, &statbuf) < 0)
    {
        clienterror(fd, filename, "404", "Not found", "Required file cannot be found");
        return;
    }

    if (is_static)
    {
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
        {
            clienterror(fd, filename, "403", "Forbidden", "Required file cannot be read");
            return;
        }
        serve_static(fd, filename, statbuf.st_size, method);
    }
    else
    {
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode))
        {

            clienterror(fd, filename, "403", "Forbidden", "Required CGI program cannot be run");
            return;
        }
        serve_dynamic(fd, filename, cgiarg, method);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];
    int len = 0;
    len += sprintf(body + len, "<html><title>Tiny Error</title>");
    len += sprintf(body + len, "<body bgcolor=\"ffffff\">\r\n");
    len += sprintf(body + len, "%s: %s\r\n", errnum, shortmsg);
    len += sprintf(body + len, "<p>%s: %s\r\n", longmsg, cause);
    len += sprintf(body + len, "<hr><em>The Tiny Web server</em>\r\n");

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}

int read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    int content_length = 0;
    rio_readlineb(rp, buf, MAXLINE);

    while (strcmp(buf, "\r\n") && strcmp(buf, "\n"))
    {
        printf("%s", buf);

        if (strncasecmp(buf, "Content-Length:", 15) == 0)
            content_length = atoi(buf + 15);
        rio_readlineb(rp, buf, MAXLINE);
    }
    return content_length;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin"))
    {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (filename[strlen(filename) - 1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else
    {
        ptr = strchr(uri, '?');
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else
        {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[128], buf[MAXBUF];
    get_filetype(filename, filetype);
    int ofs = 0;
    ofs += sprintf(buf + ofs, "HTTP/1.0 200 OK\r\n");
    ofs += sprintf(buf + ofs, "Server: Tiny Web Server\r\n");
    ofs += sprintf(buf + ofs, "Connection: close\r\n");
    ofs += sprintf(buf + ofs, "Content-length: %d\r\n", filesize);
    ofs += sprintf(buf + ofs, "Content-type: %s\r\n\r\n", filetype);
    if (rio_writen(fd, buf, strlen(buf)) != strlen(buf))
    {
        if (errno == EPIPE)
            fprintf(stderr, "EPIPE: Client closed connection before receiving headers.\n");
        return;
    }

    printf("Response headers:\n");
    printf("%s", buf);
    if (strcasecmp(method, "HEAD") == 0)
        return;

    srcfd = open(filename, O_RDONLY, 0);
    srcp = malloc(filesize);
    rio_readn(srcfd, srcp, filesize);
    close(srcfd);
    if (rio_writen(fd, srcp, filesize) != filesize)
    {
        if (errno == EPIPE)
            fprintf(stderr, "EPIPE: Client closed connection prematurely while writing body.\n");
    }

    free(srcp);
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
    else if (strstr(filename, ".mpg") || strstr(filename, "mp4"))
        strcpy(filetype, "video/mpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
    char buf[MAXLINE];
    char *emptylist[] = {NULL};
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    rio_writen(fd, buf, strlen(buf));

    if (fork() == 0)
    {
        setenv("QUERY_STRING", cgiargs, 1);
        setenv("REQUEST_METHOD", method, 1);
        dup2(fd, STDOUT_FILENO);
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
    signal(SIGPIPE, SIG_IGN);
    int listenfd, connfd;
    char host[MAXLINE];
    char port[MAXLINE];
    struct sockaddr_storage cliaddr;
    socklen_t clilen;
    pthread_t tid[MAXTHREAD];

    sbuf_init(&s, SBUF_SIZE);
    thread_no = 0;

    sem_init(&m_tno, 0, 1);

    for (int i = 0; i < NTHREAD; i++)
        pthread_create(&tid[i], NULL, thread, NULL);

    peak = 0;
    pthread_t mon;
    pthread_create(&mon, NULL, monitor, NULL);

    listenfd = open_listenfd(atoi(argv[1]));
    while (1)
    {

        clilen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&cliaddr, &clilen);
        getnameinfo((SA *)&cliaddr, clilen, host, MAXLINE, port, MAXLINE, 0);
        printf("Connected to: %s:%s\n", host, port);

        P(&s.mutex);
        int is_full = (s.cnt == s.max);
        if (s.cnt > peak) peak = s.cnt;
        V(&s.mutex);
        if (is_full)
        {
            P(&m_tno);
            int temp = thread_no;
            for (int i = temp; i < temp * 2 && i < MAXTHREAD; i++)
            {
                pthread_create(&tid[i], NULL, thread, NULL);
                printf("A new thread is created\n");
                fflush(stdout);
            }
            V(&m_tno);
        }
        sbuf_insert(&s, connfd);
    }

    exit(0);
}

void *thread(void *vargp)
{
    pthread_detach(pthread_self());
    P(&m_tno);
    int no = thread_no;
    thread_no++;
    V(&m_tno);

    while (1)
    {
        int fd = sbuf_remove(&s);

        if (fd == -1)
        {
            printf("Thread exiting\n");
            fflush(stdout);
            P(&m_tno);
            thread_no--;
            V(&m_tno);
            return NULL;
        }

        P(&s.mutex);
        int is_emtpy = (s.cnt == 0);
        if (s.cnt > peak) peak = s.cnt;
        V(&s.mutex);

        if (is_emtpy)
        {
            P(&m_tno);
            int temp = thread_no;
            V(&m_tno);
            if (temp > NTHREAD)
            {
                for (int i = temp / 2; i < temp; i++)
                {
                    sbuf_insert(&s, -1);
                }
            }
        }

        doit(fd, no);
        close(fd);
    }
    return NULL;
}
