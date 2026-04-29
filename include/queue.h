#ifndef BOUNDED_QUEUE_H
#define BOUNDED_QUEUE_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    char *url;
    int depth;
} url_entry_t;

typedef struct {
    url_entry_t *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    int max_count_seen;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool shutdown;
} bounded_queue_t;

int queue_init(bounded_queue_t *q, int capacity);
void queue_destroy(bounded_queue_t *q);
int queue_enqueue(bounded_queue_t *q, const char *url, int depth);
bool queue_dequeue(bounded_queue_t *q, url_entry_t *entry);
void queue_shutdown(bounded_queue_t *q);
int queue_size(bounded_queue_t *q);
int queue_max_size(bounded_queue_t *q);

#endif