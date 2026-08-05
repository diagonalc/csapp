#include "csapp.h"

void *thread(void *argv)
{
    printf("Hihi\n");
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("format: %s <num>\n", argv[0]);
        exit(0);
    }

    pthread_t *tid;
    int n = atoi(argv[1]);
    tid = malloc(n * sizeof(pthread_t));
    // for (int i = 0; i < n; i++)
    //     tid[i] = 0;
    int i, *ptr;
    for (i = 0; i < n; i++)
    {
        // ptr = malloc(sizeof(int));
        // *ptr = i;
        pthread_create(&tid[i], NULL, thread, ptr);
    }
    for (i = 0; i < n; i++)
    {
        pthread_join(tid[i], NULL);
    }
    exit(0);
}