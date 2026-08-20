/*练习题 12.20
考虑读者—写者问题的一个更简单的变种，即最多只有 N 个读者。推导出一个解答，给予读者和写者同等的优先级，即等待中的读者和写者被赋予对资源访问的同等的机会。提示：你可以用一个计数信号量和一个互斥锁来解决这个问*/

#include <csapp.h>
#define N 5

sem_t s;
sem_t lock;

void reader()
{
    while (1)
    {
        P(&lock);
        P(&s);
        V(&lock);
        // reading
        V(&s);
    }
};
void writer()
{
    while (1)
    {
        P(&lock);
        for (int i = 0; i < N; i++) // cannot write when there are readers
            P(&s);                  // occupy all five quota to make sure there is no reader
        // writing
        for (int i = 0; i < N; i++)
            V(&s);
        V(&lock);
    }
};

int main()
{
    sem_init(&s, 0, N);
    sem_init(&lock, 0, 1);
}