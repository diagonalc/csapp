#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef DT_DIR
#define DT_DIR 4
#endif

// Node structure using First-Child / Next-Sibling representation
typedef struct ProcessNode
{
    int pid;
    int ppid;
    char name[256];
    struct ProcessNode *first_child;
    struct ProcessNode *next_sibling;
} ProcessNode;

bool is_pid_folder(const char *folder)
{
    if (!folder || *folder == '\0')
        return false;
    for (int i = 0; folder[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)folder[i]))
            return false;
    }
    return true;
}

// Fetch PPid and Name (Comm) from /proc/<pid>/status
bool get_process_info(int pid, int *ppid, char *name_buf, size_t name_buf_size)
{
    char path[64], line[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (!fp)
        return false;

    *ppid = -1;
    name_buf[0] = '\0';

    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "Name:\t", 6) == 0)
        {
            sscanf(line, "Name:\t%255s", name_buf);
        }
        else if (strncmp(line, "PPid:\t", 6) == 0)
        {
            sscanf(line, "PPid:\t%d", ppid);
        }
    }
    fclose(fp);
    return (*ppid != -1);
}

// Helper: Add a child to a parent process
void add_child(ProcessNode *parent, ProcessNode *child)
{
    child->next_sibling = parent->first_child;
    parent->first_child = child;
}

// Recursively print the tree with indentation
void print_tree(ProcessNode *node, int depth)
{
    if (!node)
        return;

    for (int i = 0; i < depth; i++)
        printf("  │ ");
    printf("  ├─ [%d] %s\n", node->pid, node->name);

    // Print children
    ProcessNode *child = node->first_child;
    while (child)
    {
        print_tree(child, depth + 1);
        child = child->next_sibling;
    }
}

int main(void)
{
    DIR *proc = opendir("/proc");
    if (!proc)
    {
        perror("Failed to open /proc");
        exit(1);
    }

    // Dynamic map to lookup nodes by PID
    int map_capacity = 32768;
    ProcessNode **node_map = calloc(map_capacity, sizeof(ProcessNode *));

    struct dirent *entry;

    // PASS 1: Read /proc and create all nodes
    while ((entry = readdir(proc)) != NULL)
    {
        if (entry->d_type == DT_DIR && is_pid_folder(entry->d_name))
        {
            int pid = atoi(entry->d_name);
            int ppid;
            char name[256];

            if (!get_process_info(pid, &ppid, name, sizeof(name)))
                continue;

            ProcessNode *node = calloc(1, sizeof(ProcessNode));
            node->pid = pid;
            node->ppid = ppid;
            strncpy(node->name, name, sizeof(node->name) - 1);

            // Resize lookup map if PID exceeds capacity
            if (pid >= map_capacity)
            {
                int old_cap = map_capacity;
                map_capacity = pid + 1000;
                node_map = realloc(node_map, map_capacity * sizeof(ProcessNode *));
                memset(node_map + old_cap, 0, (map_capacity - old_cap) * sizeof(ProcessNode *));
            }

            node_map[pid] = node;
        }
    }
    closedir(proc);

    // PASS 2: Connect parent-child links
    ProcessNode *root = NULL;

    for (int i = 0; i < map_capacity; i++)
    {
        ProcessNode *node = node_map[i];
        if (!node)
            continue;

        // PID 1 (systemd/init) or PID 0 are root nodes
        if (node->pid == 1 || node->ppid == 0)
        {
            root = node;
        }
        else if (node->ppid < map_capacity && node_map[node->ppid])
        {
            // Parent exists in our map -> link them
            add_child(node_map[node->ppid], node);
        }
    }

    // Print the process hierarchy
    if (root)
    {
        printf("Process Hierarchy:\n");
        print_tree(root, 0);
    }
    else
    {
        printf("Root process not found.\n");
    }

    // Clean up memory
    for (int i = 0; i < map_capacity; i++)
    {
        if (node_map[i])
            free(node_map[i]);
    }
    free(node_map);

    return 0;
}