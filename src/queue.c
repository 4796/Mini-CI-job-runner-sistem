#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void queue_init(JobQueue *q) {
    if (q == NULL) {
        return;
    }
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    q->shutdown = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cv_not_empty, NULL);
}

void queue_push(JobQueue *q, int value) {
    if (q == NULL) {
        return;
    }
    ListNode *node = malloc(sizeof(ListNode));
    if (node == NULL) {
        fprintf(stderr, "Error: memory allocation failed in queue_push\n");
        exit(1);
    }
    node->value = value;
    node->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->tail == NULL) {
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
    q->count++;
    pthread_cond_signal(&q->cv_not_empty); /* signal exactly one waiting worker */
    pthread_mutex_unlock(&q->lock);
}

int queue_pop_blocking(JobQueue *q, int *out) {
    if (q == NULL || out == NULL) {
        return 1;
    }
    pthread_mutex_lock(&q->lock);
    while (q->count == 0 && !q->shutdown) {
        pthread_cond_wait(&q->cv_not_empty, &q->lock);
    }
    if (q->count == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return 1; /* signal caller that queue is shut down and empty */
    }
    ListNode *n = q->head;
    *out = n->value;
    q->head = n->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->count--;
    pthread_mutex_unlock(&q->lock);
    free(n);
    return 0;
}

void queue_shutdown(JobQueue *q) {
    if (q == NULL) {
        return;
    }
    pthread_mutex_lock(&q->lock);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->cv_not_empty); /* wake up all waiting workers to exit */
    pthread_mutex_unlock(&q->lock);
}

void queue_destroy(JobQueue *q) {
    if (q == NULL) {
        return;
    }
    ListNode *cur = q->head;
    while (cur != NULL) {
        ListNode *next = cur->next;
        free(cur);
        cur = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cv_not_empty);
}
