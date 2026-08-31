#include "csapp.h"
#include "pthread.h"
#include "semaphore.h"

typedef struct sbuf_t sbuf_t;
void sbuf_init(sbuf_t *s, int n);
void sbuf_deinit(sbuf_t *s);
void sbuf_insert(sbuf_t *s, int val);
int sbuf_remove(sbuf_t *s);

struct sbuf_t
{
    int *buf;
    int front;
    int rear;
    int max;
    int cnt;
    sem_t mutex;
    sem_t slots;
    sem_t items;
};
