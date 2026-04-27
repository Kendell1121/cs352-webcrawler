#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include "visited.h"

#define NUM_THREADS 4
#define QUEUE_CAP 1024
#define HASH_BUCKETS 1024
#define MAX_DEPTH 3 



static bounded_queue_t work_queue; // all threads read from and write to this queue, if removed crawler cannot store pending URLs
static visited_set_t visited; // necessary to prevent loops

static int idle_count =0; // counts the amount of worker threads that are idle, if every thread is idle , queue is empty
static pthread_mutex_t idle_mutex =PTHREAD_MUTEX_INITIALIZER; // protects idle_count , mutex is necessary to prevent race conditions
static pthread_cond_t idle_cond = PTHREAD_COND_INITIALIZER;

static void *worker(void *arg)
{
    (void)arg;

    while (1){
        url_entry_t entry;


        pthread_mutex_lock(&idle_mutex);
        idle_count++;

        if (idle_count == NUM_THREADS && queue_size(&work_queue) == 0){
            queue_shutdown(&work_queue);

            pthread_cond_broadcast(&idle_cond);
            pthread_mutex_unlock(&idle_mutex);
            break;
        }
        pthread_mutex_unlock(&idle_mutex);

        bool got = queue_dequeue(&work_queue, &entry);

        pthread_mutex_lock(&idle_mutex);
        idle_count--;
        pthread_mutex_unlock(&idle_mutex);

        if (!got) break;



    }
}