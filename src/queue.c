#include "queue.h"
#include <stdlib.h>
#include <string.h>





int queue_init(bounded_queue_t *q, int capacity) {
    // 1. Allocate the circular buffer
    q->buffer = malloc(capacity * sizeof(url_entry_t));
    if (!q->buffer) return -1;  // Allocation failed
    
   
    q->capacity = capacity;
    q->head = 0;        
    q->tail = 0;        
    q->count = 0;       
    q->max_count_seen = 0;
    q->shutdown = false;
    
    
    pthread_mutex_init(&q->mutex, NULL);
    
    
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    
    return 0;  // Success
}

void queue_destroy(bounded_queue_t *q) {
    for (int i = 0; i < q->count; i++) {
        int idx = (q->head + i) % q->capacity;
        free(q->buffer[idx].url);
    }
    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}

int queue_enqueue(bounded_queue_t *q, const char *url, int depth) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == q->capacity && !q->shutdown)
        pthread_cond_wait(&q->not_full, &q->mutex);
    if (q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    q->buffer[q->tail].url = strdup(url);
    q->buffer[q->tail].depth = depth;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    if (q->count > q->max_count_seen) q->max_count_seen = q->count;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

bool queue_dequeue(bounded_queue_t *q, url_entry_t *entry) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0 && !q->shutdown)
        pthread_cond_wait(&q->not_empty, &q->mutex);
    if (q->count == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    *entry = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

void queue_shutdown(bounded_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    q->shutdown = true;
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

int queue_size(bounded_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    int size = q->count;
    pthread_mutex_unlock(&q->mutex);
    return size;
}

int queue_max_size(bounded_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    int max = q->max_count_seen;
    pthread_mutex_unlock(&q->mutex);
    return max;
}