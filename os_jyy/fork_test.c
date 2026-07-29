#include <stdio.h>
#include <unistd.h>

int main()
{
    // int i = 0;
    // printf("main\n");
    // int a = 0;
    // a++;

    // while (1)
    // {

    //     pid_t pid = fork();
    //     printf("i=%d\n", i);
    //     // a++;
    //     i++;
    // }
    for (int i = 0; i < 2; i++)
    {
        fork();
        printf("Hello\n");
    }
}