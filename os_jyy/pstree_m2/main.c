#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // 提供 pid_t, getpid, read 等
#include <dirent.h>    // 目录操作
#include <ctype.h>     // 字符判断
#include <stdbool.h>   // bool 类型
#include <sys/types.h> // 显式包含 pid_t 定义（虽然unistd.h已包含）
#include <errno.h>     // 错误处理

// 最大进程数假设，可根据需要动态扩展
#define MAX_PROCESSES 1024
#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 512

// 进程信息结构体
typedef struct
{
    pid_t pid;                 // 进程ID
    pid_t ppid;                // 父进程ID
    char command[MAX_CMD_LEN]; // 进程命令名
} ProcessInfo;

// 全局选项
typedef struct
{
    bool show_pids;    // -p 选项
    bool numeric_sort; // -n 选项
} Options;

Options opts = {false, false};

// 进程数组和数量
ProcessInfo processes[MAX_PROCESSES];
int process_count = 0;

/**
 * 从 /proc/[pid]/stat 或 /proc/[pid]/status 读取进程信息
 * 这是核心的OS交互部分，需要你根据文档 man 5 proc 实现
 * 参数: pid - 进程ID
 * 返回: 填充好的 ProcessInfo 结构体，若读取失败则返回 NULL
 */
ProcessInfo *get_process_info(pid_t pid)
{
    // TODO:
    // 1. 构建路径: /proc/%d/stat
    // 2. 打开文件，读取进程名和父进程ID
    // 3. 解析 stat 文件格式 (提示: 第二字段是命令名，第四字段是ppid)
    // 4. 填充 ProcessInfo 结构体
    // 注意错误处理 (文件可能已被删除)
    char path[128];
    char name[128];
    char state[16];
    int p, pp;
    sprintf(path, "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;
    fscanf(fp, "%d %s %s %d", &p, name, state, &pp);
    ProcessInfo *proc = malloc(sizeof(ProcessInfo));
    // 去除命令名的括号: (bash) -> bash
    int len = strlen(name);
    if (len >= 2 && name[0] == '(' && name[len - 1] == ')')
    {
        strncpy(proc->command, name + 1, len - 2);
        proc->command[len - 2] = '\0';
    }
    else
    {
        strcpy(proc->command, name);
    }

    strcpy(proc->command, name);
    proc->pid = p;
    proc->ppid = pp;
    fclose(fp);
    return proc;
}

/**
 * 扫描 /proc 目录下所有数字命名的子目录，收集进程信息
 * 这是遍历OS进程列表的关键OS交互部分
 */
void collect_processes()
{
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir)
    {
        perror("opendir /proc");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL)
    {
        // 检查目录名是否全是数字 (即为pid)
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0')
            continue; // 非数字名跳过

        // TODO: 调用 get_process_info(pid) 获取进程信息
        // 如果成功，添加到 processes 数组，process_count++

        // 示例 (需替换):
        ProcessInfo *info = get_process_info((pid_t)pid);
        if (info)
            processes[process_count++] = *info;

        // 限制最大数量避免溢出
        if (process_count >= MAX_PROCESSES)
            break;
    }
    closedir(proc_dir);
}

/**
 * 比较函数，用于按pid数值排序 (配合qsort)
 */
int compare_pid(const void *a, const void *b)
{
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    return (pa->pid - pb->pid);
}

/**
 * 递归打印进程树
 * 参数: pid - 当前要打印的进程ID
 *       depth - 当前深度 (用于缩进)
 *       is_last - 用于控制树形符号 (可选)
 */

void print_process_tree(pid_t pid, int depth, bool is_last)
{
    // 1. 在 processes 数组中找到 pid 对应的进程信息
    ProcessInfo *current = NULL;
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == pid)
        {
            current = &processes[i];
            break;
        }
    }
    if (!current)
        return; // 进程不存在，直接返回

    // 2. 打印缩进和树形符号
    // 缩进部分：对于每个深度级别，打印合适的缩进
    for (int i = 0; i < depth; i++)
    {
        if (i == depth - 1)
        {
            // 当前级别：如果是最后一个子节点，用 "└─"，否则用 "├─"
            printf(is_last ? "    " : "│   ");
        }
        else
        {
            // 更深的级别：根据是否是最后一个子节点决定是否显示垂直线
            printf("    "); // 简化版：统一使用空格缩进
        }
    }

    // 3. 打印树形符号和进程名
    if (depth > 0)
    {
        printf(is_last ? "└─ " : "├─ ");
    }

    // 打印进程名
    printf("%s", current->command);
    if (opts.show_pids)
    {
        printf("[%d]", current->pid);
    }
    printf("\n");

    // 4. 收集所有子进程 (ppid == 当前pid)
    pid_t children[MAX_PROCESSES];
    int child_count = 0;
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].ppid == pid)
        {
            children[child_count++] = processes[i].pid;
        }
    }

    // 如果没有任何子进程，直接返回
    if (child_count == 0)
        return;

    // 5. 如果启用 -n 选项，对子进程按pid排序
    if (opts.numeric_sort)
    {
        // 简单的冒泡排序
        for (int i = 0; i < child_count - 1; i++)
        {
            for (int j = 0; j < child_count - 1 - i; j++)
            {
                if (children[j] > children[j + 1])
                {
                    pid_t temp = children[j];
                    children[j] = children[j + 1];
                    children[j + 1] = temp;
                }
            }
        }
    }

    // 6. 递归打印每个子进程
    for (int i = 0; i < child_count; i++)
    {
        // 判断当前子进程是否是最后一个
        bool child_is_last = (i == child_count - 1);
        print_process_tree(children[i], depth + 1, child_is_last);
    }
}

/**
 * 查找根进程 (pid == 1 或 ppid == 0 的进程)
 */
pid_t find_root_pid()
{
    // TODO: 在 processes 数组中查找根进程
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == 1 || processes[i].ppid == 0)
            return processes[i].pid;
    }
    return 1; // 一般systemd/init进程为1
}

/**
 * 解析命令行参数 (支持 -p, -n, -V)
 */
void parse_arguments(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0)
        {
            opts.show_pids = true;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort") == 0)
        {
            opts.numeric_sort = true;
        }
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
        {
            printf("pstree (M2) version 0.1\n");
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s [-p] [-n] [-V]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{
    // 1. 解析命令行参数
    parse_arguments(argc, argv);

    // 2. 收集系统所有进程信息 (核心OS交互)
    collect_processes();

    if (process_count == 0)
    {
        fprintf(stderr, "No processes found.\n");
        return EXIT_FAILURE;
    }

    // 3. 如果启用 -n 选项，对整个进程列表按pid排序 (便于后续处理)
    if (opts.numeric_sort)
    {
        qsort(processes, process_count, sizeof(ProcessInfo), compare_pid);
    }

    // 4. 找到根进程
    pid_t root_pid = find_root_pid();

    // 5. 递归打印进程树
    print_process_tree(root_pid, 0, true);

    return EXIT_SUCCESS;
}