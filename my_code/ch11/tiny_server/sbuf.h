#include "csapp.h"
#include "pthread.h"
#include "semaphore.h"

typedef struct sbuf_t sbuf_t;
void sbuf_init(sbuf_t *s, int n);
void sbuf_deinit(sbuf_t *s);
void sbuf_insert(sbuf *s, int val);
int sbuf_remove(sbuf *s);

struct sbuf
{
    int *buf;
    int front;
    int back;
    int max;
    sem_t mutex;
    sem_t slots;
    sem_t items;
};
