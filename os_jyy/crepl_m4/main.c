#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <regex.h>
#include <dlfcn.h>
#include <sys/wait.h>
#define MAX_FUNC 100
#define MAX_FUNC_NAME 128

typedef struct func func_t;
struct func
{
    char func_name[MAX_FUNC_NAME];
    void *handle;
    int (*entry)(void);
};

func_t funcs[MAX_FUNC];
int func_cnt = 0;

void insert(char *name, const void *handle, const int (*entry)(void))
{
    func_t nf;
    nf.handle = handle;
    nf.entry = entry;
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

int build_and_load(const char *c_path, const char *func_name, void **handle, int (**entry)(void))
{
    // dlsym is returning a function's address, so we need a funtion pointer to receive it
    // int (**entry)(void); means entry is a pointer of a function pointer, the function receive void and return int type data
    // int *(*entry)(void); means entry is a function pointer, the function receive void and return int * type data

    char so_path[128];
    strcpy(so_path, c_path);
    strcpy(so_path + strlen(so_path) - 2, ".so");
    char *argv[] = {"gcc", "-shared", "-fPIC", "-Wno-implicit-function-declaration",
                    "-o", so_path, c_path, NULL};
    pid_t pid = fork();
    if (pid == 0)
    {
        execvp("gcc", argv);

        perror("Error during compilation");
        exit(-1);
    }
    waitpid(pid, NULL, NULL);
    *handle = dlopen(so_path, RTLD_NOW);
    if (!(*handle))
        return -1;
    *entry = dlsym(*handle, func_name);
    // unlink(c_path);
    // unlink(so_path);
}

int main()
{

    while (1)
    {
        char c_path[] = "./temp/func_XXXXXX.c";
        char *line = readline(">> ");
        if (!line)
            break;

        if (strncmp(line, "int", 3) == 0)
        {
            printf("Its a function\n");
            int fd = mkstemps(c_path, 2);
            write(fd, line, strlen(line));
            close(fd);
            char name[MAX_FUNC_NAME];
            if (extract_funcname(line, name, MAX_FUNC_NAME) == -1)
            {
                printf("Extraction failed\n");
                return -1;
            }

            void *h;
            int (*entry)(void);
            build_and_load(c_path, name, &h, &entry);
            insert(name, h, entry);
            printf("name: %s\n", funcs[func_cnt - 1].func_name);
        }

        free(line);
    }
    for (int i = 0; i < func_cnt; i++)
        dlclose(funcs[i].handle);
    exit(0);
}

// save
