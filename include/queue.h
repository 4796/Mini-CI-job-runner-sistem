#ifndef QUEUE_H
#define QUEUE_H

#include "list_node.h"
#include <pthread.h>

typedef struct {
    ListNode *head;
    ListNode *tail;
    int    count;
    pthread_mutex_t lock;
    pthread_cond_t  cv_not_empty;
    int    shutdown;
} JobQueue;

void queue_init(JobQueue *q);
void queue_push(JobQueue *q, int value);
int  queue_pop_blocking(JobQueue *q, int *out);
void queue_shutdown(JobQueue *q);
void queue_destroy(JobQueue *q);

#endif /* QUEUE_H */
