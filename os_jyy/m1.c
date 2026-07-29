#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#define MAX_ROWS 100
#define MAX_COLS 100

// 地图结构体
typedef struct
{
    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Map;

// 参数解析存储结构体
typedef struct
{
    char *map_file;  // -m / --map
    int player_id;   // -p / --player (范围 0~9, 未指定可设为 -1)
    char *move_dir;  // --move (up, down, left, right)
    bool is_version; // --version
} Options;

// ============================================================================
// 【已实现】错误处理与辅助功能 (允许 AIGC 生成)
// ============================================================================

// 统一错误退出：按 UNIX 规则输出错误并返回状态码 1
void error_exit(const char *msg)
{
    if (msg)
    {
        fprintf(stderr, "Error: %s\n", msg);
    }
    exit(1);
}

// 读取并校验地图合法性 (尺寸、矩形、字符合法性)
void load_and_validate_map(const char *filename, Map *map)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        error_exit("Cannot open map file.");
    }

    map->rows = 0;
    map->cols = -1;

    char line[MAX_COLS + 10]; // 留出缓冲区处理换行
    while (fgets(line, sizeof(line), f))
    {
        // 去除换行符 \n 和 \r
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }
        if (len == 0)
            continue; // 忽略末尾空行

        // 校验列数与行数限制 (<= 100x100)
        if (map->rows >= MAX_ROWS || len > MAX_COLS)
        {
            fclose(f);
            error_exit("Map dimensions exceed 100x100 limit.");
        }

        // 校验形状是否为标准矩形
        if (map->cols == -1)
        {
            map->cols = (int)len;
        }
        else if (map->cols != (int)len)
        {
            fclose(f);
            error_exit("Map is not rectangular.");
        }

        // 校验字符合法性: 仅限 '#', '.', '0'~'9'
        for (int c = 0; c < map->cols; c++)
        {
            char ch = line[c];
            if (ch != '#' && ch != '.' && !(ch >= '0' && ch <= '9'))
            {
                fclose(f);
                error_exit("Invalid character in map file.");
            }
            map->grid[map->rows][c] = ch;
        }
        map->rows++;
    }
    fclose(f);

    if (map->rows == 0 || map->cols == 0)
    {
        error_exit("Map file is empty.");
    }
}

// 检查连通性 (广度优先搜索 BFS)
bool check_connectivity(const Map *map)
{
    bool visited[MAX_ROWS][MAX_COLS] = {false};
    int total_passable = 0;
    int start_r = -1, start_c = -1;

    // 统计所有非墙壁格子（'.' 和 '0'~'9' 均为可通行格子）
    for (int r = 0; r < map->rows; r++)
    {
        for (int c = 0; c < map->cols; c++)
        {
            if (map->grid[r][c] != '#')
            {
                total_passable++;
                if (start_r == -1)
                {
                    start_r = r;
                    start_c = c;
                }
            }
        }
    }

    if (total_passable == 0)
        return true;

    // BFS 队列
    int q_r[MAX_ROWS * MAX_COLS], q_c[MAX_ROWS * MAX_COLS];
    int head = 0, tail = 0;

    q_r[tail] = start_r;
    q_c[tail] = start_c;
    tail++;
    visited[start_r][start_c] = true;
    int visited_count = 0;

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    while (head < tail)
    {
        int cr = q_r[head];
        int cc = q_c[head];
        head++;
        visited_count++;

        for (int i = 0; i < 4; i++)
        {
            int nr = cr + dr[i];
            int nc = cc + dc[i];

            if (nr >= 0 && nr < map->rows && nc >= 0 && nc < map->cols)
            {
                if (!visited[nr][nc] && map->grid[nr][nc] != '#')
                {
                    visited[nr][nc] = true;
                    q_r[tail] = nr;
                    q_c[tail] = nc;
                    tail++;
                }
            }
        }
    }

    return visited_count == total_passable;
}

// 处理玩家登场与移动
void process_player_action(Map *map, int player_id, const char *dir)
{
    char player_ch = '0' + player_id;
    int pr = -1, pc = -1;

    // 1. 查找玩家是否已经在地图上
    for (int r = 0; r < map->rows; r++)
    {
        for (int c = 0; c < map->cols; c++)
        {
            if (map->grid[r][c] == player_ch)
            {
                pr = r;
                pc = c;
                break;
            }
        }
        if (pr != -1)
            break;
    }

    // 2. 玩家尚未登场：在优先找到的第一块空地 '.' 放置玩家
    if (pr == -1)
    {
        for (int r = 0; r < map->rows; r++)
        {
            for (int c = 0; c < map->cols; c++)
            {
                if (map->grid[r][c] == '.')
                {
                    pr = r;
                    pc = c;
                    map->grid[pr][pc] = player_ch;
                    break;
                }
            }
            if (pr != -1)
                break;
        }
        if (pr == -1)
        {
            error_exit("No empty space '.' left to spawn player.");
        }
    }

    // 若未传 --move 参数，说明仅为登场/查看操作，结束
    if (dir == NULL)
        return;

    // 3. 计算移动目标位置
    int tr = pr, tc = pc;
    if (strcmp(dir, "up") == 0)
        tr--;
    else if (strcmp(dir, "down") == 0)
        tr++;
    else if (strcmp(dir, "left") == 0)
        tc--;
    else if (strcmp(dir, "right") == 0)
        tc++;
    else
    {
        error_exit("Invalid direction. Must be up, down, left, or right.");
    }

    // 4. 校验移动目标合法性（边界越界、墙壁 '#' 或其他玩家）
    if (tr < 0 || tr >= map->rows || tc < 0 || tc >= map->cols)
    {
        error_exit("Player move out of bounds.");
    }
    if (map->grid[tr][tc] != '.')
    {
        error_exit("Target square is blocked or occupied.");
    }

    // 5. 更新坐标
    map->grid[pr][pc] = '.';
    map->grid[tr][tc] = player_ch;
}

// 将更新后的地图覆盖写回文件
void save_map(const char *filename, const Map *map)
{
    FILE *f = fopen(filename, "w");
    if (!f)
    {
        error_exit("Failed to open file for writing.");
    }

    for (int r = 0; r < map->rows; r++)
    {
        for (int c = 0; c < map->cols; c++)
        {
            fputc(map->grid[r][c], f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

// 原样输出当前地图到标准输出
void print_map(const Map *map)
{
    for (int r = 0; r < map->rows; r++)
    {
        for (int c = 0; c < map->cols; c++)
        {
            putchar(map->grid[r][c]);
        }
        putchar('\n');
    }
}

// ============================================================================
// 🔴【你需要手写的部分】命令行参数解析
// ============================================================================

void parse_args(int argc, char *argv[], Options *opts)
{
    // 默认初始化
    opts->map_file = NULL;
    opts->player_id = -1;
    opts->move_dir = NULL;
    opts->is_version = false;

    /*
     * TODO: 请用你的双脑与手写代码完成这里的参数解析逻辑！
     *
     * 需要包含的功能：
     * 1. 使用 getopt_long() 循环解析：
     *    - '-m' / '--map' <file>
     *    - '-p' / '--player' <id>
     *    - '--move' <direction>
     *    - '--version'
     * 2. 参数合法性与组合校验：
     *    - 如果遇到未定义选项或解析错误 -> 调用 error_exit(...)
     *    - 如果解析到 -p / --player，校验其值是否为【单个数字字符 '0'~'9'】，转为整数存入 opts->player_id
     *    - 如果使用了 --version，必须确保【不能】同时传入 -m、-p 或 --move 参数
     */
    struct option long_opts[] =
        {
            {"version", no_argument, 0, 'v'},
            {"map", required_argument, 0, 'm'},
            {"player", required_argument, 0, 'p'},
            {"move", required_argument, 0, 'd'},
            {0, 0, 0, 0}};
    int c;
    opts->is_version = false;
    int not_version = 0;
    while ((c = getopt_long(argc, argv, "m:p:", long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case '?':
            error_exit("Invaild option");
            break;
        case 'v':
            opts->is_version = 1;
            if (((c = getopt_long(argc, argv, "m:p:", long_opts, NULL)) != -1) || not_version == 1)
            {
                error_exit("version option can only be used alone");
            }
            break;
        case 'm':
            not_version = 1;
            opts->map_file = optarg;

            break;
        case 'p':
            if (strlen(optarg) != 1 || optarg[0] < '0' || optarg[0] > '9')
                error_exit("Invalid player id");
            not_version = 1;
            opts->player_id = '0';
            opts->player_id = optarg[0] - '0';
            break;
        case 'd':
            opts->move_dir = optarg;
            not_version = 1;
            break;
        }
    }
}

// ============================================================================
// 主程序入口与任务调度
// ============================================================================

int main(int argc, char *argv[])
{
    Options opts;

    // 1. 调用你自己手写的参数解析函数
    parse_args(argc, argv, &opts);

    // 2. 版本号模式处理
    if (opts.is_version)
    {
        printf("Labyrinth Game v1.0\n");
        return 0;
    }

    // 3. 必选参数检查 (-m 和 -p 必须提供)
    if (opts.map_file == NULL || opts.player_id == -1)
    {
        error_exit("Missing mandatory arguments: --map and --player are required.");
    }

    // 4. 读取并校验地图
    Map map;
    load_and_validate_map(opts.map_file, &map);

    // 5. 地图连通性校验
    if (!check_connectivity(&map))
    {
        error_exit("Map is disconnected.");
    }

    // 6. 执行玩家动作（登场/移动）
    process_player_action(&map, opts.player_id, opts.move_dir);

    // 7. 根据是否发生了移动，决定保存还是打印地图
    if (opts.move_dir != NULL)
    {
        save_map(opts.map_file, &map);
    }
    else
    {
        print_map(&map);
    }

    return 0; // 成功执行退出
}