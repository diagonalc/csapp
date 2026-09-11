#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <sys/wait.h>
#define MAX_SYSCALL 128

typedef struct syscall_stat syscall_stat;

syscall_stat *get_stat(char *name);
int parse_line(const char *line, regex_t *regex, char *name, size_t name_len, double *time);

struct syscall_stat
{
    char name[64];
    double totaltime;
    long call_cnt;
};

syscall_stat stats[MAX_SYSCALL];
int stat_cnt = 0;

syscall_stat *get_stat(char *name)
{
    for (int i = 0; i < stat_cnt; i++)
    {
        if (strcmp(name, stats[i].name) == 0)
            return &stats[i];
    }
    if (stat_cnt < MAX_SYSCALL)
    {
        strcpy(stats[stat_cnt].name, name);
        stats[stat_cnt].call_cnt = 0;
        stats[stat_cnt].totaltime = 0.0;
        return &stats[stat_cnt++];
    }
    return NULL;
}

int parse_line(const char *line, regex_t *regex, char *name, size_t name_len, double *time)
{
    regmatch_t matches[3]; // matches[0]=整行, matches[1]=函数名, matches[2]=时间

    if (regexec(regex, line, 3, matches, 0) != 0)
    {
        return -1; // 没有匹配
    }

    // 提取函数名（matches[1]）
    int name_start = matches[1].rm_so;
    int name_end = matches[1].rm_eo;
    int len = name_end - name_start;
    if (len >= name_len)
        len = name_len - 1;
    strncpy(name, line + name_start, len);
    name[len] = '\0';

    // 提取时间（matches[2]）
    int time_start = matches[2].rm_so;
    int time_end = matches[2].rm_eo;
    char time_buf[32];
    len = time_end - time_start;
    if (len >= (int)sizeof(time_buf))
        len = sizeof(time_buf) - 1;
    strncpy(time_buf, line + time_start, len);
    time_buf[len] = '\0';

    *time = atof(time_buf); // 字符串转 double

    return 0;
}

int main(int argc, char **argv)
{
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("Pipe error");
        exit(0);
    }
    if (fork() == 0) // child process
    {
        close(fd[0]); // close read end
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);
        char *exec_argv[] = {"strace", "-T", argv[1], NULL};
        char *exec_envp[] = {"PATH=/usr/bin", NULL};
        execve("/usr/bin/strace", exec_argv, exec_envp);
        exit(1);
    }
    else
    {
        close(fd[1]); // close write end
        char buf[4096];
        ssize_t n;
        regex_t regex;
        const char *pattern = "^([a-zA-Z_][a-zA-Z0-9_]*)\\(.*\\) *= .* <([0-9]+\\.[0-9]+)>";
        int ret = regcomp(&regex, pattern, REG_EXTENDED);
        if (ret != 0)
        {
            char err[256];
            regerror(ret, &regex, err, sizeof(err));
            fprintf(stderr, "正则编译失败: %s\n", err);
            return 1;
        }

        FILE *fp = fdopen(fd[0], "r");
        char *line = NULL;
        size_t cap = 0;
        char name[64];
        double time;
        while (getline(&line, &cap, fp) != -1)
        {
            if (parse_line(line, &regex, name, sizeof(name), &time) == 0)
            {
                syscall_stat *s = get_stat(name);
                if (s != NULL)
                {
                    s->totaltime += time;
                    s->call_cnt++;
                }
                //printf("函数: %-10s 耗时: %.6f 秒\n", name, time);
            }
        }
        free(line);
        fclose(fp);
        regfree(&regex);
        wait(NULL);
    }
}
