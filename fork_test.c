#include <stdio.h>
#include <unistd.h>

int main()
{
    int i = 0;
    printf("main\n");
    int a = 0;
    while (i < 10)
    {

        pid_t pid = fork();
        printf("While, a=%d, i = %d\n", a, i);
        a++;
        i++;
        sleep(1);
    }
}