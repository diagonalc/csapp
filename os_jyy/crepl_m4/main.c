#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <regex.h>
#include <dlfcn.h>
#define MAX_FUNC 100
#define MAX_FUNC_NAME 128

typedef struct func func_t;
struct func
{
    char func_name[MAX_FUNC_NAME];
    int fd;
};

func_t funcs[MAX_FUNC];
int func_cnt = 0;

void insert(char *name, int fd)
{
    func_t nf;
    nf.fd = fd;
    strcpy(nf.func_name, name);
    if (func_cnt < MAX_FUNC)
    {
        funcs[func_cnt++] = nf;
        return;
    }
    printf("Too many functions!\n");
}

int extract_funcname(const char *src, char *out, size_t outsz)
{
    regex_t re;
    // 匹配: int  <空白>  函数名  <空白>*  (
    const char *pattern = "int[ \t\r\n]+([A-Za-z_][A-Za-z0-9_]*)[ \t\r\n]*\\(";

    if (regcomp(&re, pattern, REG_EXTENDED) != 0)
        return -1;

    regmatch_t m[2];
    int rc = regexec(&re, src, 2, m, 0);
    if (rc != 0)
    {
        regfree(&re);
        return -1; // 没匹配到
    }

    size_t len = m[1].rm_eo - m[1].rm_so;
    if (len >= outsz)
    {
        regfree(&re);
        return -1;
    }

    memcpy(out, src + m[1].rm_so, len);
    out[len] = '\0';

    regfree(&re);
    return 0;
}

int main()
{

    while (1)
    {
        char tt[] = "./temp/func_XXXXXX.c";
        char *line = readline(">> ");
        if (!line)
            break;

        if (strncmp(line, "int", 3) == 0)
        {
            printf("Its a function\n");
            int fd = mkstemps(tt, 2);
            write(fd, line, strlen(line));
            char name[MAX_FUNC_NAME];
            if (extract_funcname(line, name, MAX_FUNC_NAME) == -1)
            {
                printf("Extraction failed\n");
                return -1;
            }
            insert(name, fd);
            printf("name: %s, fd: %d\n", funcs[func_cnt - 1].func_name, funcs[func_cnt - 1].fd);

            char so_p[128];
            strcpy(so_p, tt);
            strcpy(so_p + strlen(so_p) - 2, ".so");
            char *argv[] = {"gcc", "-shared", "-fPIC", "-Wno-implicit-function-declaration",
                            "-o", so_p, tt, NULL};
            if (fork() == 0)
            {
                execvp("gcc", argv);
                perror("Error during compilation");
                exit(-1);
            }

            void *h = dlopen(so_p, RTLD_NOW | RTLD_GLOBAL);
            if (!h)
                return -1;

            // unlink(tt);
            // unlink(so_p);
        }
        free(line);
    }

    for (int i = 0; i < func_cnt; i++)
        close(funcs[i].fd);
    exit(0);
}
