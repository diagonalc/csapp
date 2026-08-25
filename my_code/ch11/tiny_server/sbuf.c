#include "csapp.h"
#include "sbuf.h"

void sbuf_init(sbuf_t *s, int n);
void sbuf_deinit(sbuf_t *s);
void sbuf_insert(sbuf *s, int val);
int sbuf_remove(sbuf *s);

