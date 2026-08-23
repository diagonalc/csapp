#include "csapp.h"
#define N 3

int i = 0;
int j = 0;
sem_t lock;
int m1[N][N] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
int m2[N][N] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
int result[N][N] = {0};

void *mult(void *vargp)
{
    // pthread_detach(pthread_self());
    int r, c;
    P(&lock);
    r = i;
    c = j;
    j++;
    if (j == N)
    {
        j = 0;
        i++;
    }
    // if(i == N)
    V(&lock);
    int re = 0;
    for (int i = 0; i < N; i++)
        re += m1[r][i] * m2[i][c];
    // P(&lock);
    result[r][c] = re;
    // V(&lock);
}

int main()
{
    pthread_t tid[N * N];
    sem_init(&lock, 0, 1);
    for (int i = 0; i < N * N; i++)
        pthread_create(&tid[i], NULL, mult, NULL);
    for (int i = 0; i < N * N; i++)
        pthread_join(tid[i], NULL);
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            printf("%d ", result[i][j]);
        }
        printf("\n");
    }
}

//dfd
