#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <sys/wait.h>
#include <sys/time.h>
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

int cmp_by_time(const void *a, const void *b)
{
    const syscall_stat *sa = (const syscall_stat *)a;
    const syscall_stat *sb = (const syscall_stat *)b;
    if (sb->totaltime > sa->totaltime)
        return 1;
    if (sb->totaltime < sa->totaltime)
        return -1;
    return 0;
}

void print_top5(void)
{
    // 1. 计算总耗时
    double grand_total = 0.0;
    for (int i = 0; i < stat_cnt; i++)
    {
        grand_total += stats[i].totaltime;
    }
    if (grand_total <= 0.0)
        return;

    // 2. 复制一份数组（避免排序破坏原数组的顺序）
    syscall_stat sorted[MAX_SYSCALL];
    memcpy(sorted, stats, sizeof(syscall_stat) * stat_cnt);

    // 3. 排序（降序）
    qsort(sorted, stat_cnt, sizeof(syscall_stat), cmp_by_time);

    // 4. 打印前 5 名
    int top = (stat_cnt < 5) ? stat_cnt : 5;
    for (int i = 0; i < top; i++)
    {
        double ratio = sorted[i].totaltime / grand_total * 100.0;
        printf("%s (%d%%)\n", sorted[i].name, (int)ratio);
    }

    // 5. 打印 80 个 '\0' 作为分隔符
    for (int i = 0; i < 80; i++)
    {
        putchar('\0');
    }
    fflush(stdout);
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

        struct timeval last_print, now;
        gettimeofday(&last_print, NULL);

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

                // 检查是否距离上次打印超过 100ms
                gettimeofday(&now, NULL);
                long elapsed_ms = (now.tv_sec - last_print.tv_sec) * 1000 + (now.tv_usec - last_print.tv_usec) / 1000;
                if (elapsed_ms >= 100)
                {
                    print_top5();
                    last_print = now;
                }
            }
        }
        // 循环结束后最后打印一次
        print_top5();

        free(line);
        fclose(fp);
        regfree(&regex);
        wait(NULL);
    }
}

//
