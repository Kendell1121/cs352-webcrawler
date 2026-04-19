#ifndef VISITED_SET_H
#define VISITED_SET_H

#include <stdbool.h>
#include <pthread.h>

typedef struct visited_entry {
    char *url;
    struct visited_entry *next;
} visited_entry_t;

typedef struct {
    visited_entry_t **buckets;
    int num_buckets;
    int count;
    pthread_mutex_t mutex;
} visited_set_t;

int visited_init(visited_set_t *set, int num_buckets);
void visited_destroy(visited_set_t *set);
bool visited_check_and_add(visited_set_t *set, const char *url);
int visited_count(visited_set_t *set);

#endif
