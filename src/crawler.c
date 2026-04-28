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


        printf("[depth %d] Crawling: %s\n", entry.depth, entry.url);

        char *html = fetch_html(entry.url);
        if (!html) {
            fprintf(stderr, "  fetch failed: %s\n", entry.url);
            free(entry.url);
            continue;
    
        }

        if (entry.depth < MAX_DEPTH) {
            int link_count = 0;
            char **links   = extract_urls(html, entry.url, &link_count);

            for (int i = 0l; i < link_count; i++) {

                if (!visited_check_and_add(&visited, links[i])){

                    if (queue_enqueue(&work_queue, links[i], entry.depth + 1) != 0){
                        
                        fprintf(stderr, " queue full, dropping: %s\n", links[i]);
                    }
                } 
                free(links[i]);
            }
            free(links);
        }

        free(html);
        free(entry.url);

    }

    return NULL;

}


void crawler_run(const char *seed_url)
{
    if (queue_init (&work_queue, QUEUE_CAP) != 0){
        fprintf(stderr, "crawler_run: queue_init failed\n");
        queue_destroy(&work_queue);
        return;

    }

    visited_check_and_add(&visited, seed_url);
    if (queue_enqueue(&work_queue, seed_url, 0) != 0) {
        fprintf(stderr, "crawler_run: failure to enqueue seed URL\n");
        queue_destroy(&work_queue);
        visited_destroy(&visited);
        return;

    }

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++){
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0){
            perror("pthread_create");

            queue_shutdown(&work_queue);
            for (int j = 0; j < i; j++) pthread_join(threads[j], NULL);
            queue_destroy(&work_queue);
            visited_destroy(&visited);
            return;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("\n=== Crawl complete ===\n");
    printf(" Total URLs Visited : %d\n", visited_count(&visited));
    printf(" Peak Queue Size   : %d\n", queue_max_size(&work_queue));

    queue_destroy(&work_queue);
    visited_destroy(&visited);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <seed-url>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Starting crawl from: %s\n", argv[1]);
    printf("Threads: %d  | Max Depth: %d\n\n", NUM_THREADS, MAX_DEPTH);

    crawler_run(argv[1]);
    return EXIT_SUCCESS;
}




