#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("Pipe error");
        exit(0);
    }
    // if (fork() == 0) // child process
    {

        close(fd[0]);
        dup2(STDERR_FILENO, STDOUT_FILENO);
        char *exec_argv[3] = {"strace", "-T", argv[1]};
        char *exec_envp[] = {"PATH=/usr/bin", NULL};
        execve("strace", exec_argv, exec_envp);
    }
}