#include "csapp.h"

void run_cli(char *host, char *port, int cli_id);

void run_cli(char *host, char *port, int cli_id)
{
    int fd;
    rio_t rio;
    char header[MAXBUF], buf[MAXBUF];

    fd = open_clientfd(host, atoi(port));

    if (fd < 0)
    {
        printf("Client: %d\nFailed to connect to %s %s\n", cli_id, host, port);
        return;
    }

    rio_readinitb(&rio, fd);

    strcpy(header, "Get /ocean.mpg HTTP/1.1\r\n\r\n");

    rio_writen(fd, header, strlen(header));

    int n;
    while ((n = rio_readlineb(&rio, buf, MAXBUF)) > 0)
    {
        printf("%s", buf);
    }
    close(fd);
    printf("Client %d finished successfully.\n", cli_id);
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <host> <port> <number of clients>\n", argv[0]);
        exit(0);
    }

    char host[MAXBUF], port[MAXBUF];
    int n = atoi(argv[3]);
    pid_t pid;

    strcpy(host, argv[1]);
    strcpy(port, argv[2]);

    printf("Spawning %d clients\n", n);

    for (int i = 0; i < n; i++)
    {
        pid = fork();
        if (pid < 0)
            unix_error("Fork error");
        else if (pid == 0)
        {
            run_cli(host, port, i);
            exit(0);
        }
    }
    while (wait(NULL) > 0)
        ;
    printf("All clients completed\n");
    exit(0);
}