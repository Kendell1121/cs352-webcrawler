#include "visited.h"
#include <stdlib.h>
#include <string.h>

static unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int visited_init(visited_set_t *set, int num_buckets) {
    set->buckets = calloc(num_buckets, sizeof(visited_entry_t*));
    if (!set->buckets) return -1;
    set->num_buckets = num_buckets;
    set->count = 0;
    pthread_mutex_init(&set->mutex, NULL);
    return 0;
}

void visited_destroy(visited_set_t *set) {
    for (int i = 0; i < set->num_buckets; i++) {
        visited_entry_t *entry = set->buckets[i];
        while (entry) {
            visited_entry_t *next = entry->next;
            free(entry->url);
            free(entry);
            entry = next;
        }
    }
    free(set->buckets);
    pthread_mutex_destroy(&set->mutex);
}

bool visited_check_and_add(visited_set_t *set, const char *url) {
    unsigned long h = hash(url) % set->num_buckets;
    pthread_mutex_lock(&set->mutex);
    
    visited_entry_t *entry = set->buckets[h];
    while (entry) {
        if (strcmp(entry->url, url) == 0) {
            pthread_mutex_unlock(&set->mutex);
            return true;
        }
        entry = entry->next;
    }
    
    entry = malloc(sizeof(visited_entry_t));
    entry->url = strdup(url);
    entry->next = set->buckets[h];
    set->buckets[h] = entry;
    set->count++;
    
    pthread_mutex_unlock(&set->mutex);
    return false;
}

int visited_count(visited_set_t *set) {
    pthread_mutex_lock(&set->mutex);
    int count = set->count;
    pthread_mutex_unlock(&set->mutex);
    return count;
}
