#include <stdio.h>
#include <string.h>
#include <dirent.h>

int main(void)
{
    DIR *dir = opendir("..");
    if (dir == NULL)
    {
        perror("Unable to open directory");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Exclude "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Check if entry is a directory
        if (entry->d_type == DT_DIR)
        {
            printf("%s/\n", entry->d_name);
        }
    }

    closedir(dir);
    return 0;
}