#include "sbuf.h"

void sbuf_init(sbuf_t *s, int n);
void sbuf_deinit(sbuf_t *s);
void sbuf_insert(sbuf_t *s, int val);
int sbuf_remove(sbuf_t *s);

void sbuf_init(sbuf_t *s, int n)
{
    s->buf = (int *)malloc(sizeof(int) * n);
    s->front = 0;
    s->rear = 0;
    s->max = n;
    s->cnt = 0;
    sem_init(&s->mutex, 0, 1);
    sem_init(&s->slots, 0, n);
    sem_init(&s->items, 0, 0);
}

void sbuf_deinit(sbuf_t *s)
{
    free(s->buf);
    s->buf = NULL;
}

void sbuf_insert(sbuf_t *s, int val)
{
    P(&s->slots); // no need to write a if conditioning to check if there's available slots, s->slots has a max of n
    P(&s->mutex);
    s->buf[(++s->rear) % s->max] = val;
    s->cnt++;
    V(&s->mutex);
    V(&s->items);
    // no need to V(&s->slots); as a item has occuplied a slot
}

int sbuf_remove(sbuf_t *s)
{
    int item;
    P(&s->items); // same here, no need to check if there is any item
    P(&s->mutex);
    item = s->buf[(++s->front) % s->max];
    s->cnt--;
    V(&s->mutex);
    V(&s->slots);
    return item;
}
